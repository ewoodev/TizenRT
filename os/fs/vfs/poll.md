# `poll`

## Summary

`poll()` monitors file and socket descriptors by installing a shared semaphore into each `pollfd`, dispatching setup through the VFS or network poll backend, waiting as needed, and then tearing the registrations down before returning the ready count.

## Behavior

- Initializes each `pollfd` with the same stack semaphore, resets `revents`, and clears the internal `priv` and `filep` fields before registration.
- Ignores negative descriptor entries and leaves their `revents` field at `0`.
- Routes file-table descriptors to `fdesc_poll()` and configured socket descriptors to `net_poll()`.
- Treats other positive descriptors as an `EBADF` failure; the current entry gets `POLLERR`, and the whole call returns `ERROR`.
- Lets `file_poll()` report `POLLERR | POLLHUP` for missing inodes, delegate driver-backed descriptors to the driver's `poll()` hook, and treat mountpoints or block devices as immediately ready for requested `POLLIN` and `POLLOUT` events.
- Performs setup even when `timeout == 0`, so already-ready descriptors still count in the immediate-return path.
- Uses `sem_timedwait()` with a `CLOCK_REALTIME` absolute deadline for positive timeouts.
- Uses `sem_wait()` through `poll_semtake()` for negative timeouts.
- Re-runs the descriptor dispatch with `setup == false` during teardown, counts entries whose `revents` field is non-zero, and clears the `sem` pointer before returning.

## Inputs and Outputs

- `fds`: array of `struct pollfd` entries to monitor.
- `nfds`: number of entries in `fds`.
- `timeout`: wait limit in milliseconds; `0` means no blocking wait after setup, negative means wait forever.
- Return value: number of entries with non-zero `revents`, `0` on timeout, or `ERROR` on failure.

## Dependencies

- Uses `poll_setup()` and `poll_teardown()` around one shared semaphore.
- Uses `fdesc_poll()` for file-table descriptors.
- Uses `net_poll()` for configured socket descriptors.
- Uses `file_poll()` underneath the file-descriptor path to talk to drivers, mountpoints, and block devices.

## Notes

- `nfds == 0` with a negative timeout can wait forever because no backend will post the semaphore.
- A list that contains only negative descriptors can also wait forever when the timeout is negative.
- Positive timeouts follow `CLOCK_REALTIME`, so wall-clock changes affect the absolute deadline.
- Although `POLLNVAL` is defined, the current VFS wrapper reports invalid positive descriptors through `EBADF` and sets `POLLERR` on the failing entry instead.
- The helper-level docs for `file_poll()`, `fdesc_poll()`, and `net_poll()` are still pending in later slices.
