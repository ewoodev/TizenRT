# `os/kernel/clock` Module Guide

## Purpose

`os/kernel/clock` provides the OS-managed time base used by realtime and monotonic clock APIs, plus the lightweight wrappers that expose that state through the public headers.

## Public APIs

- `clock_settime()`: update the realtime base offset used for wall-clock reads
- `clock_gettime()`: read realtime or optional monotonic time
- `clock_getres()`: report the configured realtime resolution
- `clock()`: return the current system timer count in clock ticks
- `gettimeofday()`: return realtime in `timeval` form

Function-level notes live beside the implementation sources:

- `clock_settime.md`
- `clock_gettime.md`
- `clock_getres.md`
- `clock.md`
- `gettimeofday.md`

## Build and Configuration

- `os/kernel/clock/Make.defs` always builds this folder in the current tree.
- `CONFIG_CLOCK_MONOTONIC` enables the `CLOCK_MONOTONIC` branch in `clock_gettime()`.
- `CONFIG_SCHED_TICKLESS`, `CONFIG_SYSTEM_TIME64`, and `CONFIG_USEC_PER_TICK` change how `clock_systimer()` and `clock_systimespec()` derive elapsed time.
- `CONFIG_RTC`, `CONFIG_RTC_DATETIME`, and `CONFIG_RTC_HIRES` change the source used for initial basetime and elapsed-time calculations.
- `CONFIG_INIT_SYSTEM_TIME` seeds RTC state from the build time during initialization.

## Internal Model

- `g_basetime` stores the realtime offset added to the current `clock_systimespec()` bias when callers request `CLOCK_REALTIME`.
- `clock_systimer()` returns the current system timer count in ticks from either `g_system_timer` or platform timer helpers.
- `clock_systimespec()` converts elapsed uptime into `timespec`, with a special hi-res RTC path that subtracts `g_basetime` from the current RTC time.
- `clock_initialize()` seeds the initial basetime from RTC state or configured start date and resets the system timer counter when needed.
- Conversion helpers such as `clock_time2ticks()`, `clock_abstime2ticks()`, and `clock_ticks2time()` support timer and delay code in neighboring modules.

## Behavioral Constraints

- `clock_settime()` accepts only `CLOCK_REALTIME`.
- `clock_getres()` also accepts only `CLOCK_REALTIME`, even if `CLOCK_MONOTONIC` support is enabled for `clock_gettime()`.
- `clock()` returns system uptime ticks, not process CPU time.
- `gettimeofday()` ignores the timezone argument and just reformats `CLOCK_REALTIME`.
- The `CLOCK_MONOTONIC` branch still shares `clock_systimespec()`, so it is not completely isolated from backend-specific `g_basetime` handling.
- `time()` and most broken-down time conversion APIs are declared in `os/include/time.h` but implemented in `lib/libc/time`, not in this module.

## Maintenance Notes

- Any change to `g_basetime` semantics affects `clock_gettime(CLOCK_REALTIME)`, `gettimeofday()`, and timer code that converts absolute deadlines.
- The hi-res RTC path is easy to misread: `clock_settime()` updates only the in-memory basetime, while `clock_systimespec()` may still read the RTC hardware.
- Keep the public comments aligned with the narrower implementation, especially around `clock()`, `CLOCK_MONOTONIC`, and the lack of direct RTC writes in `clock_settime()`.
