#include <zephyr.h>

#include "data.h"

#include "devices.h"

size_t get_data_size(ha_data_type_t type)
{
	switch (type) {
	case HA_DATA_TEMPERATURE:
		return sizeof(struct ha_data_temperature);
	case HA_DATA_HUMIDITY:
		return sizeof(struct ha_data_humidity);
	case HA_DATA_BATTERY_LEVEL:
		return sizeof(struct ha_data_battery_level);
	case HA_DATA_RSSI:
		return sizeof(struct ha_data_rssi);
	case HA_DATA_DIGITAL:
		return sizeof(struct ha_data_digital);
	case HA_DATA_ANALOG:
		return sizeof(struct ha_data_analog);
	case HA_DATA_HEATER_MODE:
		return sizeof(struct ha_heater_mode);
	case HA_DATA_SHUTTER_POSITION:
		return sizeof(struct ha_shutter_position);
	default:
		return 0;
	}
}

void *ha_data_get(void *data,
		  const struct ha_data_descr *descr,
		  size_t descr_size,
		  ha_data_type_t type,
		  uint8_t index)
{
	if (!data || !descr || !descr_size)
		return NULL;

	const struct ha_data_descr *d;

	for (d = descr; d < descr + descr_size; d++) {
		if ((d->type == type) && (index-- == 0)) {
			return (uint8_t *)data + d->offset;
			break;
		}
	}

	return NULL;
}