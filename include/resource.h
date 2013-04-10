#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <stdint.h>

enum resource_type {
	RESOURCE_MEM
};

struct resource {
	char *name;
	uint16_t start;
	uint16_t end;
	enum resource_type type;
};

struct resource *resource_get(char *name, struct resource *resources,
	int num_resources);

#endif

