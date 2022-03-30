#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>

#include <net/net_ip.h>
#include <drivers/can.h>

#include <sys/types.h>

ssize_t mem_append(void *dst,
		   size_t dst_len,
		   const void *src,
		   size_t src_len);

ssize_t mem_append_string(void *dst,
			  size_t dst_len,
			  const char *string);

ssize_t mem_append_strings(void *dst,
			   size_t dst_len,
			   const char **strings,
			   size_t count);

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len);

int strcicmp(char const *a, char const *b);

int strncicmp(char const *a, char const *b, size_t len);

int get_repr_can_frame(struct zcan_frame *frame, char *buf, size_t len);

#endif /* _UTILS_H_ */