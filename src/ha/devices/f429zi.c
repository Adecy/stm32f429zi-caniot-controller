#include <zephyr/kernel.h>

#include <stdint.h>
#include <stdbool.h>

#include "f429zi.h"


static void f429zi_temp_convert(struct ha_ds_f429zi *dt,
				const float *die_temp)
{
	dt->die_temperature.value = *die_temp * 100u;
}

static int ingest(struct ha_event *ev,
	   struct ha_dev_payload *pl)
{

	f429zi_temp_convert(ev->data, (const float *) pl->buffer);

	return 0;
}

static const struct ha_data_descr ha_ds_f429zi_descr[] = {
	HA_DATA_DESCR(struct ha_ds_f429zi, die_temperature, HA_DATA_TEMPERATURE, HA_ASSIGN_SOC_TEMPERATURE),
};

static struct ha_device_endpoint_api ep = {
	.eid = HA_DEV_ENDPOINT_NUCLEO_F429ZI,
	.data_size = sizeof(struct ha_ds_f429zi),
	.expected_payload_size = sizeof(float),
	.data_descr_size = ARRAY_SIZE(ha_ds_f429zi_descr),
	.data_descr = ha_ds_f429zi_descr,
	.ingest = ingest,
	.command = NULL
};

static int init_endpoints(const ha_dev_addr_t *addr,
			  struct ha_device_endpoint *endpoints,
			  uint8_t *endpoints_count)
{
	endpoints[0].api = &ep;
	*endpoints_count = 1U;

	return 0;
}

const struct ha_device_api ha_device_api_f429zi = {
	.init_endpoints = init_endpoints,
	.select_endpoint = HA_DEV_API_SELECT_ENDPOINT_0_CB
};

int ha_dev_register_die_temperature(uint32_t timestamp, float die_temperature)
{
	const ha_dev_addr_t addr = {
		.type = HA_DEV_TYPE_NUCLEO_F429ZI,
		.mac = {
			.medium = HA_DEV_MEDIUM_NONE,
		}
	};

	return ha_dev_register_data(&addr, &die_temperature,
				    sizeof(die_temperature), timestamp, NULL);
}

float ha_ev_get_die_temperature(const ha_ev_t *ev)
{
	const struct ha_ds_f429zi *dt =
		HA_EV_GET_CAST_DATA(ev, struct ha_ds_f429zi);

	return dt->die_temperature.value / 100.0f;
}