/*
 * Part of the android-ndk-profiler library.
 * Copyright (C) Richard Quirk
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "read_maps.h"

#ifdef ANDROID
#include <android/log.h>    /* for __android_log_print, ANDROID_LOG_INFO, etc */
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, "PROFILING", __VA_ARGS__)
#else
#define LOGI(...)  do { printf(__VA_ARGS__) ; printf("\n"); } while (0)
#endif

static char s_line[256];
extern int opt_is_shared_lib;

void free_maps(struct proc_map *s)
{
	struct proc_map *next = s->next;
	while (next != NULL) {
		struct proc_map *tmp = next;
		next = next->next;
		free(tmp);
	}
	free(s);
}

struct proc_map *read_maps(FILE *fp, const char *lname)
{
	struct proc_map *results = NULL;
	struct proc_map *current = NULL;
	size_t namelen = strlen(lname);

	while (fgets(s_line, sizeof(s_line), fp) != NULL) {
		size_t len = strlen(s_line);
		len--;
		s_line[len] = 0;

		if (namelen < len
		    && strcmp(lname, &s_line[len - namelen]) == 0) {
			char c[1];
			char perm[4];
			int lo, base, hi;
			sscanf(s_line, "%x-%x %4c %x %c", &lo, &hi, perm, &base, c);

			if (results == NULL) {
				current = malloc(sizeof(struct proc_map));
				if (!current) {
					LOGI("error allocating memory");
					return NULL;
				}
				current->next = NULL;
				results = current;
			} else {
				current->next = malloc(sizeof(struct proc_map));
				current = current->next;
				if (!current) {
					LOGI("error allocating memory");
					return NULL;
				}
				current->next = NULL;
			}

			LOGI("process '%s', base = 0x%x, lo = 0x%x, hi = 0x%x", lname, base, lo, hi);

			current->base = base;
			current->lo = lo;
			current->hi = hi;
		}
	}
	return results;
}

unsigned int get_real_address(const struct proc_map *maps,
			      unsigned int fake)
{
	const struct proc_map *mp = maps;
	while (mp) {
		if (fake >= mp->lo && fake <= mp->hi) {
			if (opt_is_shared_lib) {
				return fake - mp->lo;
			}
			return fake;
		}
		mp = mp->next;
	}
	return fake;
}
