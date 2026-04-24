# `os/fs/dirent` Module Guide

## Purpose

`os/fs/dirent` implements the os-backed directory-stream APIs exported through `dirent.h`.

## Public APIs

- `opendir()`: open a directory stream from a path
- `closedir()`: release a directory stream
- `readdir()`: fetch the next entry from the stream
- `rewinddir()`: move the stream back to the beginning when supported
- `seekdir()`: move the stream position on a best-effort basis

Function-level notes live beside the implementation sources:

- `opendir.md`
- `closedir.md`
- `readdir.md`
- `rewinddir.md`
- `seekdir.md`

## Build and Configuration

- `os/fs/dirent/Make.defs` builds this folder only when `CONFIG_NFILE_DESCRIPTORS != 0`.
- `CONFIG_DISABLE_MOUNTPOINT` removes the mountpoint-specific branches and leaves only pseudo-filesystem traversal.
- The public owner header is `os/include/dirent.h`.

## Internal Model

- Public `DIR *` values are backed by `struct fs_dirent_s`.
- `fd_root` holds the root inode reference for the stream.
- Pseudo-filesystem streams keep the next inode to enumerate in `u.pseudo.fd_next`.
- Mountpoint-backed streams delegate operations through the filesystem inode method table.
- `fd_position` tracks the logical directory position used by `seekdir()` and by out-of-scope `telldir()`.

## Behavioral Constraints

- `opendir()` accepts only absolute paths.
- Root pseudo-filesystem directories and mountpoint-backed directories use different initialization and traversal logic.
- `readdir()` reports both EOF and error as `NULL`; EOF currently leaves `errno` as 0.
- `rewinddir()` and `seekdir()` are void APIs, so unsupported backend behavior is silent.
- `readdir_r()` and `telldir()` are declared in `dirent.h` but implemented outside `os/`, in `lib/libc/dirent`.

## Maintenance Notes

- Reference-count handling between `opendir()`, `readdir()`, `rewinddir()`, `seekdir()`, and `closedir()` is easy to break; changes here need careful review.
- Keep public comments aligned with the pseudo-filesystem versus mountpoint split, especially for EOF handling and silent no-op paths.
