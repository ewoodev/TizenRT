# `sem_timeout`

## Summary

`sem_timeout()` is the watchdog callback used by timed semaphore waits to cancel a blocked waiter with `ETIMEDOUT`.

## Behavior

- Enters a critical section before inspecting task state.
- Looks up the target task by the supplied process ID.
- Does nothing if the task no longer exists or is no longer blocked in `TSTATE_WAIT_SEM`.
- Calls `sem_waitirq(wtcb, ETIMEDOUT)` when the task is still blocked on a semaphore.
- Leaves all count restoration, holder cleanup, and task wakeup work to `sem_waitirq()`.

## Inputs and Outputs

- `argc`: expected watchdog argument count. The current implementation does not inspect it.
- `pid`: task ID of the blocked waiter to cancel.
- Return value: none.

## Dependencies

- `sem_timedwait.md` and `sem_tickwait.md` arm the watchdog that reaches this callback.
- Uses `sched_gettcb()` to resolve the blocked task.
- Uses `sem_waitirq()` to perform the actual timeout cleanup.

## Notes

- Although declared in the public header, this is a scheduler/watchdog callback interface rather than a normal application-facing synchronization primitive.
