# `boardctl`

## Purpose

`boardctl` provides a compact board-management entry point for operations that are implemented by board hooks instead of normal character-device ioctls.

## Behavior

- The function is compiled only when `CONFIG_LIB_BOARDCTL` is enabled.
- It dispatches a small built-in command set through board-specific hooks:
  - `BOARDIOC_INIT` -> `board_app_initialize()` with the public `arg` value ignored by the current implementation
  - `BOARDIOC_POWEROFF` -> `board_power_off()` when `CONFIG_BOARDCTL_POWEROFF`
  - `BOARDIOC_RESET` -> `board_reset()` when `CONFIG_BOARDCTL_RESET`
  - `BOARDIOC_UNIQUEID` -> `board_uniqueid()` when `CONFIG_BOARDCTL_UNIQUEID`
- Unsupported commands fall through to `-ENOTTY`.

## Reset Path Notes

- `BOARDIOC_RESET` locks the scheduler before reset handling.
- It waits twice with `up_mdelay(100)` to give log output and UART FIFOs time to drain.
- When `CONFIG_SYSTEM_REBOOT_REASON` is enabled, it emits repeated warnings if the reboot reason has not been written yet.

## Return Contract

- Board hooks return `OK` or a negative errno-style status.
- `boardctl` converts negative results into `errno` with `set_errno(-ret)` and returns `ERROR`.
- Successful commands return `OK`.

## Dependencies

- The actual work is performed by hooks declared in `tinyara/board.h`.
- Command availability depends on the corresponding board-control configuration options.
