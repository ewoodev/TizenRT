# `os/drivers/compression` Module Guide

## Purpose

`os/drivers/compression` provides the `/dev/compress` character driver. It turns the library helpers in `os/compression` into a user-visible ioctl interface for block compression, block decompression, backend queries, and one-at-a-time compressed-file decompression.

## Public API

- `compress_register()`: registers the driver at `COMP_DRVPATH`

Function-level API notes live next to the source:

- `compress_register.md`

## Driver Interface

- `COMPIOC_COMPRESS`: compress one caller-provided block
- `COMPIOC_DECOMPRESS`: decompress one caller-provided block
- `COMPIOC_GET_COMP_TYPE`: return the active backend enum value
- `COMPIOC_GET_COMP_NAME`: copy the backend name string
- `COMPIOC_FCOMP_INIT`: start a compressed-file decompression session
- `COMPIOC_FCOMP_GET_BUFSIZE`: expose the uncompressed file size
- `COMPIOC_FCOMP_DECOMPRESS`: materialize the file into a caller buffer
- `COMPIOC_FCOMP_DEINIT`: release the active decompression session

## Build and Kconfig

- The driver is compiled only when `CONFIG_COMPRESSION=y`.
- `os/drivers/compression/Make.defs` adds `compress.c` to the drivers library under that condition.
- Backend selection still comes from `CONFIG_COMPRESSION_TYPE` in `os/compression/Kconfig`.

## Dependencies

- `os/compression` for `compress_block()`, `decompress_block()`, `compress_init()`, `compress_read()`, `compress_uninit()`, and `get_compression_header()`
- `tinyara/fs/ioctl.h` for the `COMPIOC_*` command numbers
- VFS driver registration through `register_driver()`

## Maintenance Notes

- The file-decompression path is stateful and inherits the single-session limitation from `os/compression`.
- `compress_register()` does not report registration failure to its caller.
- `read()` and `write()` currently return `0`; the ioctl path is the only meaningful interface.
