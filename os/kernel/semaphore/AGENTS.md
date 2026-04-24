# `os/kernel/semaphore` Module Guide

## Purpose

`os/kernel/semaphore` contains the kernel-side wait, wakeup, timeout, holder-tracking, and recovery helpers behind TizenRT semaphore objects.

This folder does not own every public semaphore API declared under `os/include/`. The public surface is split across this folder, `lib/libc/semaphore`, and `os/fs/semaphore`.

## Public APIs Covered in This Folder

- `sem_destroy()`
- `sem_wait()`
- `sem_trywait()`
- `sem_timedwait()`
- `sem_timeout()`
- `sem_post()`
- `sem_tickwait()`
- `sem_reset()`
- `sem_setprotocol()` when `CONFIG_PRIORITY_INHERITANCE=y`
- `sem_register()` and `sem_unregister()` as recovery-only helpers when `CONFIG_BINMGR_RECOVERY=y`

Function-level notes live beside the implementation sources in this folder.

## Related Public APIs Owned Elsewhere

- `sem_init()`, `sem_getvalue()`, and `sem_getprotocol()` are declared in `os/include/` but currently implemented in `lib/libc/semaphore/`.
- `sem_setprotocol()` also has a libc fallback implementation when `CONFIG_PRIORITY_INHERITANCE` is disabled.
- `sem_open()`, `sem_close()`, and `sem_unlink()` are the named-semaphore APIs owned by `os/fs/semaphore/`.

## Build and Configuration

- `os/kernel/semaphore/Make.defs` always builds the core wait/post files: `sem_destroy.c`, `sem_wait.c`, `sem_trywait.c`, `sem_timedwait.c`, `sem_post.c`, `sem_recover.c`, `sem_reset.c`, `sem_waitirq.c`, and `sem_tickwait.c`.
- `os/kernel/Kconfig` exposes `CONFIG_PRIORITY_INHERITANCE`.
- `os/kernel/Kconfig` exposes `CONFIG_SEM_PREALLOCHOLDERS` under the priority-inheritance menu.
- `os/Kconfig.debug` exposes `CONFIG_SEMAPHORE_HISTORY`.
- `os/kernel/binary_manager/Kconfig` exposes `CONFIG_BINARY_MANAGER` and `CONFIG_BINMGR_RECOVERY`.
- When `CONFIG_PRIORITY_INHERITANCE=y`, `sem_initialize.c`, `sem_holder.c`, and the kernel-side `sem_setprotocol.c` are built.
- When `CONFIG_BINARY_MANAGER=y`, `sem_list.c` is built so kernel semaphores can participate in recovery tracking.
- `CONFIG_BINMGR_RECOVERY` gates the public `sem_register()` and `sem_unregister()` prototypes and the call sites that use the recovery list.

## Internal Model

1. `sem_wait()` and `sem_trywait()` decrement `semcount` and record holder information on successful acquire paths.
2. `sem_wait()` blocks by storing the waited-on semaphore in the current TCB, making `semcount` negative, and moving the task to `TSTATE_WAIT_SEM`.
3. `sem_post()` increments `semcount`, releases holder ownership for the posting task, and wakes one waiter through `sem_unblock_task()`.
4. `sem_timedwait()` and `sem_tickwait()` layer watchdog timeouts on top of the normal `sem_wait()` path and use `sem_timeout()` plus `sem_waitirq()` for cleanup.
5. `sem_reset()` is the recovery-style escape hatch that feeds counts to existing waiters before storing a remaining count directly.
6. Holder tracking in `sem_holder.c` supports priority inheritance and, in some builds, binary-manager recovery bookkeeping.
7. `sem_list.c` maintains a global list of kernel semaphores for binary-manager recovery.

## Behavioral Constraints

- The stored `semcount` is the real internal count. Negative values mean waiters are blocked.
- `sem_destroy()` does not wake blocked waiters before invalidating the semaphore.
- `sem_timedwait()` uses an absolute `CLOCK_REALTIME` deadline; `sem_tickwait()` uses a relative tick budget adjusted against a caller-provided start tick.
- `sem_setprotocol()` only supports `SEM_PRIO_NONE` and `SEM_PRIO_INHERIT` in the kernel implementation. `SEM_PRIO_PROTECT` is unsupported.
- `sem_register()` and `sem_unregister()` are recovery helpers, not general synchronization operations.

## Dependencies

- Scheduler and task-state helpers under `os/kernel/sched`
- Watchdog helpers for timed waits
- `tinyara/cancelpt.h` and the task cancellation-point implementation
- Binary-manager recovery when kernel semaphores must survive or be cleaned up after faults

## Scope Boundaries

- This guide covers the public entry points implemented in `os/kernel/semaphore`.
- It does not replace the libc-owned documentation for `sem_init()`, `sem_getvalue()`, `sem_getprotocol()`, or the named-semaphore module under `os/fs/semaphore`.

## Maintenance Notes

- Keep `os/include/semaphore.h` aligned with the real internal `semcount` semantics; callers can observe negative counts through `sem_getvalue()`.
- Timed-wait documentation should always mention the watchdog and `sem_waitirq()` cleanup path. Most implementation subtleties live there, not only in the public wrappers.
- Any change to holder tracking, priority restoration, or binary-manager recovery needs synchronized updates across this guide, `tinyara/semaphore.h`, and the relevant function-level Markdown.
