#include "routes.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(routes, LOG_LEVEL_WRN);

#include "rest_server.h"
#include "web_server.h"

#define REST REST_RESSOURCE
#define WEB WEB_RESSOURCE

#define DELETE HTTP_DELETE
#define GET HTTP_GET
#define POST HTTP_POST
#define PUT HTTP_PUT

static const struct http_route routes[] = {
	REST(GET, "", rest_info),
	REST(GET, "/", rest_info),
	REST(GET, "/info", rest_info),
	REST(GET, "/records/xiaomi", rest_xiaomi_records),
	REST(GET, "/records/xiaomi/prometheus", rest_xiaomi_records_promethus),

	WEB(GET, "/metrics", NULL) /* prometheus_metrics */
};

static inline const struct http_route *first(void)
{
	return routes;
}

static inline const struct http_route *last(void)
{
	return &routes[ARRAY_SIZE(routes) - 1];
}

static bool url_match(const struct http_route *res,
		      const char *url, size_t url_len)
{
	size_t check_len = MAX(res->route_len, url_len);

	return strncmp(res->route, url, check_len) == 0;
}

const struct http_route *route_resolve(struct http_request *req)
{
	for (const struct http_route *route = first(); route <= last(); route++) {
		if (route->method != req->method) {
			continue;
		}

		if (url_match(route, req->url, req->url_len)) {

			return route;
		}
	}

	return NULL;
}

http_content_type_t http_get_route_default_content_type(const struct http_route *route)
{
	switch (route->server) {
	case HTTP_REST_SERVER:
		return HTTP_CONTENT_TYPE_APPLICATION_JSON;

	case HTTP_WEB_SERVER:
	default:
		return HTTP_CONTENT_TYPE_TEXT_PLAIN;
	}
}