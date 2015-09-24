/* adapted from VisualBoyAdvance, which had the following notice.*/
/* VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
 * Copyright (C) 1999-2003 Forgotten
 * Copyright (C) 2004 Forgotten and the VBA development team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or(at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* adapted from gmon.c */
/*-
 * Copyright (c) 1991, 1998 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. [rescinded 22 July 1999]
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <android/log.h>    /* for __android_log_print, ANDROID_LOG_INFO, etc */
#include <errno.h>          /* for errno */
#include <signal.h>         /* for sigaction, etc */
#include <stdint.h>         /* for uint32_t, uint16_t, etc */
#include <stdio.h>          /* for FILE */
#include <stdlib.h>         /* for getenv */
#include <sys/time.h>       /* for setitimer, etc */

#include "gmon.h"
#include "gmon_out.h"
#include "prof.h"
#include "read_maps.h"

#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, "PROFILING", __VA_ARGS__)

#define DEFAULT_GMON_OUT "/sdcard/gmon.out"

typedef struct {
	unsigned short *froms;
	struct tostruct *tos;
	long tolimit;
} callgraph_t;

typedef struct {
	size_t low_pc;
	size_t high_pc;
	size_t text_size;
} process_t;

typedef struct {
	uint32_t nb_bins;
	char *bins;
} histogram_t;

static histogram_t hist;
static process_t process;
static callgraph_t cg;
static struct proc_map *s_maps = NULL;
int opt_is_shared_lib = 0;

static void systemMessage(int a, const char *msg)
{
	LOGI("%d: %s", a, msg);
}

static void profPut32(char *b, uint32_t v)
{
	b[0] = v & 255;
	b[1] = (v >> 8) & 255;
	b[2] = (v >> 16) & 255;
	b[3] = (v >> 24) & 255;
}

static void profPut16(char *b, uint16_t v)
{
	b[0] = v & 255;
	b[1] = (v >> 8) & 255;
}

static int profWrite8(FILE *f, uint8_t b)
{
	if (fwrite(&b, 1, 1, f) != 1) {
		return 1;
	}
	return 0;
}

static int profWrite32(FILE *f, uint32_t v)
{
	char buf[4];
	profPut32(buf, v);
	if (fwrite(buf, 1, 4, f) != 4) {
		return 1;
	}
	return 0;
}

static int profWrite(FILE *f, char *buf, unsigned int n)
{
	if (fwrite(buf, 1, n, f) != n) {
		return 1;
	}
	return 0;
}

static int get_max_samples_per_sec()
{
	struct itimerval timer;

	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 1;
	timer.it_value = timer.it_interval;
	setitimer(ITIMER_PROF, &timer, 0);
	setitimer(ITIMER_PROF, 0, &timer);
	return 1000000 / timer.it_interval.tv_usec;
}

static void histogram_bin_incr(int sig, siginfo_t *info, void *context)
{
	ucontext_t *ucontext = (ucontext_t *) context;
	struct sigcontext *mcontext = &ucontext->uc_mcontext;
	uint32_t frompcindex = mcontext->arm_pc;

	uint16_t *b = (uint16_t *) hist.bins;

	/* the pc should be divided by HISTFRACTION, but we do */
	/* a right shift with 1 because HISTFRACTION=2 */
	size_t pc = (frompcindex - process.low_pc) >> 1;

	if (pc < hist.nb_bins) {
		b[pc]++;
	}
}

static void add_profile_handler(int sample_freq)
{
	struct sigaction action;
	/* request info, sigaction called instead of sighandler */
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	action.sa_sigaction = histogram_bin_incr;
	sigemptyset(&action.sa_mask);
	int result = sigaction(SIGPROF, &action, NULL);
	if (result != 0) {
		/* panic */
		LOGI("add_profile_handler, sigaction failed %d %d", result,
		     errno);
		return;
	}

	struct itimerval timer;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 1000000 / sample_freq;
	timer.it_value = timer.it_interval;
	setitimer(ITIMER_PROF, &timer, 0);
}

static long remove_profile_handler(void)
{
	struct itimerval timer;
	struct itimerval oldtimer;

	memset(&timer, 0, sizeof(timer));
	setitimer(ITIMER_PROF, &timer, &oldtimer);

	return oldtimer.it_value.tv_usec;
}

static int select_frequency()
{
	int max_samples = get_max_samples_per_sec();
	char *freq = getenv("CPUPROFILE_FREQUENCY");

	if (!freq) {
		LOGI("using sample frequency: %d", max_samples);
		return max_samples;
	}

	int freqval = strtol(freq, 0, 0);

	if (freqval <= 0) {
		LOGI("Invalid frequency value: %d, using default: %d",
		     freqval, max_samples);
		return max_samples;
	}

	LOGI("Maximum number of samples per second: %d", max_samples);
	LOGI("Specified frequency: %d", freqval);

	if (freqval > max_samples) {
		LOGI("Specified sample rate is too large, using %d",
		     max_samples);
		return max_samples;
	}

	return freqval;
}

static void process_init()
{
	process.low_pc = s_maps->lo;
	process.high_pc = s_maps->hi;
	/*
	 * round lowpc and highpc to multiples of the density we're using
	 * so the rest of the scaling (here and in gprof) stays in ints.
	 */
	process.low_pc = ROUNDDOWN(process.low_pc, HISTFRACTION * sizeof(HISTCOUNTER));
	process.high_pc = ROUNDUP(process.high_pc, HISTFRACTION * sizeof(HISTCOUNTER));
	process.text_size = process.high_pc - process.low_pc;
}

#define MSG ("No space for profiling buffer(s)\n")

static int histogram_init()
{
	hist.nb_bins = (process.text_size / HISTFRACTION);

	/* FIXME: check if '2' is the size of short or if it has another meaning */
	hist.bins = calloc(1, sizeof(short) * hist.nb_bins);
	if (!hist.bins) {
		systemMessage(0, MSG);
		return 0;
	}
	return 1;
}

static int cg_init()
{
	/* FIXME: what should be the size of 'froms' */
	/* froms = calloc(1, 4 * process.text_size / HASHFRACTION); */
	cg.froms = calloc(1, sizeof(short) * hist.nb_bins);

	if (cg.froms == NULL) {
		systemMessage(0, MSG);
		free(hist.bins);
		return 0;
	}

	cg.tolimit = process.text_size * ARCDENSITY / 100;

	if (cg.tolimit < MINARCS) {
		cg.tolimit = MINARCS;
	} else if (cg.tolimit > 65534) {
		cg.tolimit = 65534;
	}

	cg.tos = (struct tostruct *) calloc(1, cg.tolimit * sizeof(struct tostruct));
	if (cg.tos == NULL) {
		systemMessage(0, MSG);
		free(hist.bins);
		free(cg.froms);
		cg.froms = NULL;
		return 0;
	}
	cg.tos[0].link = 0;

	return 1;
}

__attribute__((visibility("default")))
void monstartup(const char *libname)
{
	FILE *self = fopen("/proc/self/maps", "r");

	if (!self) {
		systemMessage(1, "Cannot open memory maps file");
		return;
	}

	if (strstr(libname, ".so")) {
		LOGI("start profiling shared library %s", libname);
		opt_is_shared_lib = 1;
	} else {
		LOGI("start profiling executable %s", libname);
		opt_is_shared_lib = 0;
	}

	s_maps = read_maps(self, libname);

	if (s_maps == NULL) {
		systemMessage(0, "No maps found");
		return;
	}

	process_init();

	if (!histogram_init()) {
		return;
	}

	LOGI("Profile %s, pc: 0x%x-0x%x, base: 0x%d", libname,
	     process.low_pc, process.high_pc, s_maps->base);

	if (!cg_init()) {
		return;
	}

	int sample_freq = select_frequency();
	add_profile_handler(sample_freq);
}

static const char *get_gmon_out(void)
{
	char *gmon_out = getenv("CPUPROFILE");
	if (gmon_out && strlen(gmon_out))
		return gmon_out;
	return DEFAULT_GMON_OUT;
}

__attribute__((visibility("default")))
void moncleanup(void)
{
	FILE *fd;
	int fromindex;
	int endfrom;
	uint32_t frompc;
	int toindex;
	struct gmon_hdr ghdr;
	const char *gmon_name = get_gmon_out();

	long ival = remove_profile_handler();
	int sample_freq = 1000000 / ival;

	LOGI("Sampling frequency: %d", sample_freq);
	LOGI("moncleanup, writing output to %s", gmon_name);

	fd = fopen(gmon_name, "wb");
	if (fd == NULL) {
		systemMessage(0, "mcount: gmon.out");
		return;
	}
	memcpy(&ghdr.cookie[0], GMON_MAGIC, 4);
	profPut32((char *) ghdr.version, GMON_VERSION);
	if (fwrite(&ghdr, sizeof(ghdr), 1, fd) != 1) {
		systemMessage(0, "mcount: gmon.out header");
		fclose(fd);
		return;
	}

	if (profWrite8(fd, GMON_TAG_TIME_HIST)
	    || profWrite32(fd, get_real_address(s_maps, (uint32_t) process.low_pc))
	    || profWrite32(fd, get_real_address(s_maps, (uint32_t) process.high_pc))
	    || profWrite32(fd, hist.nb_bins)
	    || profWrite32(fd, sample_freq)
	    || profWrite(fd, "seconds", 15)
	    || profWrite(fd, "s", 1)
	   ) {
		systemMessage(0, "ERROR writing mcount: gmon.out hist");
		fclose(fd);
		return;
	}
	uint16_t *hist_sample = (uint16_t *) hist.bins;
	uint16_t count;
	int i;
	for (i = 0; i < hist.nb_bins; ++i) {
		profPut16((char *) &count, hist_sample[i]);
		/* LOGI("bin: %d, value: %d", i, hist_sample[i]); */
		if (fwrite(&count, sizeof(count), 1, fd) != 1) {
			systemMessage(0, "ERROR writing file mcount: gmon.out sample");
			fclose(fd);
			return;
		}
	}
	endfrom = process.text_size / (HASHFRACTION * sizeof(*cg.froms));
	for (fromindex = 0; fromindex < endfrom; fromindex++) {
		if (cg.froms[fromindex] == 0) {
			continue;
		}
		frompc =
			process.low_pc + (fromindex * HASHFRACTION * sizeof(*cg.froms));
		frompc = get_real_address(s_maps, frompc);
		for (toindex = cg.froms[fromindex]; toindex != 0;
		     toindex = cg.tos[toindex].link) {
			if (profWrite8(fd, GMON_TAG_CG_ARC)
			    || profWrite32(fd, (uint32_t) frompc)
			    || profWrite32(fd,
					   get_real_address(s_maps,
							    (uint32_t)
							    cg.tos[toindex].
							    selfpc))
			    || profWrite32(fd, cg.tos[toindex].count)) {
				systemMessage(0, "ERROR writing mcount: arc");
				fclose(fd);
				return;
			}
		}
	}
	fclose(fd);
}

void profCount(size_t *frompcindex, char *selfpc)
{
	struct tostruct *top;
	struct tostruct *prevtop;
	size_t toindex;
	/*
	 * find the return address for mcount,
	 * and the return address for mcount's caller.
	 */
	/* selfpc = pc pushed by mcount call.
	   This identifies the function that was just entered.  */
	/*selfpc = (char *) reg[14].I; */
	/* frompcindex = pc in preceding frame.
	   This identifies the caller of the function just entered.  */
	/*frompcindex = (unsigned short *) reg[12].I; */
	/*
	 * check that we are profiling
	 * and that we aren't recursively invoked.
	 */

	/*
	 * check that frompcindex is a reasonable pc value.
	 * for example: signal catchers get called from the stack,
	 *   not from text space.  too bad.
	 */

	/* frompcindex = (size_t *) ( (size_t) frompcindex - process.low_pc); */

	size_t frompc_val = (size_t) frompcindex - process.low_pc;
	size_t *frompc_ptr = (size_t *) frompc_val;

	if (frompc_val > process.text_size) {
		return;
	}

	frompc_ptr = (size_t *) &cg.froms[frompc_val / (HASHFRACTION * sizeof(*cg.froms))];
	toindex = *frompc_ptr;

	if (toindex == 0) {
		/*
		 * first time traversing this arc
		 */
		toindex = ++cg.tos[0].link;
		if (toindex >= cg.tolimit) {
			goto overflow;
		}
		*frompc_ptr = (size_t) toindex;
		top = &cg.tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		return;
	}
	top = &cg.tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 * arc at front of chain; usual case.
		 */
		top->count++;
		return;
	}

	/*
	 * have to go looking down chain for it.
	 * top points to what we are looking at,
	 * prevtop points to previous top.
	 * we know it is not at the head of the chain.
	 */
	for (; /* goto done */ ;) {
		if (top->link == 0) {
			/*
			 * top is end of the chain and none of the chain
			 * had top->selfpc == selfpc.
			 * so we allocate a new tostruct
			 * and link it to the head of the chain.
			 */
			toindex = ++cg.tos[0].link;
			if (toindex >= cg.tolimit) {
				goto overflow;
			}
			top = &cg.tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompc_ptr;
			*frompc_ptr = (size_t) toindex;
			return;
		}
		/*
		 * otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &cg.tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 * there it is.
			 * increment its count
			 * move it to the head of the chain.
			 */
			top->count++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompc_ptr;
			*frompc_ptr = (size_t) toindex;
			return;
		}
	}

out:
	return;			/* normal return restores saved registers */
overflow:
	systemMessage(0, "mcount: tos overflow\n");
	goto out;

}
