/*
 * Copyright 2014 Fastly, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Librip is a minimal-overhead API for instruction-level tracing in highly
 * concurrent software.
 */
#ifndef LIBRIP_H_
#define LIBRIP_H_

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if __STDC_LIB_EXT1__ >= 201112L
#include <threads.h>
#else
#define thread_local __thread
#endif

struct rip_ring {
        char            name[64];
        uintptr_t       rip[32];
};

extern thread_local pid_t rip_tid;
extern thread_local uint8_t rip_idx;
extern thread_local uint8_t rip_ctr;

extern struct rip_ring *rip_ring;  

static inline bool
rip_create_ring(const char *file, uint32_t max_procs)
{
	uint64_t max_tid;
	char buf[32];
	int fd;

	if (max_procs == 0) {
		fd = open("/proc/sys/kernel/pid_max", O_RDONLY);
		if (fd < 0) {
			return false;
		}
		memset(buf, 0, sizeof (buf));

		if (read(fd, &buf, sizeof (buf)) <= 0) {
			return false;
		}
		close(fd);

		errno = 0;
		max_tid = strtoul(buf, NULL, 10);
		if (errno != 0) {
			return false;
		}
	} else {
		max_tid = max_procs;
	}

        fd = open(file, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		return false;
	}

	if (ftruncate(fd, max_tid * sizeof (*rip_ring)) != 0) {
		close(fd);
		return false;
	}

        rip_ring = mmap(NULL, max_tid * sizeof (*rip_ring), PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
	close(fd);

	if (rip_ring == MAP_FAILED) {
		return false;
	}

	return true;
}

static inline void
__attribute__((always_inline))
rip_init(pid_t id, const char *name, size_t namelen, uint8_t idx, uint8_t ctr)
{

	if (id == 0) {
		id = syscall(SYS_gettid);
	}

	memset(rip_ring + id, 0, sizeof (*rip_ring));
	
	if (name) {
		size_t len = (namelen < sizeof (rip_ring[id].name) - 1) ? namelen : sizeof (rip_ring[id].name) - 1;
		memcpy(rip_ring[id].name, name, len);
		rip_ring[id].name[len] = '\0';
	}

	rip_tid = id;
	rip_idx = idx;
	rip_ctr = ctr;
}

static inline void
__attribute__((always_inline))
rip_exit(void)
{
	uintptr_t addr = 0;

	rip_ring[rip_tid].rip[rip_idx++ & 31] = addr - 1;
}

static inline uintptr_t
__attribute__((always_inline))
rip_snap(void)
{
	uintptr_t rip;

	__asm__ __volatile__("leaq 0(%%rip), %0" : "=q" (rip));
	rip_ring[rip_tid].rip[rip_idx++ & 31] = ((uintptr_t)(rip_ctr++) << 56) | rip;

	return rip;
}

#endif
