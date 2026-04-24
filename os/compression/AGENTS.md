# Compression Module Notes

## Summary

This folder implements the in-memory compression helpers and the block-based decompression path used for compressed binaries. The public entry points are declared in `os/include/tinyara/compression.h` and `os/include/tinyara/binfmt/compression/compress_read.h`.

## Public API Groups

- Compression helpers in [compress.c](/home/ewoo/project/public/TizenRT/.worktrees/TizenRT_260417_os-api-docs/os/compression/compress.c:1):
  - `allocate_compress_buffer`
  - `compress_block`
  - `decompress_block`
- Compressed-file session helpers in [compress_read.c](/home/ewoo/project/public/TizenRT/.worktrees/TizenRT_260417_os-api-docs/os/compression/compress_read.c:1):
  - `compress_init`
  - `compress_read`
  - `compress_uninit`
  - `get_compression_header`

Function-level Markdown files live beside the implementation sources in this directory.

## Kconfig

- `CONFIG_COMPRESSION`: enables the compression library.
- `CONFIG_COMPRESSION_TYPE`: selects the backend algorithm.
  - `1`: LZMA
  - `2`: Miniz
- `CONFIG_COMPRESSED_BINARY`: enables compressed binary loading and selects `CONFIG_COMPRESSION`.
- `CONFIG_COMPRESSION_BLOCK_SIZE`: sets the block size expected in compressed binary headers.

## Data Flow

- `compress_block` and `decompress_block` are pure buffer transforms around the configured backend.
- `compress_init` parses a file header, validates it against the selected Kconfig values, and allocates temporary block buffers.
- `compress_read` maps an uncompressed byte range to compressed blocks, inflates each required block, and copies the requested bytes into the caller buffer.
- `compress_uninit` releases the session state.

## Dependencies

- Kernel heap allocation through `kmm_malloc` and `kmm_free`.
- File I/O through `lseek` and `read`.
- Compression backend libraries:
  - `tinyara/lzma/LzmaLib.h`
  - `miniz/miniz.h`
- The `/dev/compress` character driver is implemented separately under `os/drivers/compression`.

## Maintenance Notes

- The implementation is single-session: `compression_header`, `buffers`, and `active_filefd` are global.
- `compress_init` validates the on-disk header against `CONFIG_COMPRESSION_TYPE` and `CONFIG_COMPRESSION_BLOCK_SIZE`; mismatched images are rejected.
- Allocation failures after header parsing leave partially initialized global state behind until `compress_uninit()` runs.
- `compress_read` depends on prior `compress_init()` success and assumes the cached header remains valid for the lifetime of the session.
