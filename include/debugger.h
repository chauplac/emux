#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <stdbool.h>
#include <stdint.h>

enum debugger_event_type {
	EVENT_EXECUTE,
	EVENT_MEM_ACCESS
};

struct debugger_event {
	enum debugger_event_type type;
	uint16_t address;
};

bool debugger_init();
void debugger_run();
void debugger_update();
void debugger_report(struct debugger_event *event);
void debugger_deinit();

#endif

