# `os/arch` Module Guide

## Purpose

`os/arch` is the top-level architecture glue layer for the OS tree. In this worktree, the public API owned directly by the top-level `os/arch` folder is the common `boardctl()` dispatch entry point, while the architecture-specific implementation trees live under `os/arch/arm` and `os/arch/xtensa`.

This guide documents the shared dispatch contract in `boardctl.c`. It does not attempt to trace board-private implementations under `os/board/**`.

## Public APIs

- `boardctl()`: common board-control entry point for a small built-in command set

Function-level notes live beside the source:

- `boardctl.md`

Related `board_*` hooks are declared in `tinyara/board.h`, but they stay on the `os/board` summary track for this task.

## Build and Configuration

- `os/Kconfig` exposes `os/arch/Kconfig` as the architecture-selection menu.
- `os/arch/Kconfig` chooses the active CPU architecture (`CONFIG_ARCH_ARM` or `CONFIG_ARCH_XTENSA`), sets the derived `CONFIG_ARCH` string, and sources the architecture-specific submenus.
- `os/arch/Kconfig` also owns capability symbols such as `ARCH_HAVE_RESET`, `ARCH_HAVE_POWEROFF`, `ARCH_HAVE_MMU`, `ARCH_USE_MMU`, and related address-environment options.
- `boardctl()` itself is not gated by a dedicated symbol in `os/arch/Kconfig`. Its public availability comes from `CONFIG_LIB_BOARDCTL`, which is defined under `os/board/Kconfig`.
- Individual built-in commands are gated separately:
  - `CONFIG_BOARDCTL_POWEROFF` enables `BOARDIOC_POWEROFF`
  - `CONFIG_BOARDCTL_RESET` enables `BOARDIOC_RESET`
  - `CONFIG_BOARDCTL_UNIQUEID` enables `BOARDIOC_UNIQUEID`
- `CONFIG_BOARDCTL_BOARD_HEADER` controls whether `sys/boardctl.h` also includes `<arch/chip/boardctl.h>` for board-specific command definitions. In the current tree, `os/arch/arm/Kconfig` selects that symbol.

## Internal Model

1. `boardctl()` receives a public command ID and one untyped argument.
2. The switch in `os/arch/boardctl.c` handles only the built-in command subset compiled into the image.
3. Each supported command dispatches into a hook declared by `tinyara/board.h`:
   - `BOARDIOC_INIT` -> `board_app_initialize()` with the public `arg` value ignored by the current implementation
   - `BOARDIOC_POWEROFF` -> `board_power_off()`
   - `BOARDIOC_RESET` -> `board_reset()`
   - `BOARDIOC_UNIQUEID` -> `board_uniqueid()`
4. Negative hook returns are converted into `errno` and surfaced as `ERROR`.
5. Unsupported commands fall through to `-ENOTTY`.

## Behavioral Constraints

- `boardctl()` is compiled only when `CONFIG_LIB_BOARDCTL` is enabled.
- The current `boardctl.c` implementation does not provide a generic dispatcher for `BOARDIOC_USER` or other board-defined command ranges. Unsupported commands still fail with `ENOTTY`.
- `BOARDIOC_RESET` is more than a direct hook call: it locks the scheduler, delays twice with `up_mdelay(100)`, emits reset logging, optionally warns when the reboot reason has not been recorded, and then calls `board_reset()`.
- Command semantics are not fully board-independent. The top-level dispatcher only standardizes the call/return pattern around board hooks.

## Dependencies

- `tinyara/board.h` provides the hook declarations that do the real board-specific work.
- `sys/boardctl.h` owns the public command IDs and the `boardctl()` declaration.
- `tinyara/arch.h` provides architecture helpers used by the reset path such as `up_mdelay()` and, when enabled, reboot-reason helpers.
- `sched_lock()` / `sched_unlock()` serialize the reset path around the board reset callback.

## Scope Boundaries

- Do not treat this guide as a per-board behavior reference. Board-private implementations remain out of scope while `os/board/**` stays on the summary-only track.
- Do not assume every `BOARDIOC_*` symbol in public headers is handled by the current `os/arch/boardctl.c` implementation.
- Do not attribute board-control Kconfig ownership to `os/arch/Kconfig` when the gating symbols actually come from `os/board/Kconfig`.

## Maintenance Notes

- If `boardctl.c` grows support for additional built-in commands or for `BOARDIOC_USER` dispatch, update both this guide and `boardctl.md`.
- The reset path is used as a common reboot entry from multiple non-app contexts elsewhere in the tree, so changes there affect crash, assert, and recovery flows as well as direct application calls.
- Keep the public comments aligned with the narrower implementation: `boardctl()` is a small shared dispatcher, not a complete abstraction over all board-specific control paths.
