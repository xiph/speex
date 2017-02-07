/*
 * Part of the android-ndk-profiler library.
 * Copyright (C) Richard Quirk
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef read_maps_h_seen
#define read_maps_h_seen

struct proc_map {
	unsigned int base;
	unsigned int lo;
	unsigned int hi;
	struct proc_map *next;
};

struct proc_map *read_maps(FILE *fp, const char *lname);
void free_maps(struct proc_map *s);
unsigned int get_real_address(const struct proc_map *maps, unsigned int fake);

#endif
