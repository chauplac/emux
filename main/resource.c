#include <string.h>
#include <resource.h>

struct resource *resource_get(char *name, struct resource *resources,
	int num_resources)
{
	int i;
	for (i = 0; i < num_resources; i++)
		if (!strcmp(name, resources[i].name))
			return &resources[i];
	return NULL;
}

