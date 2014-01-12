#include <stdio.h>
#include <stdbool.h>
#include <audio.h>
#include <log.h>
#include <util.h>

static bool null_init(struct audio_specs *specs);
static void null_start();
static void null_stop();
static void null_deinit();

bool null_init(struct audio_specs *UNUSED(specs))
{
	return true;
}

void null_start()
{
}

void null_stop()
{
}

void null_deinit()
{
}

AUDIO_START(null)
	.init = null_init,
	.start = null_start,
	.stop = null_stop,
	.deinit = null_deinit
AUDIO_END

