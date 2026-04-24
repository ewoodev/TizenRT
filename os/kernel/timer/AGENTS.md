# `os/kernel/timer` Module Guide

## Purpose

`os/kernel/timer` implements the POSIX timer APIs declared in `os/include/time.h` by wrapping kernel watchdog timers with per-task ownership and signal delivery state.

## Public APIs

- `timer_create()`: allocate a timer object and copy notification settings into it
- `timer_delete()`: release a timer handle and cancel the underlying watchdog
- `timer_settime()`: translate a timeout request into watchdog ticks and arm or disarm the timer
- `timer_gettime()`: report the remaining watchdog delay and the module's recorded last delay
- `timer_getoverrun()`: public stub that currently fails with `ENOSYS`

Function-level notes live beside the implementation sources:

- `timer_create.md`
- `timer_delete.md`
- `timer_settime.md`
- `timer_gettime.md`
- `timer_getoverrun.md`

## Build and Kconfig

- The entire folder is excluded when `CONFIG_DISABLE_POSIX_TIMERS=y`.
- `CONFIG_PREALLOC_TIMERS` sizes the preallocated timer-object pool used before falling back to heap allocation.
- `CONFIG_SCHED_WAKEUPSOURCE` enables the `TIMER_WAKEUPSOURCE` flag path in `timer_settime()`.
- `CONFIG_USEC_PER_TICK` affects the tick resolution used by the clock conversion helpers that arm these timers.
- Kconfig ownership is in `os/kernel/Kconfig`, while the object list is gated in `os/kernel/timer/Make.defs`.

## Internal Model

- Each public `timer_t` is an internal `struct posix_timer_s`.
- The structure stores the owner PID, signal number, signal payload, watchdog handle, periodic delay, and a reference count.
- `timer_initialize()` seeds the free list for preallocated timers and initializes the allocated timer list.
- `timer_deleteall()` walks the allocated timers and deletes any timer owned by an exiting task.
- `timer_release()` is the shared cleanup path used by deletion and timeout completion.

## Data Flow

- `timer_create()` allocates the watchdog and timer structure, sets default notification state, and returns a disarmed handle.
- `timer_settime()` cancels any existing watchdog, records periodic state, converts the requested timeout to ticks, and arms the watchdog when the final delay is positive.
- The watchdog callback queues the configured signal to the owner task, then either restarts the watchdog for periodic timers or releases the timer reference.
- `timer_gettime()` snapshots the watchdog delay and converts it back to `struct timespec`.

## Maintenance Notes

- Only `CLOCK_REALTIME` is accepted by `timer_create()`.
- `timer_settime()` ignores `ovalue`, so the API contract is narrower than the usual POSIX interface.
- `timer_gettime()` returns `pt_last` through `it_interval`; this is the last watchdog delay used by the module, not a strict copy of the caller's last `it_interval`.
- `timer_getoverrun()` is still unimplemented even though the declaration is public.
