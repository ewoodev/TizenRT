# `net_initlist`

## Summary

`net_initlist()` forwards task-owned socket-list initialization to the default socket netstack.

## Behavior

- Resolves the default socket backend with `get_netstack(TR_SOCKET)`.
- Calls the backend `initlist` hook only when the backend pointer is non-NULL.
- Returns immediately without touching the list when no backend is available.
- With the current lwIP backend, zeroes every `sl_sockets[i].sock` entry and initializes `list->sl_sem` with `sem_init(..., 1)`.

## Inputs and Outputs

- `list`: pre-allocated socket-list object owned by the task or task group.
- Return value: none.

## Dependencies

- Depends on `get_netstack(TR_SOCKET)` from `netstack.c`.
- Uses the backend `initlist` hook declared in `netmgr/netstack.h`.
- The current lwIP backend implementation lives in `netstack_lwip.c`.

## Notes

- The wrapper does not validate `list`.
- Semaphore initialization is backend-defined. The current wrapper assumes a later `net_releaselist()` call will be paired with a backend that initialized `list->sl_sem`.
