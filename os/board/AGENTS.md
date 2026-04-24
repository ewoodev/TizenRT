# `os/board` Module Guide

## Purpose

`os/board` is the top-level home for board-specific hooks, board-control Kconfig, shared board helpers in `common/`, and the per-board directories that hold board-private bring-up code.

For this task, `os/board` stays on the summary-only track. This guide summarizes the public `board_*` hook families and their configuration ownership without tracing individual board implementations under `os/board/**`.

## Public API Families

The public summary surface is declared in `os/include/tinyara/board.h`.

### Boot and Board-Control Hooks

- `board_initialize()`: optional early board bring-up during boot
- `board_app_initialize()`: no-argument application bring-up hook used by `BOARDIOC_INIT`
- `board_power_off()`, `board_reset()`, `board_uniqueid()`: board-control hooks used by the common `boardctl()` dispatcher when their options are enabled
- `board_ioctl()`: optional board-specific control hook that exists in the header, but the current common `os/arch/boardctl.c` does not forward unknown commands to it

### Peripheral Bring-Up Hooks

- Touchscreen: `board_tsc_setup()`, `board_tsc_teardown()`
- Analog/PWM: `board_adc_setup()`, `board_pwm_setup()`
- Graphics and display: `board_graphics_setup()`, `board_lcd_initialize()`, `board_lcd_getdev()`, `board_lcd_uninitialize()`
- CAN: `board_can_initialize()`

These hooks are public board interfaces, but the current common `boardctl()` path does not dispatch the historical test/setup commands described by older comments.

### LEDs and Buttons

- Automatic LED status hooks: `board_autoled_initialize()`, `board_autoled_on()`, `board_autoled_off()`, `board_led_on()`
- User LED hooks: `board_userled_initialize()`, `board_userled()`, `board_userled_all()`
- Button hooks: `board_button_initialize()`, `board_buttons()`, `board_button_irq()`

Availability depends on architecture capability symbols such as `CONFIG_ARCH_LEDS`, `CONFIG_ARCH_HAVE_LEDS`, `CONFIG_ARCH_BUTTONS`, and `CONFIG_ARCH_IRQBUTTONS`.

### Fault and Recovery Hooks

- `board_crashdump()`: board-level crash-state capture hook used by assert or hard-fault recovery paths when enabled

## Build and Configuration

- `os/Kconfig` exposes `os/board/Kconfig` through the `Board Selection` menu.
- `os/board/Kconfig` owns the common board-control feature switches:
  - `CONFIG_LIB_BOARDCTL`
  - `CONFIG_BOARDCTL_POWEROFF`
  - `CONFIG_BOARDCTL_RESET`
  - `CONFIG_BOARDCTL_BOARD_HEADER`
  - `CONFIG_BOARDCTL_UNIQUEID`
  - `CONFIG_BOARDCTL_UNIQUEID_SIZE`
- `CONFIG_BOARDCTL_POWEROFF` and `CONFIG_BOARDCTL_RESET` depend on architecture capability symbols such as `ARCH_HAVE_POWEROFF` and `ARCH_HAVE_RESET`.
- `CONFIG_BOARD_CRASHDUMP`, `CONFIG_BOARD_ASSERT_AUTORESET`, and `CONFIG_BOARD_ASSERT_SYSTEM_HALT` connect board hooks to fault-handling and recovery behavior.
- `CONFIG_BOARD_ASSERT_AUTORESET` depends on `CONFIG_BOARDCTL_RESET`, so reset-hook support changes assert and reboot behavior as well as direct `boardctl()` behavior.
- `os/board/Kconfig` also selects board-specific submenus after the common options.

## Layout

- `os/board/common/`: shared helpers used by multiple boards
- `os/board/<board>/include/`: board-private headers
- `os/board/<board>/src/`: board-private bring-up and device integration

The top-level folder is intentionally broad: one public header family fans out into many board-private implementations.

## Scope Boundaries

- This guide is not a per-board implementation trace.
- Do not assume every `board_*` prototype is compiled for every build; many are individually gated in `tinyara/board.h`.
- Do not assume every board hook is reachable through the current common `boardctl()` dispatcher.
- Do not follow board-private source behavior from this guide unless the task scope explicitly expands beyond the summary track.

## Maintenance Notes

- Keep `tinyara/board.h` comments aligned with the current common `os/arch/boardctl.c` implementation. Several historical comments implied extra `boardctl()` dispatch paths that the common code no longer provides.
- `board_app_initialize()` is the public no-argument hook behind `BOARDIOC_INIT`; if that contract changes, update both `tinyara/board.h` and the `os/arch` documentation.
- The LED helper family mixes legacy automatic-status hooks with user-facing LED controls. Preserve the config guards and note that some of the automatic LED naming is already marked as deprecated in the public header.
- Changes to `BOARDCTL_RESET` support affect both explicit reset requests and fault-management flows that depend on board-level reset behavior.
