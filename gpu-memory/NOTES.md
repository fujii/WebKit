# WPE GPU memory investigation

## Measurement setup
- `Tools/Scripts/webkit-gpu-memory`: MiniBrowser (WebDriver), Chrome (CDP), Firefox (WebDriver BiDi, ~/firefox).
  Sums DRM fdinfo of the browser process tree, clients deduplicated by (drm-pdev, drm-client-id).
  "total" counts dma-bufs shared between processes twice (WPE swap chain buffers: WebProcess + UIProcess);
  "private" excludes everything shared.
- Run every browser inside a headless Weston on the amdgpu (RX 6400), kiosk shell, 1920x1080, scale 1:
  `weston --backend=headless --renderer=gl --width=1920 --height=1080 --shell=kiosk --socket=wl-gpumem --no-config`
  then `WAYLAND_DISPLAY=wl-gpumem gpu-memory/run-compare.sh <tag> [repeat] [browsers]`.
  Why: on the desktop, WPE stops rendering when its window is not visible (no frame callbacks), and the
  desktop runs at 150% where WPE renders at 2x (no wp_fractional_scale_v1 in WPEPlatform) while Chrome and
  Firefox render at 1.5x. Neither WPE `--size` nor WebDriver set-window-rect resizes the WPE window there.
- `WEBKIT_GPU_MEMORY_DEBUG=1` logs a breakdown every 2s of rendering (Skia caches per thread, tiles, swap chain,
  BitmapTexturePool); `=2` also logs the composited layers of the main frame grouped by compositing reasons.

## Findings so far (2026-10-17)
- Desktop, fractional scale 150%: WPE renders at device scale 2, Chrome/Firefox at 1.5 -> 1.78x pixels for every
  buffer and tile in WPE. Real-world issue for fractional scaling setups; not addressed yet.
- reddit.com/r/popular, Weston 1920x1080: WPE ~418 MiB total (WebProcess 373), Chrome ~111 MiB.
  Chrome: 5 composited layers (3 drawing). WPE: 156 GraphicsLayers.
- WPE WebProcess breakdown on reddit (after load + 10 scroll steps):
  - compositor Skia context: 217 MiB, of which 142 MiB purgeable (unused, kept as cache):
    uploaded images 57 MiB, scratch textures 45 MiB + 15 MiB, text atlas 12 MiB, render target 8 MiB.
  - live GPU tiles: ~44 MiB (31 tiles).
  - swap chain: 2 x 1920x1080 color (GBM) + 2 x DEPTH24_STENCIL8 renderbuffers (AcceleratedSurface).
    Skia's StencilAttachment/MakeWrappedRenderBuffer is a bookkeeping wrapper (renderbuffer 0), not memory,
    but it is counted in the Skia budget.
  - main thread Skia context: 20 MiB (mostly purgeable).
  - fdinfo total is ~130 MiB above what Skia + swap chain explain: TBD (Mesa BO cache? GL context overhead?).

## Fixes (local commits)
1. 40e8ece38c29 Purge Skia GPU resources not used for 5 s (per thread, every 5 s while rendering).
   reddit 10 scroll steps: WebProcess 360 -> 287 MiB.
2. c827f352fe5d MSAA only for canvas accelerated ImageBuffers. RenderLayerFilters creates two accelerated
   ImageBuffers (source graphic + FilterImage) for every repaint of a non-composited layer with a CSS filter,
   and they were 8x MSAA on x86 (sample count + 1 times the memory: ~19 MiB for 834x636). Tiles use no MSAA.
   reddit 20 scroll steps: idle 719 -> 464 MiB, peak 959 -> 547 MiB, main thread Skia 245 -> 73 MiB.

## Other observations
- Backing sharing (RenderLayerCompositor.cpp:281-311, 419-493) only lets an overlap layer paint into a provider
  that is an ancestor in its containing-block chain; overlapping siblings never share. No Chromium-like squashing.
  reddit: each post's 3D-transformed blurred <img> is composited and the following siblings (overlay border div,
  <img>, <figure>) each get an Overlap layer with its own backing store (46-58 overlap layers vs 5 layers in Chrome).
- Tile coverage: root layer ~4 viewports tall (cover = 2x visible in both axes, width clipped to the content and
  the area moved to the height by adjustForContentsRect). ~45 MiB on reddit. Not the main gap.
- x.com / youtube: a second WebProcess (the about:blank one, ~76 MiB GPU) stays alive after the cross-site
  navigation (process swap) with its swap chain and Skia contexts.
- CNN times out in WPE (load > 60 s), excluded.
- Mesa radeonsi BO cache (pb_cache): freed BOs expire after 500 ms but are destroyed only on the next BO
  allocation/free, limit (VRAM+GTT)/8. Freed memory can linger at idle.
- Unexplained in WebProcess after the fixes (reddit): fdinfo 419 MiB vs Skia (main 73 + compositor 157) +
  depth/stencil 16 = 246 MiB -> ~170 MiB unexplained (video/GStreamer? Mesa internals? BO cache?).
3. 7bb5156dd4f1 Purge the 512x512 MSAA test surface (8 MiB per web process with 8 samples).
4. e68b0b9069c6 GPU tile height rounded down to a multiple of 256 (1920x270 tiles took ~42% more than their
   logical size on amdgpu: textures are allocated in 64KB swizzle blocks, 128x128 pixels at 32 bpp, so 270 rows
   are padded to 384).
   wikipedia WebProcess 158 -> 146 MiB, webkit.org 233 -> 189 MiB.

## Bot checks / measurement pitfalls
- reddit shows "Prove your humanity" to Chrome with a fresh profile (Firefox and WPE get the page). The user passed
  it once in a persistent profile: ~/.cache/webkit-gpu-memory/chrome (run-compare.sh passes it with --profile).
- Firefox opened on the X11 desktop while DISPLAY was set; the script now drops DISPLAY and forces Wayland.
- The script flags pages that can't scroll (NOSCROLL) and records the title.

## Where the "unexplained" WebProcess memory goes (gpu-memory/botrace, reddit)
- LD_PRELOAD shim on amdgpu_bo_alloc/free. Buffers Mesa keeps in its BO cache never reach amdgpu_bo_free, so they
  show as live (e.g. a 9 MiB MSAA renderbuffer in an idle process).
- reddit WebProcess (fdinfo 223 MiB, 182 MiB traced): Skia textures 85, GStreamer VA-API video decoding ~35,
  swap chain color 11.1 and depth/stencil 11.1 per buffer (8.3 logical, alignment), Mesa command buffers created
  with exportable fences 8, GBM device 6, Skia vertex/index buffers a few MiB.
- So the residual is video + alignment + driver fixed costs (~28 MiB for an idle web process), not one big leak.

## Leftover web process after the about:blank -> cross-site navigation (~75 MiB)
- Not a back/forward cache page (about:blank isn't cacheable), so WebPage::setIsSuspended() isn't called and the
  hidden-page timer never fires. Likely a cached/kept process whose freed buffers sit in Mesa's BO cache (freed BOs
  are destroyed only on the next allocation/free). Tried releasing memory in setIsSuspended() and on the hidden
  timer; could not reproduce a bfcache process swap in MiniBrowser, so reverted (TODO).

## Chrome baseline (Weston 1920x1080, profile with reddit bot check passed), idle MiB
reddit 313, x.com 162, github 139, wikipedia 155, amazon 191, yahoo.co.jp 199, webkit.org 165, cnn 600.

## Comparison after fixes 1-4 (Weston 1920x1080, 20 scroll steps, median of 2, MiB)
idle total / peak total; "private" excludes buffers shared between processes (WPE's swap chain is shared
between the WebProcess and the UIProcess, so "total" counts it twice).

| site | Chrome | Firefox | WPE before | WPE after | private idle: Chrome / Firefox / WPE before / WPE after |
|---|---|---|---|---|---|
| reddit | 313 / 626 | 301 / 328 | 782 / 929 | 345 / 475 | 287 / 283 / 731 / 294 |
| x.com | 162 / 270 | 231 / 237 | 360 / 366 | 267 / 320 | 136 / 214 / 300 / 208 |
| youtube (no scroll) | 139 / 193 | 207 / 207 | 269 / 272 | 251 / 261 | 113 / 190 / 209 / 192 |
| github | 139 / 189 | 209 / 337 | 246 / 246 | 183 / 215 | 113 / 192 / 195 / 132 |
| wikipedia | 155 / 173 | 227 / 233 | 223 / 223 | 187 / 198 | 128 / 210 / 172 / 136 |
| amazon (Firefox blocked) | 191 / 217 | - | 451 / 459 | 293 / 324 | 164 / - / 400 / 242 |
| yahoo.co.jp | 199 / 232 | 237 / 249 | 420 / 436 | 313 / 396 | 171 / 219 / 369 / 262 |
| webkit.org | 165 / 298 | 200 / 200 | 310 / 331 | 260 / 293 | 138 / 183 / 259 / 209 |
nytimes (WPE and Chrome got a bot check) and cnn (WPE and Firefox time out) excluded.
Results: gpu-memory/results/{baseline,fix4}-*.{txt,json,log} (not committed: large).
