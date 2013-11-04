#ifndef _MACHINE_H
#define _MACHINE_H

#include <stdbool.h>

#define MACHINE_SECTION_NAME	"machines"
#ifdef __APPLE__
#define MACHINE_SEGMENT_NAME	"__DATA"
#define MACHINE_SECTION_LABEL	MACHINE_SEGMENT_NAME ", " MACHINE_SECTION_NAME
#else
#define MACHINE_SECTION_LABEL	MACHINE_SECTION_NAME
#endif

#define MACHINE_START(_name, _description) \
	static struct machine machine_##_name \
		__attribute__(( \
			__used__, \
			__section__(MACHINE_SECTION_LABEL), \
			__aligned__(__alignof__(struct machine)))) = { \
		.name = #_name, \
		.description = _description,
#define MACHINE_END \
	};

typedef void machine_priv_data_t;

struct machine {
	char *name;
	char *description;
	machine_priv_data_t *priv_data;
	bool running;
	bool (*init)(struct machine *machine);
	void (*deinit)(struct machine *machine);
};

bool machine_init();
void machine_run();
void machine_deinit();

#endif

