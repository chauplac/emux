#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <controller.h>
#include <machine.h>

#ifdef __APPLE__
# include <mach-o/getsect.h>
void cx_controllers(void) __attribute__((__constructor__));
#endif

#if defined(_WIN32)
extern struct controller _controllers_begin, _controllers_end;
static struct controller *controllers_begin = &_controllers_begin;
static struct controller *controllers_end = &_controllers_end;
#elif defined(__APPLE__)
static struct controller *controllers_begin;
static struct controller *controllers_end;
void cx_controllers(void)
{
# ifdef __LP64__
	const struct section_64 *sect = getsectbyname(CONTROLLER_SEGMENT_NAME,
						      CONTROLLER_SECTION_NAME);
# else
	const struct section *sect = getsectbyname(CONTROLLER_SEGMENT_NAME,
						   CONTROLLER_SECTION_NAME);
# endif
	if (sect) {
		controllers_begin = (struct controller *)(sect->addr);
		controllers_end   = (struct controller *)(sect->addr +
							  sect->size);
	}
}
#else
extern struct controller __controllers_begin, __controllers_end;
static struct controller *controllers_begin = &__controllers_begin;
static struct controller *controllers_end = &__controllers_end;
#endif
extern struct machine *machine;

void controller_add(struct controller_instance *instance)
{
	struct controller *c;
	for (c = controllers_begin; c < controllers_end; c++)
		if (!strcmp(instance->controller_name, c->name)) {
			instance->controller = c;
			if ((c->init && c->init(instance)) || !c->init)
				list_insert(&machine->controller_instances,
					instance);
			return;
		}

	/* Warn as controller was not found */
	fprintf(stderr, "Controller \"%s\" not recognized!\n",
		instance->controller_name);
}

struct resource *controller_get_resource(char *name,
	struct controller_instance *instance)
{
	return resource_get(name, instance->resources, instance->num_resources);
}

void controller_remove_all()
{
	struct list_link *link = machine->controller_instances;
	struct controller_instance *instance;

	while ((instance = list_get_next(&link)))
		if (instance->controller->deinit)
			instance->controller->deinit(instance);

	list_remove_all(&machine->controller_instances);
}

