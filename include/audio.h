#ifndef _AUDIO_H
#define _AUDIO_H

#include <stdbool.h>
#include <stdint.h>
#include <list.h>

#define AUDIO_START(_name) \
	static struct audio_frontend _audio_frontend = { \
		.name = #_name,
#define AUDIO_END \
	}; \
	__attribute__((constructor)) static void _register() \
	{ \
		list_insert(&audio_frontends, &_audio_frontend); \
	} \
	__attribute__((destructor)) static void _unregister() \
	{ \
		list_remove(&audio_frontends, &_audio_frontend); \
	}

typedef void audio_data_t;

enum audio_format {
	AUDIO_FORMAT_U8,
	AUDIO_FORMAT_S8,
	AUDIO_FORMAT_U16,
	AUDIO_FORMAT_S16
};

struct audio_specs {
	int freq;
	enum audio_format format;
	int channels;
	int samples;
	void (*mix)(audio_data_t *data, void *buffer, int len);
	audio_data_t *data;
};

struct audio_frontend {
	char *name;
	bool (*init)(struct audio_specs *specs);
	void (*start)();
	void (*stop)();
	void (*deinit)();
};

bool audio_init(struct audio_specs *specs);
void audio_start();
void audio_stop();
void audio_deinit();

extern struct list_link *audio_frontends;

#endif

