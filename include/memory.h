#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>
#include <resource.h>

#define KB(x) (x * 1024)

typedef void region_data_t;

struct mops {
	uint8_t (*readb)(region_data_t *data, uint16_t address);
	uint16_t (*readw)(region_data_t *data, uint16_t address);
	void (*writeb)(region_data_t *data, uint8_t b, uint16_t address);
	void (*writew)(region_data_t *data, uint16_t w, uint16_t address);
};

struct region {
	struct resource *area;
	struct resource *mirrors;
	int num_mirrors;
	struct mops *mops;
	region_data_t *data;
};

struct region_link {
	struct region *region;
	struct region_link *next;
};

void memory_add_region(struct region *region);
void memory_region_remove_all();
uint8_t memory_readb(uint16_t address);
uint16_t memory_readw(uint16_t address);
void memory_writeb(uint8_t b, uint16_t address);
void memory_writew(uint16_t w, uint16_t address);
void *memory_map_file(char *path, int offset, int size);
void memory_unmap_file(void *data, int size);

#endif

