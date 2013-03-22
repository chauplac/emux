#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <clock.h>
#include <machine.h>

static uint32_t gcd(uint32_t a, uint32_t b);
static uint32_t lcm(uint32_t a, uint32_t b);
static uint32_t lcmm(struct clock_link *clocks);

extern struct machine *machine;

uint32_t gcd(uint32_t a, uint32_t b)
{
	uint32_t t;
	while (b) {
		t = b;
		b = a % b;
		a = t;
	}
	return a;
}

uint32_t lcm(uint32_t a, uint32_t b)
{
	return a * b / gcd(a, b);
}

uint32_t lcmm(struct clock_link *clocks)
{
	if (!clocks->next->next)
		return lcm(clocks->clock->rate, clocks->next->clock->rate);
	return lcm(clocks->clock->rate, lcmm(clocks->next));
}

void clock_add(struct clock *clock)
{
	struct clock_link *link;
	struct clock_link *tail;

	/* Create new link */
	link = malloc(sizeof(struct clock_link));
	link->clock = clock;
	link->next = NULL;

	/* Insert link */
	if (!machine->clocks) {
		machine->clocks = link;
	} else {
		tail = machine->clocks;
		while (tail->next)
			tail = tail->next;
		tail->next = link;
	}

	/* Update machine rate */
	link = machine->clocks;
	machine->clock_rate = link->next ? lcmm(link) : clock->rate;

	/* Update clock dividers */
	while (link) {
		link->clock->div = machine->clock_rate / link->clock->rate;
		link = link->next;
	}
}

void clock_tick_all(uint32_t cycle)
{
	struct clock_link *link = machine->clocks;
	while (link) {
		if (cycle % link->clock->div == 0)
			link->clock->tick(link->clock->data);
		link = link->next;
	}
}

void clock_remove_all()
{
	struct clock_link *link;
	while (machine->clocks) {
		link = machine->clocks;
		machine->clocks = machine->clocks->next;
		free(link);
	}
}

