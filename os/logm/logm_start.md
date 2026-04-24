# `logm_start`

## Purpose

Starts the logger module by creating the dedicated `logm` kernel thread and, when shell support is enabled, registering the `logm` TASH command.

## Behavior

- Calls `kernel_thread()` with the module's configured priority and stack size.
- Returns immediately if thread creation fails.
- Registers TASH commands only after a successful thread launch.

## Inputs and Outputs

- Inputs: none
- Output: none
- Failure reporting: none; the function silently returns when the logger thread cannot be created

## Dependencies

- `LOGM_TASK_PRORITY`
- `LOGM_TASK_STACKSIZE`
- `logm_task()`
- `logm_register_tashcmds()` when `CONFIG_TASH` is enabled

## Notes

This function does not make the logger ready by itself. The background thread sets the ready state after allocating the ring buffer.
