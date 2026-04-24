# `get_errno_ptr`

## Purpose

`get_errno_ptr()` returns the errno storage location that is safe to use in the current execution context.

## Behavior

- In normal task context, it calls `this_task()` and checks that the current TCB exists and is in `TSTATE_TASK_RUNNING`.
- When that check succeeds, it returns `&rtcb->pterrno`.
- In interrupt context, early boot, or other transient states where there is no safe running task, it returns the address of the module-global fallback variable `g_irqerrno`.

## Inputs and Outputs

- Inputs: none
- Return value: writable `int *` pointing to the errno slot chosen for the current context

## Notes

- The fallback path avoids corrupting a task's errno value from interrupt handlers.
- The fallback errno storage is shared and is not designed to preserve independent values across nested interrupt paths.
