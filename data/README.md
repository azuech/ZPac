# Runtime Data

This directory contains pre-built binary assets required at runtime.

## zpac_tiles.bin

The complete tileset for ZPac: 384 tiles at 16×16 pixels, 4bpp, totaling ~49KB.
This file is streamed to VRAM at startup via HostFS and must be accessible
to the emulator as `H:/zpac_tileset.bin`. On real hardware must be present on the same path of
the executable.

When running with the Zeal Native Emulator, use the `-H` flag pointing to
this directory (or copy the file to wherever your `-H` path points).

This file was generated offline from different game description sources through the asset pipeline
(decode → scale 1.5× → slice → deduplicate → encode 4bpp).
