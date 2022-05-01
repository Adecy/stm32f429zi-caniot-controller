#include <zephyr.h>

#include "mydevices.h"

#include "uart_ipc/ipc_frame.h"
#include "uart_ipc/ipc.h"

#include "ble/xiaomi_record.h"
#include "net_time.h"

#include "caniot/datatype.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(devices, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

struct {
	struct k_mutex mutex;
	struct mydevice list[20];
	uint8_t count;
} devices = {
	.mutex = Z_MUTEX_INITIALIZER(devices.mutex),
	.count = 0U
};

static bool phys_addr_valid(mydevice_phys_addr_t *addr)
{
	return (addr->medium == MYDEVICE_MEDIUM_TYPE_BLE) ||
		(addr->medium == MYDEVICE_MEDIUM_TYPE_CAN);
}

static int phys_addr_cmp(mydevice_phys_addr_t *addr1, mydevice_phys_addr_t *addr2)
{
	if ((phys_addr_valid(addr1) == false) || (phys_addr_valid(addr2) == false)) {
		return -EINVAL;
	}

	int ret = -1;

	if (addr1->medium == addr2->medium) {
		if (addr1->medium == MYDEVICE_MEDIUM_TYPE_BLE) {
			ret = bt_addr_le_cmp(&addr1->addr.ble, &addr2->addr.ble);
		} else if (addr1->medium == MYDEVICE_MEDIUM_TYPE_CAN) {
			ret = caniot_deviceid_cmp(addr1->addr.caniot,
						  addr2->addr.caniot);
		}
	}

	return ret;
}

static struct mydevice *get_device_by_addr(mydevice_phys_addr_t *addr)
{
	struct mydevice *device = NULL;

	for (struct mydevice *dev = devices.list;
	     dev < devices.list + devices.count;
	     dev++) {

		if (phys_addr_cmp(addr, &dev->addr) == 0) {
			device = dev;
			break;
		}
	}

	return device;
}

static struct mydevice *get_first_device_by_type(mydevice_type_t type)
{
	struct mydevice *device = NULL;

	for (struct mydevice *dev = devices.list;
	     dev < devices.list + devices.count;
	     dev++) {
		if (dev->type == type) {
			device = dev;
			break;
		}
	}

	return device;
}

static struct mydevice *mydevice_get(mydevice_phys_addr_t *addr,
				     mydevice_type_t type)
{
	struct mydevice *device = NULL;

	
	/* Get device by address if possible */
	if (phys_addr_valid(addr)) {
		device = get_device_by_addr(addr);

	/* if medium type is not set, device should be
	 * differienciated using their device_type */
	} else {
		device = get_first_device_by_type(type);
	}

	return device;
}

static void mydevice_clear(struct mydevice *dev)
{
	memset(dev, 0U, sizeof(*dev));
}

/**
 * @brief Register a new device in the list, addr duplicates are not verified
 * 
 * @param medium 
 * @param type 
 * @param addr 
 * @return int 
 */
static struct mydevice *mydevice_register(mydevice_phys_addr_t *addr,
					  mydevice_type_t type)
{
	if (devices.count >= ARRAY_SIZE(devices.list)) {
		return NULL;
	}

	struct mydevice *dev = devices.list + devices.count;

	mydevice_clear(dev);

	dev->addr = *addr;
	dev->type = type;
	dev->registered_timestamp = net_time_get();

	devices.count++;

	return dev;
}

static struct mydevice *mydevice_get_or_register(mydevice_phys_addr_t *addr,
						 mydevice_type_t type)
{
	struct mydevice *dev;

	dev = mydevice_get(addr, type);
	
	if (dev == NULL) {
		dev = mydevice_register(addr, type);
	}

	return dev;
}

static bool mydevice_match_filter(struct mydevice *dev, mydevice_filter_t *filter)
{
	if (dev == NULL) {
		return false;
	}

	if (filter == NULL) {
		return true;
	}

	if (filter->type == MYDEVICE_FILTER_NONE) {
		return true;
	}

	switch (filter->type) {
	case MYDEVICE_FILTER_TYPE_MEDIUM:
		return dev->addr.medium == filter->data.medium;
	case MYDEVICE_FILTER_TYPE_DEVICE_TYPE:
		return dev->type == filter->data.device_type;
	case MYDEVICE_FILTER_TYPE_MEASUREMENTS_TIMESTAMP:
		return dev->data.measurements_timestamp >= filter->data.timestamp;
	default:
		LOG_WRN("Unknown filter type %hhu", filter->type);
		break;
	}

	return false;
}

/*___________________________________________________________________________*/

size_t mydevice_iterate(void (*callback)(struct mydevice *dev,
					 void *user_data),
			mydevice_filter_t *filter,
			void *user_data)
{
	k_mutex_lock(&devices.mutex, K_FOREVER);

	size_t count = 0U;

	for (struct mydevice *dev = devices.list;
	     dev < devices.list + devices.count;
	     dev++) {
		if (mydevice_match_filter(dev, filter)) {
			callback(dev, user_data);
			count++;
		}
	}

	k_mutex_unlock(&devices.mutex);

	return count;
}

size_t mydevice_xiaomi_iterate(void (*callback)(struct mydevice *dev,
						void *user_data),
			       void *user_data)
{
	mydevice_filter_t filter = {
		.type = MYDEVICE_FILTER_TYPE_DEVICE_TYPE,
		.data = {
			.device_type = MYDEVICE_TYPE_XIAOMI_MIJIA
		}
	};

	return mydevice_iterate(callback, &filter, user_data);
}

/*___________________________________________________________________________*/

static int handle_ble_xiaomi_record(xiaomi_record_t *rec)
{
	/* check if device already exists */
	mydevice_phys_addr_t record_addr = {
		.medium = MYDEVICE_MEDIUM_TYPE_BLE,
	};

	bt_addr_le_copy(&record_addr.addr.ble, &rec->addr);

	struct mydevice *dev = mydevice_get_or_register(&record_addr,
							MYDEVICE_TYPE_XIAOMI_MIJIA);
	if (dev == NULL) {
		LOG_ERR("Failed to register device, list full %hhu / %lu",
			devices.count,
			ARRAY_SIZE(devices.list));
		return -ENOMEM;
	}

	/* device does exist now, update it measurements */
	dev->data.measurements_timestamp = rec->time;
	dev->data.xiaomi.temperature.type = MYDEVICE_SENSOR_TYPE_EMBEDDED;
	dev->data.xiaomi.temperature.value = rec->measurements.temperature;
	dev->data.xiaomi.humidity = rec->measurements.humidity;
	dev->data.xiaomi.battery_level = rec->measurements.battery;

	return 0;
}

int mydevices_register_ble_xiaomi_dataframe(xiaomi_dataframe_t *frame)
{
	int ret = 0;
	char addr_str[BT_ADDR_LE_STR_LEN];

	// show dataframe records
	LOG_INF("Received BLE Xiaomi records count: %u, frame_time: %u",
		frame->count, frame->time);

	uint32_t frame_ref_time = frame->time;
	uint32_t now = net_time_get();

	k_mutex_lock(&devices.mutex, K_FOREVER);

	// Show all records
	for (uint8_t i = 0; i < frame->count; i++) {
		xiaomi_record_t *const rec = &frame->records[i];
		
		bt_addr_le_to_str(&rec->addr,
				  addr_str,
				  sizeof(addr_str));

		int32_t record_rel_time = rec->time - frame_ref_time;
		uint32_t record_timestamp = now + record_rel_time;
		rec->time = record_timestamp;

		ret = handle_ble_xiaomi_record(rec);
		if (ret != 0) {			
			goto exit;
		}

		
		/* Show BLE address, temperature, humidity, battery
		 *   Only raw values are showed in debug because, there is no formatting (e.g. float)
		 */
		LOG_DBG("BLE Xiaomi record %u [%d s]: addr: %s, " \
			"temp: %d°C, hum: %u %%, bat: %u mV",
			i, record_rel_time,
			log_strdup(addr_str),
			(int32_t)rec->measurements.temperature,
			(uint32_t)rec->measurements.humidity,
			(uint32_t)rec->measurements.battery);
	}

exit:
	k_mutex_unlock(&devices.mutex);

	return ret;
}

int mydevice_register_die_temperature(uint32_t timestamp,
				      float die_temperature)
{
	int ret = 0;

	k_mutex_lock(&devices.mutex, K_FOREVER);
	
	/* check if device already exists */
	mydevice_phys_addr_t record_addr = {
		.medium = MYDEVICE_MEDIUM_TYPE_NONE,
	};

	struct mydevice *dev = mydevice_get_or_register(&record_addr,
							MYDEVICE_TYPE_NUCLEO_F429ZI);
	if (dev == NULL) {
		LOG_ERR("Failed to register device, list full %hhu / %lu",
			devices.count,
			ARRAY_SIZE(devices.list));
		ret = -ENOMEM;
		goto exit;
	}

	/* device does exist now, update it measurements */
	dev->data.measurements_timestamp = timestamp;
	dev->data.nucleo_f429zi.die_temperature = die_temperature;

exit:
	k_mutex_unlock(&devices.mutex);
	
	return ret;
}

int mydevice_register_caniot_telemetry(uint32_t timestamp,
				       union deviceid did,
				       struct caniot_board_control_telemetry *data)
{
	int ret = 0;

	k_mutex_lock(&devices.mutex, K_FOREVER);

	/* check if device already exists */
	mydevice_phys_addr_t phys_addr;
	phys_addr.medium = MYDEVICE_MEDIUM_TYPE_CAN;
	phys_addr.addr.caniot = did;

	struct mydevice *dev = mydevice_get_or_register(&phys_addr,
							MYDEVICE_TYPE_CANIOT);
	if (dev == NULL) {
		LOG_ERR("Failed to register device, list full %hhu / %lu",
			devices.count,
			ARRAY_SIZE(devices.list));
		ret = -ENOMEM;
		goto exit;
	}

	/* device does exist now, update it measurements */
	dev->data.measurements_timestamp = timestamp;

	if (CANIOT_DT_VALID_T10_TEMP(data->int_temperature)) {
		dev->data.caniot.temperatures[0].type = MYDEVICE_SENSOR_TYPE_EMBEDDED;
		dev->data.caniot.temperatures[0].value = 
			caniot_dt_T10_to_T16(data->int_temperature);
	} else {
		dev->data.caniot.temperatures[0].type = MYDEVICE_SENSOR_TYPE_NONE;
	}

	if (CANIOT_DT_VALID_T10_TEMP(data->ext_temperature)) {
		dev->data.caniot.temperatures[1].type = MYDEVICE_SENSOR_TYPE_EXTERNAL;
		dev->data.caniot.temperatures[1].value = 
			caniot_dt_T10_to_T16(data->ext_temperature);
	} else {
		dev->data.caniot.temperatures[1].type = MYDEVICE_SENSOR_TYPE_NONE;
	}

	/* todo board IO */

	LOG_INF("Registered CANIOT record for device 0x%hhx", did.val);

exit:
	k_mutex_unlock(&devices.mutex);
	
	return ret;
}

/*___________________________________________________________________________*/


K_MSGQ_DEFINE(ipc_ble_msgq, sizeof(ipc_frame_t), 1U, 4U);
struct k_poll_event ipc_event =
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&ipc_ble_msgq, 0);
static struct k_work_poll ipc_ble_work;

static void ipc_work_handler(struct k_work *work)
{
	int ret;

	ipc_frame_t frame;

	/* process all available messages */
	while (k_msgq_get(&ipc_ble_msgq, &frame, K_NO_WAIT) == 0U) {
		ret = mydevices_register_ble_xiaomi_dataframe(
			(xiaomi_dataframe_t *)frame.data.buf);
		if (ret != 0) {
			LOG_ERR("Failed to handle BLE Xiaomi record, err: %d",
				ret);
		}
	}	

	ipc_event.state = K_POLL_STATE_NOT_READY;

	/* re-register for next event */
	ret = k_work_poll_submit(&ipc_ble_work, &ipc_event, 1U, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to resubmit work %p to poll queue: %d", work, ret);
	}
}


int mydevice_init(void)
{
	int ret = net_time_wait_synced(K_FOREVER);
	if (ret == 0) {
		ipc_attach_rx_msgq(&ipc_ble_msgq);

		k_work_poll_init(&ipc_ble_work, ipc_work_handler);
		k_work_poll_submit(&ipc_ble_work, &ipc_event, 1U, K_FOREVER);

	} else {
		LOG_ERR("Time not synced ! ret = %d", ret);
	}

	return ret;
}
