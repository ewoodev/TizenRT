# `sem_close`

## Summary

`sem_close()` drops one open reference to a named semaphore and destroys the underlying object only when it has already been unlinked and this close removes the final reference.

## Behavior

- Treats the incoming `sem_t *` as the leading field of `struct nsem_inode_s`.
- Uses the back-pointer in `nsem->ns_inode` to find the owning inode.
- Decrements `inode->i_crefs` when it is still positive.
- Checks for the combined condition `i_crefs <= 0` and `FSNODEFLAG_DELETED` to decide whether final destruction should happen now.
- On that final-destruction path, calls `sem_destroy()` on the contained unnamed semaphore, frees the `struct nsem_inode_s`, releases the inode semaphore, frees `inode->i_child`, and finally frees the inode itself.
- Otherwise just releases the inode semaphore and returns `OK`.

## Inputs and Outputs

- `sem`: named semaphore handle previously returned by `sem_open()`.
- Return value: always `OK` in the normal implementation paths shown here.

## Dependencies

- Uses `sem_destroy.md` for the contained unnamed semaphore teardown on the final close after unlink.
- Depends on inode reference counting and the `FSNODEFLAG_DELETED` state managed by `sem_unlink.md`.

## Notes

- The implementation relies on `DEBUGASSERT()` rather than runtime validation for invalid pointers or unnamed semaphores, so misuse is not reported through normal `errno` paths.
- A close on a still-linked named semaphore only releases the caller's reference. It does not remove the name from the namespace.
