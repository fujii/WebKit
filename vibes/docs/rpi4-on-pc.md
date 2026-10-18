# Optimizing for the Raspberry Pi 4 on a PC

## Summary
- **Most of the candidate screening can be done on a PC.**
- How much faster a change makes the Raspberry Pi 4 cannot be predicted on a PC. The final check runs on the device.
- Use the device only for the final check of candidates, not for every attempt.

## How different they are

| | PC used for runs 1-3 | Raspberry Pi 4 |
|---|---|---|
| CPU | 64 threads, 125 GB RAM | Cortex-A72, 4 cores, 1.5 GHz, LPDDR4 |
| GPU | AMD Radeon RX 6400 (radeonsi, Mesa); also an NVIDIA GPU (proprietary driver) | VideoCore VI (Mesa v3d, GLES 3.1, tile-based) |

The performance gap is tens of times, and **the bottlenecks are in different places**. A change that shortens the
measured time on the PC does not necessarily help the Raspberry Pi 4.

## What the PC can verify
Reducing the amount of work, independently of the hardware, almost always helps the Raspberry Pi 4 too. The changes
merged in run 1 (Images, canvas dirty rect) are of this kind.
- Less repainted area, fewer tiles and layers
- Fewer draw calls, FBO switches, texture uploads and readbacks
- Removing needless copies and synchronous waits

Measured as **counts** rather than time, these are the same metrics on the PC and on the device:
- CPU instructions with `perf stat -e instructions` or callgrind
- GL calls, FBO switches and `glTexSubImage` bytes with apitrace
- MotionMark at a **fixed complexity** instead of the ramp, comparing the cost per frame

## Making the PC closer to the Raspberry Pi 4
1. **Use a Mesa GPU, not the NVIDIA proprietary driver**: the GBM / EGL / dma-buf paths are shared with v3d.
2. **Restrict the CPU**: limit to 4 cores with `taskset`, and set `WEBKIT_SKIA_CPU_PAINTING_THREADS` /
   `WEBKIT_SKIA_GPU_PAINTING_THREADS` to the device's values. Match `WEBKIT_SKIA_ENABLE_CPU_RENDERING` to the device
   too (check the settings of the Raspberry Pi 4 perf bot).
3. **Match the GL feature set**: `MESA_GLES_VERSION_OVERRIDE=3.1` and `MESA_EXTENSION_OVERRIDE` make Skia and WebKit
   take the same code paths as on the device.
4. **Use the v3d drm-shim**: Mesa's `libv3d_noop_drm_shim.so` compiles shaders for v3d without the hardware, so
   shader instruction counts can be compared (`V3D_DEBUG=shaderdb`). Useful when changing shaders.
5. **Cross-build for aarch64 on the PC and only run on the device**: this likely removes most of the time spent on the
   device.

## What the PC cannot verify
- Costs specific to tile-based GPUs (FBO flushes and load/store, sensitivity to overdraw)
- Limited memory bandwidth, A72 cache / NEON characteristics, ARM64 JIT and compiler code generation
- MotionMark score behaviour around the boundary of sticking to 60 fps or not
- Thermal throttling

## Recommended workflow
Two stages:
1. On the PC, with the conditions above, screen candidates with count-based metrics.
2. Batch the promising candidates and run MotionMark on the Raspberry Pi 4 a few times a day (automated over ssh).

For upstream changes, the Raspberry Pi 4 bots on the perf dashboard (webkit-perf-dashboard.igalia.com) help too.

## Next steps
1. A script to start a "pseudo Raspberry Pi 4" (Mesa GPU, CPU limits)
2. Fixed-complexity MotionMark and instruction / GL call counting
3. Cross-build for the Raspberry Pi 4 and automated measurement over ssh
4. Check the actual settings of the Raspberry Pi 4 perf bot (CPU / GPU rendering, thread counts)
