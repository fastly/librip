# librip

Librip is a minimal-overhead API for instruction-level tracing in highly
concurrent software. It is released under the Apache 2.0 license.

## Impetus

Software with many thousands of threads and / or coroutines suffers from lack
of run-time and post-mortem visibility with debugging tools. When many
thousands of threads are present in a software system, it may be impossible to
attach a debugger to an apparently stuck process. With many thousands of
coroutines, even with a debugger, it can be difficult to even find a stuck
coroutine on myriad scheduling lists -- especially if it is erroneously
waiting for data in the kernel!

This library attempts to solve these problems through an API that is
reasonably efficient in terms of both CPU and memory requirements -- enough so
that traces can be take in production.

## Design

As the name suggests, this library provides an interface for snapshots of the
instruction pointer. These snapshots are stored in a per-thread ring buffer,
and contain a packed counter / function address. (Additional interfaces allow
registration for coroutines, but require additional runtime support.)

Currently, only Linux on amd64 architectures is supported. Patches for other
platforms, operating systems, and compilers are more than welcome.

## API

### Global Variables

There are a few global variables that need defined storage:

```c
extern thread_local pid_t rip_tid;
extern thread_local uint8_t rip_idx;
extern thread_local uint8_t rip_ctr;

extern struct rip_ring *rip_ring;
```

All of these are used internally in the API, but require storage defined in the
consumer's software. The consumer should not need to worry about these.

### rip\_create\_ring

```
bool rip_create_ring(const char *file, uint32_t max_procs);
```

Create a new shared memory segment mapped from the specified file, and
supporting at least `max\_procs` concurrent processes. When `max\_procs` is
zero, the API uses the platform's maximum process id as its limiter.

For this software to work as intended, `file` must reference a file present
on a Linux `tmpfs`.

### rip\_init

```
void rip_init(uint32_t id, const char *name, size_t namelen, uint8_t idx, uint8_t ctr)
```

Initialize the counters for a new concurrent process identified by `id`. When
these counters are non-zero, they can be used to re-initialize counters for
coroutines.

The `name` parameter serves as a friendly identifier for the traced process.

### rip\_exit
```
void rip_exit(void)
```

Mark the concurrent process `id` as exited.

### rip\_snap
```
void rip_snap(void)
```

Take a new snapshot and insert it into the appropriate trace buffer.
