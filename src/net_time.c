#include "net_time.h"

#include <net/sntp.h>
#include <posix/time.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(net_time, LOG_LEVEL_INF);

void sntp_handler(struct k_work *work);

K_WORK_DEFINE(sntp_work, sntp_handler);

void sntp_handler(struct k_work *work)
{
        int ret;
        struct sntp_time time;
        struct timespec tspec;
        const char *server = "0.fr.pool.ntp.org";

        /* https://www.pool.ntp.org/zone/fr */
	ret = sntp_simple(server, 10U * MSEC_PER_SEC, &time);
        if (ret != 0) {
                /* todo retry later */
                LOG_ERR("sntp_simple() failed with error = %d", ret);
                return;
        }

        LOG_INF("SNTP time from %s:123 = %llu", server, time.seconds);

        tspec.tv_sec = time.seconds;
        tspec.tv_nsec = ((uint64_t)time.fraction * (1000LU * 1000LU * 1000LU)) >> 32;

        clock_settime(CLOCK_REALTIME, &tspec);
}

void net_time_sync(void)
{
        k_work_submit(&sntp_work);
}

uint32_t net_time_get(void)
{
	struct timespec ts;

        if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
                LOG_ERR("Invalid net time ! %d", -1);
                return 0;
        }

	return ts.tv_sec;
}

void net_time_show(void)
{
	time_t timestamp = net_time_get();
	
        struct tm time_infos;

        gmtime_r(&timestamp, &time_infos);

        printk("Local (Europe/Paris) Date and time : %04d/%02d/%02d %02d:%02d:%02d\n",
               time_infos.tm_year + 1900, time_infos.tm_mon + 1, time_infos.tm_mday,
               time_infos.tm_hour, time_infos.tm_min, time_infos.tm_sec);
}