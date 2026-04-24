# `net_releaselist`

## Summary

`net_releaselist()` forwards socket-list teardown to the default socket netstack and then destroys the list semaphore.

## Behavior

- Rejects `list == NULL` only through `DEBUGASSERT`.
- Resolves the default socket backend with `get_netstack(TR_SOCKET)`.
- Calls the backend `releaselist` hook only when the backend pointer is non-NULL.
- Always destroys `list->sl_sem` with `sem_destroy()` after the optional backend call.
- With the current lwIP backend, iterates every socket slot, closes sockets whose reference count is `1`, decrements positive reference counts greater than `1`, zeroes fully released socket entries, and leaves early if a backend close fails.

## Inputs and Outputs

- `list`: initialized socket-list object to tear down.
- Return value: none.

## Dependencies

- Depends on `get_netstack(TR_SOCKET)` from `netstack.c`.
- Uses the backend `releaselist` hook declared in `netmgr/netstack.h`.
- The current lwIP backend implementation lives in `netstack_lwip.c`.

## Notes

- The wrapper destroys `list->sl_sem` even when no backend is available, so callers rely on a prior successful `net_initlist()` path that initialized the semaphore.
- Backend close failures in the current lwIP implementation leave the loop early, but the wrapper still destroys the semaphore afterward.
