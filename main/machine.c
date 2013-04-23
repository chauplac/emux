#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <clock.h>
#include <controller.h>
#include <cpu.h>
#include <input.h>
#include <machine.h>
#include <memory.h>
#include <util.h>

#define NS(s) ((s) * 1000000000)

static void machine_input_event(int id, struct input_state *state,
	input_data_t *data);

#ifdef _WIN32
extern struct machine _machines_begin, _machines_end;
static struct machine *machines_begin = &_machines_begin;
static struct machine *machines_end = &_machines_end;
#else
extern struct machine __machines_begin, __machines_end;
static struct machine *machines_begin = &__machines_begin;
static struct machine *machines_end = &__machines_end;
#endif
struct machine *machine;

bool machine_init(char *name)
{
	struct machine *m;
	for (m = machines_begin; m < machines_end; m++)
		if (!strcmp(name, m->name))
			machine = m;

	/* Exit if machine has not been found */
	if (!machine) {
		fprintf(stderr, "Machine \"%s\" not recognized!\n", name);
		return false;
	}

	/* Display machine name and description */
	fprintf(stdout, "Machine: %s (%s)\n", machine->name,
		machine->description);

	if (machine->init)
		return machine->init();
	return true;
}

void machine_input_event(int UNUSED(id), struct input_state *UNUSED(state),
	input_data_t *UNUSED(data))
{
	machine->running = false;
}

void machine_run()
{
	uint32_t counter = 0;
	struct timeval start_time;
	struct timeval current_time;
	unsigned int mach_delay;
	unsigned int real_delay;
	struct input_config input_config;
	struct input_event quit_event;

	/* Return if no clock has been registered */
	if (machine->clock_rate == 0) {
		fprintf(stderr, "No clock registered for this machine!\n");
		return;
	}

	fprintf(stdout, "Machine clock rate: %uHz\n", machine->clock_rate);

	/* Compute machine delay between two ticks (in ns) */
	mach_delay = NS(1) / machine->clock_rate;

	/* Initialize start time */
	gettimeofday(&start_time, NULL);

	/* Set running flag and register for quit events */
	machine->running = true;
	quit_event.type = EVENT_QUIT;
	input_config.events = &quit_event;
	input_config.num_events = 1;
	input_config.callback = machine_input_event;
	input_register(&input_config);

	/* Run until user quits */
	while (machine->running) {
		/* Update input sub-system */
		input_update();

		/* Tick all registered clocks */
		clock_tick_all(counter++);

		/* Get actual delay (in ns) */
		gettimeofday(&current_time, NULL);
		real_delay = NS(current_time.tv_sec - start_time.tv_sec) +
			(current_time.tv_usec - start_time.tv_usec) * 1000;

		/* Sleep to match machine delay */
		if (counter * mach_delay > real_delay)
			usleep((counter * mach_delay - real_delay) / 1000);

		/* Reset counter and start time if needed */
		if (counter == machine->clock_rate) {
			gettimeofday(&start_time, NULL);
			counter = 0;
		}
	}

	/* Unregister quit events */
	input_unregister(&input_config);
}

void machine_deinit()
{
	clock_remove_all();
	cpu_remove_all();
	controller_remove_all();
	memory_region_remove_all();
	if (machine->deinit)
		machine->deinit();
}

