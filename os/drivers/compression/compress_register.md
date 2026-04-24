# `compress_register`

## Purpose

Registers the `/dev/compress` character driver that exposes the compression module through a small ioctl API.

## Behavior

- Calls `register_driver()` with `COMP_DRVPATH`, `compress_fops`, and mode `0666`.
- Publishes a driver with stub `read()` and `write()` handlers and a functional `ioctl()` handler.
- Does not keep or return the result of `register_driver()`.

## Driver Surface

- `COMPIOC_COMPRESS` and `COMPIOC_DECOMPRESS` forward block operations to the library helpers.
- `COMPIOC_GET_COMP_TYPE` and `COMPIOC_GET_COMP_NAME` expose the configured backend.
- `COMPIOC_FCOMP_INIT`, `COMPIOC_FCOMP_GET_BUFSIZE`, `COMPIOC_FCOMP_DECOMPRESS`, and `COMPIOC_FCOMP_DEINIT` wrap the compressed-file decompression flow.

## Inputs and Outputs

- Inputs: none
- Output: none
- Failure reporting: none directly; registration failure is not surfaced to the caller

## Dependencies

- `COMP_DRVPATH`
- `compress_fops`
- `register_driver()`
- Compression helpers in `os/compression`

## Notes

Because the function ignores the return value from `register_driver()`, callers cannot detect duplicate registration or driver-registration failure through this API alone.
