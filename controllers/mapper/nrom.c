#include <stdlib.h>
#include <controller.h>
#include <memory.h>
#include <controllers/mapper/nes_mapper.h>

struct nrom {
	bool vertical_mirroring;
	struct region vram_region;
	struct region prg_rom_region;
	uint8_t *vram;
	uint8_t *prg_rom;
	int prg_rom_size;
};

static bool nrom_init(struct controller_instance *instance);
static void nrom_deinit(struct controller_instance *instance);
static uint8_t vram_readb(region_data_t *data, uint16_t address);
static uint16_t vram_readw(region_data_t *data, uint16_t address);
static void vram_writeb(region_data_t *data, uint8_t b, uint16_t address);
static void vram_writew(region_data_t *data, uint16_t w, uint16_t address);
static void mirror_address(struct nrom *nrom, uint16_t *address);
static uint8_t prg_rom_readb(region_data_t *data, uint16_t address);
static uint16_t prg_rom_readw(region_data_t *data, uint16_t address);

static struct mops vram_mops = {
	.readb = vram_readb,
	.readw = vram_readw,
	.writeb = vram_writeb,
	.writew = vram_writew
};

static struct mops prg_rom_mops = {
	.readb = prg_rom_readb,
	.readw = prg_rom_readw
};

uint8_t vram_readb(region_data_t *data, uint16_t address)
{
	struct nrom *nrom = data;
	mirror_address(nrom, &address);
	return ram_mops.readb(nrom->vram, address);
}

uint16_t vram_readw(region_data_t *data, uint16_t address)
{
	struct nrom *nrom = data;
	mirror_address(nrom, &address);
	return ram_mops.readw(nrom->vram, address);
}

void vram_writeb(region_data_t *data, uint8_t b, uint16_t address)
{
	struct nrom *nrom = data;
	mirror_address(nrom, &address);
	ram_mops.writeb(nrom->vram, b, address);
}

void vram_writew(region_data_t *data, uint16_t w, uint16_t address)
{
	struct nrom *nrom = data;
	mirror_address(nrom, &address);
	ram_mops.writew(nrom->vram, w, address);
}

void mirror_address(struct nrom *nrom, uint16_t *address)
{
	bool bit;

	/* The NES hardware lets the cart control some VRAM lines as follows:
	Vertical mirroring: $2000 equals $2800 and $2400 equals $2C00
	Horizontal mirroring: $2000 equals $2400 and $2800 equals $2C00 */

	/* Adapt address in function of selected mirroring */
	if (nrom->vertical_mirroring) {
		/* Clear bit 11 of address */
		*address &= ~(1 << 11);
	} else {
		/* Set bit 10 of address to bit 11 and clear bit 11 */
		bit = *address & (1 << 11);
		*address &= ~(1 << 10);
		*address |= (bit << 10);
		*address &= ~(bit << 11);
	}
}

uint8_t prg_rom_readb(region_data_t *data, uint16_t address)
{
	struct nrom *nrom = data;

	/* Handle NROM-128 mirroring */
	address %= nrom->prg_rom_size;

	return *(nrom->prg_rom + address);
}

uint16_t prg_rom_readw(region_data_t *data, uint16_t address)
{
	struct nrom *nrom = data;
	uint8_t *mem;

	/* Handle NROM-128 mirroring */
	address %= nrom->prg_rom_size;

	mem = nrom->prg_rom + address;
	return (*(mem + 1) << 8) | *mem;
}

bool nrom_init(struct controller_instance *instance)
{
	struct nrom *nrom;
	struct nes_mapper_mach_data *mach_data = instance->mach_data;
	struct cart_header *cart_header;
	struct region *region;

	/* Allocate NROM structure */
	instance->priv_data = malloc(sizeof(struct nrom));
	nrom = instance->priv_data;

	/* Map cart header */
	cart_header = memory_map_file(mach_data->path, 0,
		sizeof(struct cart_header));

	/* Get mirroring information (used for VRAM access) - NROM supports
	only horizontal and vertical mirroring */
	nrom->vertical_mirroring = (cart_header->flags6 & 0x09);

	/* Save VRAM (specified by machine) */
	nrom->vram = mach_data->vram;

	/* Add VRAM region */
	region = &nrom->vram_region;
	region->area = resource_get("vram",
		RESOURCE_MEM,
		instance->resources,
		instance->num_resources);
	region->mops = &vram_mops;
	region->data = nrom;
	memory_region_add(region);

	/* Allocate and fill region data */
	nrom->prg_rom_size = PRG_ROM_SIZE(cart_header);
	nrom->prg_rom = memory_map_file(mach_data->path,
		PRG_ROM_OFFSET(cart_header),
		nrom->prg_rom_size);

	/* Fill and add PRG ROM region */
	region = &nrom->prg_rom_region;
	region->area = resource_get("prg_rom",
		RESOURCE_MEM,
		instance->resources,
		instance->num_resources);
	region->mops = &prg_rom_mops;
	region->data = nrom;
	memory_region_add(region);

	/* Unmap cart header */
	memory_unmap_file(cart_header, sizeof(struct cart_header));

	return true;
}

void nrom_deinit(struct controller_instance *instance)
{
	struct nrom *nrom = instance->priv_data;
	memory_unmap_file(nrom->prg_rom, nrom->prg_rom_size);
	free(nrom);
}

CONTROLLER_START(nrom)
	.init = nrom_init,
	.deinit = nrom_deinit
CONTROLLER_END

