#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <audio.h>
#include <cmdline.h>
#include <config.h>
#include <debugger.h>
#include <machine.h>
#include <video.h>

static void print_usage(bool error);

#if defined(_WIN32)
extern struct machine _machines_begin, _machines_end;
extern struct audio_frontend _audio_frontends_begin, _audio_frontends_end;
extern struct video_frontend _video_frontends_begin, _video_frontends_end;
static struct machine *machines_begin = &_machines_begin;
static struct machine *machines_end = &_machines_end;
static struct audio_frontend *audio_frontends_begin = &_audio_frontends_begin;
static struct audio_frontend *audio_frontends_end = &_audio_frontends_end;
static struct video_frontend *video_frontends_begin = &_video_frontends_begin;
static struct video_frontend *video_frontends_end = &_video_frontends_end;
#elif defined(__APPLE__)
extern struct machine *machines_begin;
extern struct machine *machines_end;
extern struct audio_frontend *audio_frontends_begin;
extern struct audio_frontend *audio_frontends_end;
extern struct video_frontend *video_frontends_begin;
extern struct video_frontend *video_frontends_end;
#else
extern struct machine __machines_begin, __machines_end;
extern struct audio_frontend __audio_frontends_begin, __audio_frontends_end;
extern struct video_frontend __video_frontends_begin, __video_frontends_end;
static struct machine *machines_begin = &__machines_begin;
static struct machine *machines_end = &__machines_end;
static struct audio_frontend *audio_frontends_begin = &__audio_frontends_begin;
static struct audio_frontend *audio_frontends_end = &__audio_frontends_end;
static struct video_frontend *video_frontends_begin = &__video_frontends_begin;
static struct video_frontend *video_frontends_end = &__video_frontends_end;
#endif

void print_usage(bool error)
{
	FILE *stream = error ? stderr : stdout;
	struct machine *m;
	struct audio_frontend *af;
	struct video_frontend *vf;

	fprintf(stream, "Usage: emux [OPTION]...\n");
	fprintf(stream, "Emulates various machines (consoles, arcades).\n");

	/* Don't print full usage in case of error */
	if (error) {
		fprintf(stream, "Try `emux --help' for more information.\n");
		return;
	}

	/* Print options */
	fprintf(stream, "\n");
	fprintf(stream, "Emux options:\n");
	fprintf(stream, "  --machine=MACH    Select machine to emulate\n");
	fprintf(stream, "  --audio=AUDIO     Select audio frontend\n");
	fprintf(stream, "  --video=VIDEO     Select video frontend\n");
	fprintf(stream, "  --width=WIDTH     Override window width\n");
	fprintf(stream, "  --height=HEIGHT   Override window height\n");
	fprintf(stream, "  --debugger        Enable debugger support\n");
	fprintf(stream, "  --help            Display this help and exit\n");
	fprintf(stream, "\n");

	/* Print supported machines */
	fprintf(stream, "Supported machines:\n");
	for (m = machines_begin; m < machines_end; m++)
		fprintf(stream, "  %s (%s)\n", m->name, m->description);
	fprintf(stream, "\n");

	/* Print audio frontends */
	fprintf(stream, "Audio frontends:\n");
	for (af = audio_frontends_begin; af < audio_frontends_end; af++)
		fprintf(stream, "  %s\n", af->name);
	fprintf(stream, "\n");

	/* Print video frontends */
	fprintf(stream, "Video frontends:\n");
	for (vf = video_frontends_begin; vf < video_frontends_end; vf++)
		fprintf(stream, "  %s\n", vf->name);
	fprintf(stream, "\n");

	/* Display project related info */
	fprintf(stream, "Report bugs to: sronsse@gmail.com\n");
	fprintf(stream, "Project page: <http://emux.googlecode.com>\n");
}

int main(int argc, char *argv[])
{
	int i;

	/* Initialize random seed */
	srand(time(NULL));

	/* Print version and command line */
	fprintf(stdout, "Emux version %s\n", PACKAGE_VERSION);
	fprintf(stdout, "Command line:");
	for (i = 0; i < argc; i++)
		fprintf(stdout, " %s", argv[i]);
	fprintf(stdout, "\n");

	/* Initialize command line and parse it */
	cmdline_init(argc, argv);

	/* Check if user requires help */
	if (cmdline_parse_bool("help", NULL)) {
		print_usage(false);
		return 0;
	}

	/* Initialize debugger */
	if (debugger_init()) {
		debugger_run();
		debugger_deinit();
		return 0;
	}

	/* Initialize, run, and deinit machine */
	if (!machine_init()) {
		print_usage(true);
		return 1;
	}
	machine_run();
	machine_deinit();

	return 0;
}

