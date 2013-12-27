#ifndef _GB_MAPPER_H
#define _GB_MAPPER_H

#include <stdint.h>

#define CART_HEADER_START	0x0100
#define NINTENDO_LOGO_SIZE	48
#define MANUFACTURER_CODE_SIZE	4
#define NEW_LICENSEE_CODE_SIZE	2
#define TITLE_SIZE		11

struct gb_mapper_mach_data {
	char *path;
};

struct cart_header {
	uint32_t entry_point;
	uint8_t nintendo_logo[NINTENDO_LOGO_SIZE];
	char title[TITLE_SIZE];
	char manufacturer_code[MANUFACTURER_CODE_SIZE];
	uint8_t cgb_flag;
	char new_licensee_code[NEW_LICENSEE_CODE_SIZE];
	uint8_t sgb_flag;
	uint8_t cartridge_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t dest_code;
	uint8_t old_licensee_code;
	uint8_t rom_version;
	uint8_t header_checksum;
	uint16_t global_checksum;
};

#endif

