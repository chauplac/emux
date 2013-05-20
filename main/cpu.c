#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpu.h>
#include <machine.h>

#ifdef __APPLE__
# include <mach-o/getsect.h>
void cx_cpus(void) __attribute__((__constructor__));
#endif

#if defined(_WIN32)
extern struct cpu _cpus_begin, _cpus_end;
static struct cpu *cpus_begin = &_cpus_begin;
static struct cpu *cpus_end = &_cpus_end;
#elif defined(__APPLE__)
static struct cpu *cpus_begin;
static struct cpu *cpus_end;
void cx_cpus(void)
{
# ifdef __LP64__
	const struct section_64 *sect = getsectbyname(CPU_SEGMENT_NAME,
						      CPU_SECTION_NAME);
# else
	const struct section *sect = getsectbyname(CPU_SEGMENT_NAME,
						   CPU_SECTION_NAME);
# endif
	if (sect) {
		cpus_begin = (struct cpu *)(sect->addr);
		cpus_end   = (struct cpu *)(sect->addr + sect->size);
	}
}
#else
extern struct cpu __cpus_begin, __cpus_end;
static struct cpu *cpus_begin = &__cpus_begin;
static struct cpu *cpus_end = &__cpus_end;
#endif
extern struct machine *machine;

void cpu_add(struct cpu_instance *instance)
{
	struct cpu *cpu;
	for (cpu = cpus_begin; cpu < cpus_end; cpu++)
		if (!strcmp(instance->cpu_name, cpu->name)) {
			instance->cpu = cpu;
			if ((cpu->init && cpu->init(instance)) || !cpu->init)
				list_insert(&machine->cpu_instances, instance);
			return;
		}

	/* Warn as CPU was not found */
	fprintf(stderr, "CPU \"%s\" not recognized!\n", instance->cpu_name);
}

void cpu_remove_all()
{
	struct list_link *link = machine->cpu_instances;
	struct cpu_instance *instance;

	while ((instance = list_get_next(&link)))
		if (instance->cpu->deinit)
			instance->cpu->deinit(instance);

	list_remove_all(&machine->cpu_instances);
}

