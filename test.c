#include <unistd.h>

#include "librip.h"

thread_local pid_t rip_tid;
thread_local uint8_t rip_idx;
thread_local uint8_t rip_ctr;

struct rip_ring *rip_ring;

int
main(void)
{
	int i = 255;

	if (rip_create_ring("foo", 2) == false) {
		return -1;
	}

	rip_init(1, "lol", 3, 0, 0);

	while (i--) {
		rip_snap();
	}

	rip_exit();

	return 0;
}
