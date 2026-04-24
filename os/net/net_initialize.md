# `net_initialize`

## Summary

`net_initialize()` completes the later netmgr bring-up phase after timers and interrupts are available.

## Behavior

- Starts virtual WLAN support through `vwifi_start()` when `CONFIG_VIRTUAL_WLAN` is enabled.
- Starts virtual BLE support through `vble_start()` when `CONFIG_VIRTUAL_BLE` is enabled.
- Resolves the default socket netstack with `get_netstack(TR_SOCKET)` and calls its `start` hook through `NETSTACK_CALL_RET()`.
- Logs and asserts when the stack start hook is missing or returns a negative result.
- Starts the TRWiFi event handler through `trwifi_run_handler()`.
- Logs and asserts when the TRWiFi event handler fails to start.

## Inputs and Outputs

- Inputs: none.
- Return value: none.

## Dependencies

- Uses `get_netstack(TR_SOCKET)` and the backend `start` hook declared in `netmgr/netstack.h`.
- Depends on `trwifi_run_handler()` for post-stack event processing.
- Optionally starts virtual interfaces before the main stack start.

## Notes

- Unlike `net_setup()`, this function treats missing backend support as fatal because `res` starts at `-1` before `NETSTACK_CALL_RET()`.
- The function does not attempt cleanup after a failed stack start or event-handler start; it asserts instead.
