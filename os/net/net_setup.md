# `net_setup`

## Summary

`net_setup()` performs the early netmgr bring-up phase before network driver initialization.

## Behavior

- Under `CONFIG_LWNL80211`, allocates `g_netmgr.dev` once with `kmm_zalloc(sizeof(struct lwnl_lowerhalf_s))` and keeps that global pointer for later reuse.
- Under `CONFIG_LWNL80211`, attempts to register that lower-half through `lwnl_register()`.
- Resolves the default socket netstack with `get_netstack(TR_SOCKET)` and calls its `init` hook through `NETSTACK_CALL_RET()`.
- Logs failures from LWNL registration or netstack initialization, but does not return a status code or stop the rest of the setup sequence.
- Always calls `netdev_mgr_start()` before returning.

## Inputs and Outputs

- Inputs: none.
- Return value: none.

## Dependencies

- Uses `get_netstack(TR_SOCKET)` and the backend `init` hook declared in `netmgr/netstack.h`.
- Depends on `lwnl_register()` when `CONFIG_LWNL80211` is enabled.
- Starts the network-device manager through `netdev_mgr_start()`.

## Notes

- Missing netstack backends or missing `init` hooks are treated like success in the current implementation because `res` is initialized to `0` before `NETSTACK_CALL_RET()`.
- The function performs no rollback if LWNL registration or stack initialization logs an error.
