#ifndef _WIN32
#include <signal.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmdline.h>
#include <debugger.h>
#include <machine.h>
#include <util.h>

#define MAX_LINE_SIZE 256

struct debugger_command {
	char *short_name;
	char *long_name;
	bool (*f)();
};

struct breakpoint {
	int id;
	enum debugger_event_type type;
	uint16_t address;
};

static void debugger_prompt();
static bool debugger_scan();
#ifndef _WIN32
static void debugger_sig_int(int sig);
#endif
static void debugger_insert_bp(enum debugger_event_type type, uint16_t address);
static bool debugger_cmd_help();
static bool debugger_cmd_run();
static bool debugger_cmd_continue();
static bool debugger_cmd_kill();
static bool debugger_cmd_quit();
static bool debugger_cmd_break();
static bool debugger_cmd_watch();
static bool debugger_cmd_list();
static bool debugger_cmd_delete();

extern struct machine *machine;

static struct debugger_command debugger_commands[] = {
	{ "h", "help", debugger_cmd_help },
	{ "r", "run", debugger_cmd_run },
	{ "c", "continue", debugger_cmd_continue },
	{ "k", "kill", debugger_cmd_kill },
	{ "q", "quit", debugger_cmd_quit },
	{ "b", "break", debugger_cmd_break },
	{ "w", "watch", debugger_cmd_watch },
	{ "l", "list", debugger_cmd_list },
	{ "d", "delete", debugger_cmd_delete }
};

static bool enabled;
static bool active;
static bool prompt;
static struct list_link *events;
static struct list_link *breakpoints;

static char buffer[MAX_LINE_SIZE];

bool debugger_init()
{
	return cmdline_parse_bool("debugger", &enabled);
}

void debugger_run()
{
	for (;;) {
		/* Prompt first */
		debugger_prompt();

		/* Quit if debugger has been disabled */
		if (!enabled)
			break;

		/* Initialize machine and quit in case of failure */
		if (!machine_init()) {
			fprintf(stdout, "Error initializing machine!\n");
			break;
		}

		/* We now have an active running session */
		active = true;
		prompt = false;
		machine_run();

		/* At this point, machine is not running anymore */
		machine_deinit();

		/* Program exited naturally if session is still active */
		if (active) {
			fprintf(stdout, "Machine stopped sucessfully.\n");
			active = false;
		}

		/* Quit if debugger is disabled */
		if (!enabled)
			break;
	}
}

void debugger_update()
{
	struct list_link *ev_link = events;
	struct list_link *bp_link;
	struct debugger_event *event;
	struct breakpoint *breakpoint;

	if (!enabled)
		return;

	/* Parse all events and request prompt if needed */
	while ((event = list_get_next(&ev_link))) {
		bp_link = breakpoints;
		while ((breakpoint = list_get_next(&bp_link))) {
			/* Check if event matches a breakpoint */
			if ((breakpoint->type != event->type) ||
				(breakpoint->address != event->address))
				continue;

			/* Indicate we hit a breakpoint */
			prompt = true;
			fprintf(stdout, "Breakpoint/watchpoint %u hit.\n",
				breakpoint->id);
		}
		free(event);
		list_remove(&events, event);
	}

	if (prompt) {
		prompt = false;
		debugger_prompt();
	}
}

void debugger_report(struct debugger_event *event)
{
	struct debugger_event *e = malloc(sizeof(struct debugger_event));
	memcpy(e, event, sizeof(struct debugger_event));
	list_insert(&events, e);
}

void debugger_deinit()
{
	struct list_link *link;
	struct breakpoint *breakpoint;
	struct debugger_event *event;

	link = breakpoints;
	while ((breakpoint = list_get_next(&link)))
		free(breakpoint);

	link = events;
	while ((event = list_get_next(&link)))
		free(event);

	list_remove_all(&breakpoints);
	list_remove_all(&events);
}

void debugger_prompt()
{
	char *str;
	unsigned int i;
	struct debugger_command *cmd;
#ifndef _WIN32
	struct sigaction action;
#endif

#ifndef _WIN32
	/* Ignore Ctrl-C */
	action.sa_handler = SIG_IGN;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGINT, &action, NULL);
#endif

	/* Loop until a command releases prompt */
	for (;;) {
		/* Print prompt */
		fprintf(stdout, "\r(edb) ");
		fflush(stdout);

		/* Scan user command */
		if (!debugger_scan())
			continue;

		/* Parse command */
		if (!(str = strtok(buffer, " ")))
			continue;

		/* Find command in our array */
		cmd = NULL;
		for (i = 0; i < ARRAY_SIZE(debugger_commands); i++)
			if (!strcmp(str, debugger_commands[i].short_name) ||
				!strcmp(str, debugger_commands[i].long_name))
					cmd = &debugger_commands[i];

		/* Warn if command was not found */
		if (!cmd) {
			fprintf(stdout,
			"Undefined command \"%s\" - type \"help\" if needed.\n",
			str);
			continue;
		}

		/* Run command and exit prompt if needed */
		if (cmd->f())
			break;
	}

#ifndef _WIN32
	/* Restore Ctrl-C */
	action.sa_handler = enabled ? debugger_sig_int : SIG_DFL;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGINT, &action, NULL);
#endif
}

bool debugger_scan()
{
	char c;

	/* Read from standard input */
	if (!fgets(buffer, MAX_LINE_SIZE, stdin))
		return false;

	/* Flush standard input in case buffer is too long */
	if (buffer[strlen(buffer) - 1] != '\n')
		while (((c = getchar()) != '\n') && (c != EOF));

	/* Terminate string properly */
	buffer[strlen(buffer) - 1] = '\0';

	return true;
}

#ifndef _WIN32
void debugger_sig_int(int UNUSED(sig))
{
	fprintf(stdout, "\nReceived interrupt.\n");
	prompt = true;
}
#endif

void debugger_insert_bp(enum debugger_event_type type, uint16_t address)
{
	struct breakpoint *breakpoint;
	struct breakpoint *bp;
	struct list_link *link = breakpoints;

	/* Allocate breakpoint */
	breakpoint = malloc(sizeof(struct breakpoint));
	breakpoint->type = type;
	breakpoint->address = address;

	/* Set breakpoint ID */
	breakpoint->id = 0;
	while ((bp = list_get_next(&link)))
		breakpoint->id = bp->id + 1;

	/* Finally insert breakpoint in our list */
	list_insert(&breakpoints, breakpoint);

	/* Indicate successful insertion */
	fprintf(stdout, "%s %u at address 0x%04x.\n",
		breakpoint->type == EVENT_EXECUTE ? "Breakpoint" : "Watchpoint",
		breakpoint->id, breakpoint->address);
}

bool debugger_cmd_help()
{
	fprintf(stdout, "List of commands:\n");
	fprintf(stdout, "  help               Display this help\n");
	fprintf(stdout, "  run                Start machine execution\n");
	fprintf(stdout, "  continue           Continue machine execution\n");
	fprintf(stdout, "  kill               Stop machine execution\n");
	fprintf(stdout, "  quit               Quit debugger and Emux\n");
	fprintf(stdout, "  break <address>    Add breakpoint\n");
	fprintf(stdout, "  watch <address>    Add watchpoint\n");
	fprintf(stdout, "  list               List batchpoints/watchpoints\n");
	fprintf(stdout, "  delete <id>        Delete breakpoint/watchpoint\n");
	return false;
}

bool debugger_cmd_run()
{
	/* Make sure machine is not running already */
	if (active) {
		fprintf(stdout, "Machine is already running.\n");
		return false;
	}

	return true;
}

bool debugger_cmd_continue()
{
	/* Make sure machine is running first */
	if (!active) {
		fprintf(stdout, "Machine is not running.\n");
		return false;
	}

	return true;
}

bool debugger_cmd_kill()
{
	/* Make sure machine is running first */
	if (!active) {
		fprintf(stdout, "Machine is not running.\n");
		return false;
	}

	/* Stop machine */
	machine->running = false;

	/* Flag that session was killed by debugger */
	active = false;

	return true;
}

bool debugger_cmd_quit()
{
	/* Stop machine if session is active */
	if (active)
		machine->running = false;

	/* Disable debugger and allow it to quit */
	enabled = false;

	/* Flag that session was killed by debugger */
	active = false;

	return true;
}

bool debugger_cmd_break()
{
	char *str;
	unsigned int address;

	/* Scan address */
	if (!(str = strtok(NULL, " "))) {
		fprintf(stdout, "Please append address (hex).\n");
		return false;
	}

	/* Convert string to integer */
	if (sscanf(str, "%x", &address) != 1) {
		fprintf(stdout, "Please append a correct address (hex).\n");
		return false;
	}

	/* Insert new breakpoint */
	debugger_insert_bp(EVENT_EXECUTE, address);

	return false;
}

bool debugger_cmd_watch()
{
	char *str;
	unsigned int address;

	/* Scan address */
	if (!(str = strtok(NULL, " "))) {
		fprintf(stdout, "Please append address (hex).\n");
		return false;
	}

	/* Convert string to integer */
	if (sscanf(str, "%x", &address) != 1) {
		fprintf(stdout, "Please append a correct address (hex).\n");
		return false;
	}

	/* Insert new breakpoint */
	debugger_insert_bp(EVENT_MEM_ACCESS, address);

	return false;
}

bool debugger_cmd_list()
{
	struct list_link *link = breakpoints;
	struct breakpoint *bp;

	/* List all available breakpoints (in order) */
	while ((bp = list_get_next(&link)))
		fprintf(stdout, "%s %u at address 0x%04x.\n",
			bp->type == EVENT_EXECUTE ? "Breakpoint" : "Watchpoint",
			bp->id, bp->address);

	return false;
}

bool debugger_cmd_delete()
{
	char *str;
	int id;
	struct list_link *link = breakpoints;
	struct breakpoint *bp;

	/* Scan ID */
	if (!(str = strtok(NULL, " "))) {
		fprintf(stdout, "Please append ID.\n");
		return false;
	}

	/* Convert string to integer */
	if (sscanf(str, "%u", &id) != 1) {
		fprintf(stdout, "Please append a correct ID.\n");
		return false;
	}

	/* Find and delete breakpoint */
	while ((bp = list_get_next(&link)))
		if (bp->id == id) {
			free(bp);
			list_remove(&breakpoints, bp);
			return false;
		}

	/* Breakpoint was not found */
	fprintf(stdout, "No breakpoint/watchpoint number %u.\n", id);
	return false;
}

