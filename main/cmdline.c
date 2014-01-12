#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audio.h>
#include <cmdline.h>
#include <config.h>
#include <list.h>
#include <machine.h>
#include <video.h>

struct cmdline {
	int argc;
	char **argv;
};

static int param_sort_compare(const void *a, const void *b);
int param_bsearch_compare(const void *key, const void *elem);
static bool cmdline_parse_arg(char *long_name, bool has_arg, char **arg);
static bool cmdline_parse_bool(char *long_name, bool *arg);
static bool cmdline_parse_int(char *long_name, int *arg);
static bool cmdline_parse_string(char *long_name, char **string);

struct param **params;
static int num_params;
static struct cmdline cmdline;
static char *path;

int param_sort_compare(const void *a, const void *b)
{
	struct param *p1 = *(struct param **)a;
	struct param *p2 = *(struct param **)b;
	int result;

	/* Non-options come last */
	if (!p1->name)
		return 1;
	else if (!p2->name)
		return -1;

	/* Global options have a higher priority */
	if (!p1->module && p2->module)
		return -1;
	else if (p1->module && !p2->module)
		return 1;
	else if (!p1->module && !p2->module)
		return strcmp(p1->name, p2->name);

	/* Sort by module name */
	result = strcmp(p1->module, p2->module);
	if (result != 0)
		return result;

	/* Sort by parameter name */
	return strcmp(p1->name, p2->name);
}

int param_bsearch_compare(const void *key, const void *elem)
{
	char *module = (char *)key;
	struct param *p = *(struct param **)elem;

	/* Checked for global options first */
	if (!p->module)
		return 1;
	
	/* Check for module name next */
	return strcmp(module, p->module);
}

void cmdline_register_param(struct param *param)
{
	/* Grow params array, insert param, and sort array */
	params = realloc(params, ++num_params *	sizeof(struct param *));
	params[num_params - 1] = param;
	qsort(params, num_params, sizeof(struct param *), param_sort_compare);
}

void cmdline_unregister_param(struct param *param)
{
	int index;

	/* Find param to remove */
	for (index = 0; index < num_params; index++)
		if (params[index] == param)
			break;

	/* Return if param was not found */
	if (index == num_params)
		return;

	/* Shift remaining regions */
	while (index < num_params - 1) {
		params[index] = params[index + 1];
		index++;
	}

	/* Shrink params array */
	params = realloc(params, --num_params * sizeof(struct param *));
}

void cmdline_init(int argc, char *argv[])
{
	struct param *p;
	int i;

	/* Save command line */
	cmdline.argc = argc;
	cmdline.argv = argv;

	/* Print version and command line first */
	fprintf(stdout, "Emux version %s\n", PACKAGE_VERSION);
	fprintf(stdout, "Command line:");
	for (i = 0; i < argc; i++)
		fprintf(stdout, " %s", argv[i]);
	fprintf(stdout, "\n");

	/* Parse and fill parameters */
	for (i = 0; i < num_params; i++) {
		p = params[i];
		if (!strcmp(p->type, "bool"))
			cmdline_parse_bool(p->name, p->address);
		else if (!strcmp(p->type, "int"))
			cmdline_parse_int(p->name, p->address);
		else if (!strcmp(p->type, "string"))
			cmdline_parse_string(p->name, p->address);
	}
}

void cmdline_print_usage(bool error)
{
	FILE *stream = error ? stderr : stdout;
	struct list_link *link;
	struct param *p;
	struct machine *m;
	struct audio_frontend *af;
	struct video_frontend *vf;
	char str[20];
	int i;

	fprintf(stream, "Usage: emux [OPTION]... path\n");
	fprintf(stream, "Emulates various machines (consoles, arcades).\n");

	/* Don't print full usage in case of error */
	if (error) {
		fprintf(stream, "Try `emux --help' for more information.\n");
		return;
	}

	/* Print general options */
	fprintf(stream, "\n");
	fprintf(stream, "Emux options:\n");
	for (i = 0; i < num_params; i++) {
		/* Break when module-specific options are reached */
		p = params[i];
		if (p->module)
			break;

		/* Print argument name and type (not applicable to booleans) */
		if (!strcmp(p->type, "bool"))
			snprintf(str, 20, "%s", p->name);
		else
			snprintf(str, 20, "%s=%s", p->name, p->type);
		fprintf(stream, "  --%-20s", str);

		/* Print description */
		fprintf(stream, "%s\n", p->desc);
	}
	fprintf(stream, "\n");

	/* Print supported machines */
	link = machines;
	fprintf(stream, "Supported machines:\n");
	while ((m = list_get_next(&link)))
		fprintf(stream, "  %s (%s)\n", m->name, m->description);
	fprintf(stream, "\n");

	/* Print audio frontends */
	link = audio_frontends;
	fprintf(stream, "Audio frontends:\n");
	while ((af = list_get_next(&link)))
		fprintf(stream, "  %s\n", af->name);
	fprintf(stream, "\n");

	/* Print video frontends */
	link = video_frontends;
	fprintf(stream, "Video frontends:\n");
	while ((vf = list_get_next(&link)))
		fprintf(stream, "  %s\n", vf->name);
	fprintf(stream, "\n");

	/* Display project related info */
	fprintf(stream, "Report bugs to: sronsse@gmail.com\n");
	fprintf(stream, "Project page: <http://emux.googlecode.com>\n");
}

void cmdline_print_module_options(char *module)
{
	struct param *p;
	int i;

	/* Check if module has options */
	if (!bsearch(module,
		params,
		num_params,
		sizeof(struct param *),
		param_bsearch_compare))
		return;

	/* Print module options */
	fprintf(stderr, "\n");
	fprintf(stderr, "Valid %s options:\n", module);
	for (i = 0; i < num_params; i++) {
		p = params[i];
		if (p->module && !strcmp(p->module, module))
			fprintf(stderr, "  --%s (%s)\n", p->name, p->desc);
	}
	fprintf(stderr, "\n");
}

char *cmdline_get_path()
{
	/* Return previously stored command-line file/dir path */
	return path;
}

bool cmdline_parse_arg(char *long_name, bool has_arg, char **arg)
{
	int has_argument = has_arg ? required_argument : no_argument;
	struct option long_options[] = {
		{ long_name, has_argument, NULL, 'o' },
		{ 0, 0, 0, 0 }
	};
	int c;

	/* Reset option index and make sure errors don't get printed out */
	optind = 1;
	opterr = 0;

	/* Parse option until we either find requested one or none is left */
	do {
		c = getopt_long_only(cmdline.argc,
			cmdline.argv,
			"",
			long_options,
			NULL);

		/* Check if option is found */
		if (c == 'o') {
			if (has_arg)
				*arg = optarg;
			return true;
		}
	} while (c != -1);

	/* Check if a non-option is found if needed */
	if (!long_name && (optind < cmdline.argc)) {
		if (has_arg)
			*arg = cmdline.argv[optind];
		return true;
	}

	return false;
}

bool cmdline_parse_bool(char *long_name, bool *arg)
{
	bool b;

	b = cmdline_parse_arg(long_name, false, NULL);

	if (arg)
		*arg = b;
	return b;
}

bool cmdline_parse_int(char *long_name, int *arg)
{
	char *str;
	char *end;
	int i;

	if (!cmdline_parse_arg(long_name, true, &str))
		return false;

	i = strtol(str, &end, 10);
	if (*end)
		return false;

	if (arg)
		*arg = i;
	return true;
}

bool cmdline_parse_string(char *long_name, char **arg)
{
	char *str;

	if (!cmdline_parse_arg(long_name, true, &str))
		return false;

	if (arg)
		*arg = str;
	return true;
}

