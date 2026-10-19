# WPE Vibes knowledge

## B7 (ImageQualityController / BitmapImageDescriptor::sourceSize per paint)
- 2026-10-18: no profile yet (heavy jobs blocked by baseline + gates g10-g14). Static analysis only.
- Per painted bitmap (RenderImage, StyleCachedImage, StyleCanvasImage), `ImageQualityController::chooseInterpolationQuality`
  calls `image.width()` and `image.height()` -> 2 virtual `Image::size()` -> `BitmapImageDescriptor::size()` ->
  `densityCorrectedSize()` (cached flag) + `sourceSize()` (RefPtr<ImageDecoder> atomic ref/deref + `frames().isEmpty()`
  + cached flags). For a rotated leaf (Leaves) the CTM is not a translation, so it then does two HashMap finds
  (SingleThreadWeakRef<RenderBoxModelObject> -> layer -> LayoutSize), `inLiveResize()`, `getCTM()` and a cheap
  `DeferrableOneShotTimer::restart()` (only sets a flag while active). Same size every frame -> no map writes.
- Cheap possible change: call `image.size()` once instead of width()+height() (halves descriptor calls). Expected to be
  well below 1% of WebProcess time unless the profile shows otherwise; needs >= 3% of samples on Leaves/Images to be
  worth a task (rule). Profile first: `mm-profile.sh <wt> b7-leaves MotionMark/Leaves`, grep the report for
  ImageQualityController|BitmapImageDescriptor|BitmapImageSource::size.

## A/A (gate g10-aa, 2026-10-18, base 1c6fb3c6bdf2, main vs gate build directory, same source)
- Verdict REJECT: MotionMark/Suits REGRESSED -1.86% (diff/se -2.08). Every other test ok. n=8+8 (no target -> 5+3 full rounds), ~85 min.
- Per test (B-A change%, unpaired diff/se from the gate; paired t over the 8 A/B pairs; A sd%):
  Overall +0.40 / 0.56; CanvasArcs +0.29 / 0.68 / t 1.0 / 0.48; CanvasLines +0.66 / 1.65 / t 1.95 / 0.52;
  Design -0.87 / -0.45 / t -1.4 / 3.97; Images +3.92 / 0.98 / t 1.0 / 10.6 (one A run 528, others 690-760);
  Leaves +0.06 / 0.10 / t 0.3 / 0.34; Multiply +0.37 / 0.73 / t 1.7 / 0.59; Paths +0.22 / 0.33 / t 0.5 / 1.54;
  Suits -1.86 / -2.08 / t -3.3 (7 of 8 pairs negative) / 1.84.
- Suits looks systematic, not chance: the gate (B) build runs Suits ~1.9% slower than the main (A) build. Two
  explanations not separated yet: build-directory bias (same source, different paths -> different code layout) or an
  order effect (mm-ab.sh always runs A then B). Test: an A/A with the order swapped (B first), or swap the roles of
  the two directories. Until then a Suits drop of ~2% in a gate is not evidence of a regression.
- Within-gate noise is larger than the baseline noise (baseline CV: Suits 0.71%, Design 2.66%, Images 3.88%), so the
  non-target rule (drop > 1% and > 1.5 SE, any one of 8 tests + overall, unpaired) has a high family-wise false
  REJECT/BORDERLINE rate. Expect M gates (no target) to fail on noise; read their tables before acting on a REJECT.

## A/A with the order swapped (g16-aa-swap, 2026-10-18 17:03-18:29, gate build first = A, main second = B)
- REJECT again: Images -3.46% (-1.81 SE; high-noise test). Suits -0.52% (-0.65 SE): the g10 Suits drop did not
  come back, so it was chance (9 tests compared per gate, within-gate noise 1.5-3% per pair).
- CanvasLines +2.96% (6.23 SE, all 8 pairs): the gate build ran CanvasLines ~3% slower (A 16736-17279) than the main
  build (B 17542-17790), although the .text sections of every ELF file in both builds are identical (md5 checked).
  The main build is ~17640 in g10, g13 and g16 whatever its position; the gate build was ~17765 in g10 (built 12:40)
  and ~17170 in g16 (rebuilt 17:03). So a fixed per-build-instance offset, not code and not order. Hypothesis:
  physical placement of the page-cached library (cache/TLB aliasing), environment/path-length stack alignment, or
  similar. Consequence: g13-m1's CanvasLines -3.72% may be this offset, not M1.
- Two of two A/A gates REJECT: the gate's false REJECT rate is very high with the current non-target rule.

## Fixed complexity vs ramp (experiment 1, 2026-10-18 19:16-19:52, main build only, host idle, 10 interleaved rounds)
Fixed complexity = MotionMark's FixedController via an extra plan patch that passes controller/complexity/test-interval
from the URL to subtests (files were in the session job tmp dir, not committed). Metric: mean fps (20 s test, ~25 s/run).
Ramp: vibes/mm.sh single test (~50 s/run).
| test | fixed (complexity) CV | ramp CV |
| Suits | 1.21% (2000, ~35.5 fps) | 0.88% |
| Images | 1.15% (1100, ~42 fps) | 1.40% |
| Design | 0.36% (470, ~38.5 fps) | 2.03% |
Ramp noise with the host idle (0.9-2.0%) is far below the noise seen in daytime gates (2-3% per pair): host use
(the measuring AMD GPU also drives the HDMI monitor) is likely the main noise source.

## M1A (GPU memory fixes 1-4, 2026-10-18 20:30-21:45, branch task-m1a = gpu1..4 rebased on wpe-vibes)
- Memory (webkit-gpu-memory, MiniBrowser, headless Weston wl-vibes 1920x1080, 8 sites (gpu-memory/sites.txt minus
  nytimes/cnn), 20 scroll steps + 30 s idle, median of n=3, total MiB, base = main build / m1 = w1 build):
  reddit idle 742 -> 304, peak 752 -> 390; x.com idle 300 -> 214; youtube 229 -> 212; github 203 -> 139;
  wikipedia 177 -> 142; amazon 340 -> 249; yahoo.co.jp 263 -> 260 (peak 280 -> 312, yahoo varies run to run:
  base run 3 peaked 381); webkit.org 278 -> 216. Sum of idle 2532 -> 1735 (-31.5%), sum of peak 2576 -> 2052 (-20.3%).
  Raw: vibes/metrics/m1a/{base,m1}.{txt,json,log}.
- CanvasLines with the per-build offset controlled: 4 alternating w1 build instances (ccache rebuild, ~3 min each:
  m1, base, m1, base), each measured against the main build (A) with mm-ab n=3 (labels m1a-{m1x,b1,m1y,b2}).
  Main (A) was stable: 17775-17877 (within-run sd ~0.3-0.5%). w1 B: m1 17659, 17718; base 17885, 17543.
  Two base instances of identical source differ by 1.9% (~6 SE): the per-build offset is real and ~1.4% sd per
  instance. m1 vs base (2 instances each): -0.15% +- ~1.4% -> no evidence that M1 costs CanvasLines; g13's -3.72%
  is within the offset range (the gate build instance in g16 was itself ~3% slow). Cheap control for future A/Bs:
  alternate rebuilds of both sides in the same worktree and compare instance means, not runs.

## M3 (process swap fixes, branch task-m3 = task-r-m3 rebased on 1c6fb3c6bdf2: a716430bc3d3 m3a, ee9758ac3446 m3b)
- Rebase was clean. Code check of m3a: every DrawingAreaCoordinatedGraphics use of m_renderer is null-checked except
  sendEnterAcceleratedCompositingModeIfNeeded() (RELEASE_ASSERT), which returns early on the suspended drawing area
  (m_compositingAccordingToProxyMessages is already true); a restore from a SuspendedPageProxy always gets a new
  DrawingAreaProxy (ProvisionalPageProxy::initializeWebPage), so reinitializeWebPage() replaces the old drawing area.
- m3b cannot show under WebDriver: automation disables PSON, and WebProcessCache::updateCapacity() sets capacity 0
  when processSwapsOnNavigation() is off. Measure it with plain MiniBrowser (PSON on): a cancelled cross-site
  navigation (204 response) leaves a process with a site but no committed load (cached on base, exits with m3b), or a
  chain of 4 cross-site COOP navigations evicting the initial process from the back/forward cache (capacity 2).
- Run 3 numbers (/tmp/m3, same machine, plain PSON A -> B, A suspended): A's process 110.5 -> 91.5 MiB GPU (shared
  swap chain 17 -> 0); WebDriver COOP leftover about:blank process 76.6 -> 59.7 MiB (first m3a version).
- Scripts (untracked): ../wpe-vibes-w1/vibes-m3/{chain.sh,measure.sh,chain-server.py,procmem.py,nav-*}; chain.sh
  measures base (main build), builds and measures +m3a (detached task-m3~1) and +m3a+m3b (task-m3), all via heavy.sh;
  results in vibes-m3/out/<tag>-{coop,chain,go204,nav,gm}.txt (procs/GPU MiB/PSS MiB per process).
- 2026-10-18 22:55: BLOCKED, no measurement yet (g17-m1a held the heavy lock, priority gate-only, 12 target runs done
  by 22:46). Run after the gate: `nohup setsid ../wpe-vibes-w1/vibes-m3/chain.sh > ../wpe-vibes-w1/vibes-m3/out/chain-stdout.log 2>&1 &`
  (~2 incremental w1 builds + 3 x ~3 min measurements; it leaves w1 on task-m3; steps in vibes-m3/out/chain.log).
- 2026-10-19 chain results (old base 1c6fb3c6bdf2, all rc=0; n=2 MiniBrowser reps per scenario, gm n=3; values were
  identical between reps for GPU, PSS within ~2 MiB). Total GPU MiB / total PSS MiB at the last sample, base / +m3a / +m3a+m3b:
  - nav (PSON A -> B, A suspended): A's process 110.5 / 91.5 / 91.5 GPU (shared 17 -> 0); back to A: B's process
    84.7 / 67.7 / 67.7 (B's suspended renderer is also destroyed).
  - coop (about:blank -> COOP page): procs 2/2/1, GPU 170.1 / 170.1 (178.1 in one rep) / 110.5, PSS 190 / 176 / 122.
    m3a lowers PSS ~14 MiB; m3b removes the 59.7 MiB GPU leftover initial process entirely.
  - chain (4 cross-site COOP navigations): procs 10/10/9, GPU 534.0 / 496.1 / 436.4, PSS 453 / 447 / 411.
  - go204 (cancelled cross-site navigation): GPU 118.5 all three, procs 2 all three, PSS 192 / 178 / 179: m3b has
    no effect here (the 8 MiB second process is not the cached one; the cancelled-load case did not reproduce).
  - gm (WebDriver, no PSON, webkit-gpu-memory idle total): coop 231.7 / 214.7 / 214.7 (leftover process 76.6 -> 59.7),
    plain 155.1 all three.
  - Verdict: both kept (m3b has a clear effect in coop and chain: -59.7 MiB GPU, -36..-54 MiB PSS).
- 2026-10-19: task-m3 rebased onto 82698439da79 (clean), cherry-pick trailers removed: 5c9e111630e6 m3a,
  7f16801d4d3e m3b. build-w1 rc=0. Not yet gated (needs `gate.sh` with no target, M track).

## M5 (hidden/idle web process GPU release, static analysis only, 2026-10-19)
- Earlier attempt (gpu-memory/NOTES.md "Leftover web process"): release in WebPage::setIsSuspended() + hidden timer, reverted
  only because a bfcache process swap could not be reproduced in MiniBrowser, not because it was wrong. The suspended
  (process swap) case is now M3 (a716430bc3d3 destroys the renderer). The "~28 MiB idle" in NOTES is driver fixed cost
  (command buffers 8, GBM device 6, alignment), not reclaimable. M5 = hidden-but-alive pages + Mesa BO cache at idle.
- What already exists: AcceleratedSurface::visibilityDidChange() releases the *free* swap chain targets 10 s after the
  view is hidden (upstream 7e2c70f8fafe); M1A's SkiaGLContext::deferredCleanupTimerFired() keeps purging unlocked Skia
  resources every 5 s until none are purgeable (so an idle/hidden thread ends with an empty purgeable cache <= ~10 s
  after the last frame). WebProcess::nonVisibleProcess*CleanupTimer: early timer only drops decoded images with UI-side
  compositing (no-op for GLib) and is driven by IsInWindow (WPEPlatform: has toplevel), not IsVisible (mapped); the
  120 s timer is Cocoa-only (ENABLE_NON_VISIBLE_WEBPROCESS_MEMORY_CLEANUP_TIMER). So nothing calls
  DrawingArea::releaseMemory() for a hidden page except memory pressure (RSS based, blind to GPU).
- Key gap: Mesa BO caches free stale BOs only when another BO is freed/allocated: radeonsi pb_cache (expire 0.5 s, limit
  (VRAM+GTT)/8 = unbounded in practice), iris (1 s), freedreno/etnaviv/panfrost (~1 s), v3d = RPi4 (2 s, CMA memory!).
  Freed non-exported textures/renderbuffers go there; dma-buf (exported) swap chain colour buffers do not. So
  (a) everything the *last* M1A purge pass frees stays allocated in an idle process (fdinfo counts it), and
  (b) the depth/stencil renderbuffer (DEPTH24_STENCIL8, 11.1 MiB at 1080p, AcceleratedSurface.cpp:196) of each swap chain
  target released on hide stays allocated too.
- Plan B (main, all idle processes, visible or hidden), PlatformDisplaySkia.cpp only (~40 lines):
  SkiaGLContext gets m_driverCacheTrimTimer (per thread, like m_deferredCleanupTimer). deferredCleanupTimerFired():
  when the pass freed bytes (getResourceCacheUsage before/after) and getResourceCachePurgeableBytes()==0, start it
  one-shot 3 s (> v3d's 2 s). scheduleDeferredCleanup() (called per frame) stops it if active. releaseUnusedResources()
  starts it too. Fire: lock, make m_skiaGLContext current, createBackendTexture(512x512 RGBA8, mipmapped no,
  renderable no) + deleteBackendTexture (Skia keeps its GL binding state; 1 MiB > amdgpu slab max so it goes through
  pb_cache), glFlush() (glthread/tc). The free makes the driver drop every expired cached BO. Export
  PlatformDisplay::skiaScheduleDriverCacheTrim() (PlatformDisplay.h, next to skiaScheduleDeferredCleanup) and call it
  from AcceleratedSurface::releaseUnusedBuffersTimerFired() (compositor thread; frees depth/stencil of released
  targets). Env knob WEBKIT_GPU_DRIVER_CACHE_TRIM=0 to A/B in one build (avoids the per-build offset seen in M1A).
- Plan A (hidden pages), DrawingAreaCoordinatedGraphicsGLib.cpp/.h (~25 lines): RunLoop::Timer
  m_releaseHiddenResourcesTimer started in suspendPainting() (10 s, ReleaseUnusedResourcesTimer priority, same delay
  as the swap chain release), stopped in resumePainting(); fire: if m_isPaintingSuspended, releaseMemory(Critical::Yes)
  -> LayerTreeHost::releaseMemory: freeGpuResources() on the compositor thread (dispatchSync, already used by memory
  pressure) and on the main thread. Main-thread Skia context is shared by all pages of the process: only free it when
  no WebPage of the process isVisible() (else Critical::No there). freeGpuResources adds over M1A: glyph atlases
  (reddit "text atlas" 12 MiB), path renderer caches, strike cache GPU parts. Then the trim (Plan B) runs.
- Not in the plan (bigger, visual risk): dropping tiles of hidden pages (reddit ~44 MiB live tiles) needs a layer flush
  while FrameRenderer is suspended (CoordinatedBackingStoreProxy::createOrDestroyTiles clears only on empty contents
  rect / scale change); Chromium does evict hidden tabs' tiles. Live canvas/WebGL/video buffers are not reclaimable.
- Estimates (unmeasured): B = what the last purge pass frees, tens of MiB on static pages (wikipedia/github/webkit.org),
  less where video/animation keeps freeing; A = ~12 MiB atlases + 11 MiB depth/stencil per hidden 1080p page (+ B).
  First implementation step: measure B alone with the env knob before adding A; drop A if < 10 MiB.
- Measure: B with Tools/Scripts/webkit-gpu-memory as in M1A (8 sites, 20 scroll + 30 s idle >= 5+5+3 s, n=3, env knob
  0 vs default, same build). A needs a hidden view, which WebDriver cannot do (minimize does not unmap on Wayland, kiosk
  keeps unfocused toplevels mapped): new harness gpu-memory/hide.py (gi WPEWebKit-2.0 + WPEPlatform-2.0 typelibs exist
  in WebKitBuild/WPE/Release): load URL, wait 20 s, wpe_view_set_visible(view, FALSE), sample WebProcess fdinfo at
  +0/+15/+30 s, set visible again and check the page repaints. Gate: gate.sh with no target (M).
- Risks: driver-behaviour workaround (harmless 1 MiB alloc/free once per idle period elsewhere); trim must never run
  per frame (stopped in scheduleDeferredCleanup); destroying many expired BOs at once costs a few ms on an idle thread;
  Plan A main-thread freeGpuResources slows the next paint of other pages if the visibility check is wrong; show after
  >10 s hidden re-uploads images/glyphs (first frame slower, UI keeps the last buffer so no flash).

## B6 (SkiaGPUAtlas upload memmove, static analysis)
- 2026-10-19, static only (w1 busy with the M3 chain, no profile). No stored profile mentions AtlasUpload (metrics/prof empty).
- MM Images = image-data.html: ~N 50x50 2D canvases (below the accelerated-canvas size -> raster), getImageData/putImageData
  every frame. Each canvas is drawn into the parent's tiles as a raster SkImage; `GraphicsContextSkia::drawNativeImage`
  collects it (8..512 px) into `SkiaImageAtlasLayoutBuilder`; fingerprint = image pointers, so every frame is a cache miss
  (all canvases change) and all N images are re-uploaded (~N x 10 KB, ~10 MB/frame at N~1000).
- Flow: main thread `SkiaPaintingEngine::record()` -> `createAtlas()` acquires a pooled linear DMA-buf texture and
  dispatches `SkiaGPUAtlas::uploadImages()` to the serial "AtlasUpload" WorkQueue. The main thread does not wait.
  Waiters: painting workers in the promise-texture fulfill (`waitForUpload()` on the per-record AtlasUploadCondition),
  then the compositor via `CoordinatedTileBuffer::waitUntilPaintingComplete()`. So it is on the tile latency path only,
  in parallel with the main thread (which is the MM Images bottleneck: JS + getImageData, see R-B1).
- The memmove is `memcpySpan` in `MemoryMappedGPUBuffer::updateContentsInLinearFormat` (200-byte rows, 50 per image):
  the single required CPU->GPU copy. Source is zero-copy (`peekPixels`, sRGB canvases skip the conversion buffer); the
  destination mapping persists in the pool with the DmaBufFDMmap strategy (GBMBoMap unmaps after each write scope).
  Nothing to remove; per-row call overhead is negligible. Estimate ~1 ms/frame on PC, off the critical thread: < 1% score.
- DROPPED. Reopen only if a profile shows (a) AtlasUpload samples mostly in kernel page faults/clear_page (pool misses on
  atlas size change, or GBMBoMap staging), or (b) painting workers blocked in waitForUpload for > 2 ms per frame
  (then split uploadImages across several workers). Check with `mm-profile.sh` + grep AtlasUpload in prof-report.

## M4 (overlap layer backing sharing, static analysis, 2026-10-19)
- Verdict: DROPPED (no contained, mergeable change). Static analysis only (w1 busy with M3).
- Sharing design (RenderLayerCompositor.cpp:281-520, 1330-1365, 1899-1983): a layer that would composite for
  Overlap may paint into a provider only if the provider is in its containing-block chain; an overlap-composited
  layer with no composited descendants starts a new sequence, the next sibling is rejected ("non-containing block
  ancestor") and composites too, ending the sequence (isAdditionalProviderCandidate needs a composited scroller).
- RenderLayerBacking hard-codes "sharing layers are descendants of the provider": updateCompositedBounds
  (RenderLayerBacking.cpp:~1226, ASSERT isDescendantOf + offsetFromAncestor), paintIntoLayer paints sharers with
  LayerPaintingInfo root = provider (convertToLayerCoords / clip rects walk the parent chain to that root), repaint
  goes to the provider as repaint container. Sibling sharing (Chromium squashing) would need: a common-ancestor
  painting root + offset, clip rects from the common ancestor, union bounds pushed into the overlap map, repaint
  mapping for a non-ancestor container, and providers without transform/opacity/filter (the reddit 3D blurred
  <img> itself can never be a provider). Cross-cutting, high correctness risk.
- Layer-tree tests: compositing/layer-creation has 38 overlap* expectations (25 dump layerTreeAsText) and
  compositing/shared-backing 62 files; they assert exactly which siblings composite -> any sibling sharing
  changes their output = rebaselines (forbidden). Not mergeable in this experiment.
- "No painted content" variant already exists: RenderLayerBacking::updateDrawsContent (~2394-2411) sets
  drawsContent false for layers without painted content (no backing store); reddit overlap layers paint (border,
  images), so nothing to gain there.
- Gain bound: reddit m1a logs (vibes/metrics/m1a/m1.log) show 50-58 backing stores but only 30-49 GPU tiles,
  40-55 MiB total tile memory (sites with ~7 backing stores: ~25 MiB; root alone ~8 MiB on youtube). Overlap
  layers outside the coverage rect have no tiles. Sharing would save at most ~10-15 MiB on reddit idle (304 MiB,
  ~3-5%), ~0 on the other 7 sites.
- Upstream (origin/main is an ancestor of wpe-vibes): no squashing/sibling sharing work; last related commits
  bd2ad8109f59 (remove OverlappingBackingStoreProvidersEnabled pref), 0a80aee13182 (multiple providers, 2023).

## Q5 marker-with-mask-cycle-crash (SVG legacy, CSS client repaint of unattached renderer)
- Bots: crashes since 324026@main = abddd7c610ec ("Changing the children of an SVG <mask> used by CSS mask-image
  does not repaint..."), which made ReferencedSVGResources::referencedSVGResourceIDs() list mask for the legacy
  engine too. No upstream fix on origin/main as of 54a3335424a5 (2026-10-17).
- Crash is the ASSERT in RenderObject::repaint (isDescendantOf(&view())); bots' release builds have asserts, ours
  (vibes/build.sh, ENABLE_ASSERTS=AUTO) do not: main build passed --repeat-each=200 (200/200). Local reproduction
  needs a temporary RELEASE_ASSERT in RenderObject::repaint (or an asserts build); qgate cannot see the crash.
- Root cause: LegacyRenderSVGResourceContainer::styleDidChange runs from RenderElement::initializeStyle before the
  marker renderer is inserted. RenderElement::styleDidChange -> updateReferencedSVGResources() registers the marker
  as CSSSVGResourceElementClient of #mask; then registerResource() finds the pending client (path inside <mask>)
  -> markForLayoutAndParentResourceInvalidation -> parent mask container -> removeFromCacheAndInvalidateDependencies
  -> mask's referencingCSSClients -> CSSSVGResourceElementClient::resourceChanged -> marker->repaint() (no parent).
- Proposed fix (ReferencedSVGResources.cpp, legacy branch of resourceChanged): repaint only when the client renderer
  has a parent; an unattached renderer has nothing painted and is painted after insertion (same reasoning as
  RenderElement::styleDidChange's `if (!m_parent) return;`). LBSE branch already returns on needsLayout().

## Q6 fast/canvas/image-buffer-backend-variants.html (DROPPED: needs a rebaseline, 2026-10-19)
- Bots (results.py history): TEXT on every run on all 4 WPE bots (x86_64 rel 95%, arm64 100%, debug since
  315793@main = 469f4dc9f247 "[GTK][WPE] Bump wkdev-sdk to Ubuntu 26.04", which ADDED
  platform/wpe/.../image-buffer-backend-variants-expected.txt). Some TIMEOUTs on wpe-legacy-api (runtime 12-15 s).
- That WPE baseline captured spurious "Context was lost" (getImageData reads transparent black on an accelerated
  canvas) for 16384x1000 (default), 1x1/16384x1/16384x16384 forced Accelerated; the glib baseline has none of them.
- Bot diffs (12 builds, x86_64 rel/arm64/debug): actual = glib baseline except 1x1 forced 'Accelerated' (first forced
  accelerated canvas after the default section, right after 32769x32769/0-size cases) is lost on most runs; x86_64
  rel also loses a random 16384xN accelerated canvas sometimes (e.g. 16384x8192 in build 30561).
- So no source fix can match the WPE baseline (it encodes random losses); passing needs deleting the WPE baseline
  (LayoutTests change = rebaseline, forbidden). Possible real bug left: accelerated surface creation succeeds
  (ImageBufferSkiaAcceleratedBackend::create, SkSurfaces::RenderTarget non-null, canvas MSAA) but drawing/readback
  silently fails after huge (1 GiB+, MSAA x samples) canvases are still alive (not GC'd yet) -> GPU OOM in Mesa.
  Not reproduced locally (heavy lock held by gate g18-m3). Would only be checkable with a new test.

## B9 (Suits SW clip masks, branch task-b9, 2026-10-19)
- Suits: half of the particles are `<rect clip-path=url(#shape-N)>` (objectBoundingBox bezier paths). Legacy SVG
  `pathOnlyClipping` -> `GraphicsContextSkia::clipPath` -> AA SkCanvas::clipPath.
- GPU tiles are recorded as DDLs on the SkiaGPUWorker threads (CoordinatedTileBuffer m_recorder) and played back on the
  compositor. Ganesh ClipStack::apply: not analytic (not convex poly) -> AtlasPathRenderer needs a direct context
  (AtlasPathRenderer::IsSupported) so it is never used in DDLs -> no MSAA on tiles (11be498e3495) -> SW mask
  (ClipStack::GetSWMaskFP -> render_sw_mask, synchronous on the worker since a DDL context has no task group) ->
  GrSWMaskHelper::toTextureView -> one lazy approx-fit texture per mask, uploaded at playback (createTexture +
  glTexSubImage per mask). The mask cache (fMasks, key = save record gen ID + bounds) never hits: every particle has its
  own clip each frame. Old run-3 notes: compositor memmove ~3-12% from these uploads, Mesa threads 7%.
- Fix tried (bfff304f4191): DDL contexts render masks <= 256x256 into a 1024x1024 CPU atlas per ClipStack, sampled
  from one lazy proxy that uploads only the used area (GrGpu::writePixels on a sub-rect). Same mask pixels and FP
  (subset + nearest), so no rendering change. Upload bytes stay the same (plus one memcpy of each mask on the worker);
  the gain can only be per-texture overhead (texture lookup/creation, GL upload calls, staging).
- Measured 2026-10-19 (w1 build current, HEAD bfff304f4191): quick A/B mm-ab label b9, n=3 interleaved, sanity ok
  (58.3/58.4 fps). Suits A (main) 1389/1367/1362 mean 1372.5, B (atlas) 1847/1861/1801 mean 1836.3: +33.8%,
  diff/se 23.1. So per-texture overhead (createTexture + one glTexSubImage per mask at playback) dominated, not bytes.
  svg/clip-path + css3/masking on w1: 122 tests all as expected. Costs: 1 MB CPU per ClipStack that uses the atlas
  (held until DDL playback) + one 1024x1024 A8 budgeted texture per atlas (scratch-reusable).

## Q7 window.resizeTo() timeouts (headless display has no screen)
- Repro on main: all 7 targets time out (viewport-percentage-lengths-{resize,anonymous-block,percent-size-child,relative-font-size}, fast/picture/viewport-resize, fast/dom/Window/window-resize-contents, fast/images/animated-gif-window-resizing).
- Root cause: WPEDisplayHeadless had no WPEScreen, so PlatformScreenWPE screenAvailableRect() fell back to the widget bounds (= view size 800x600) and LocalDOMWindow::adjustWindowRect() clamped every resizeTo to the current size: no toplevel resize, no resize event. WKTR/toplevel resize path itself was fine.
- Fix (task-q7 09e619f576ad): virtual 1920x1080 screen (508x286 mm ~96 DPI, 60 Hz, no sync observer -> display link keeps the timer) in WPEDisplayHeadless, returned by WPEToplevelHeadless get_screen.
- Results: targets 35/35 pass with --repeat-each=5; css/mediaqueries/test_media_queries.html also passes now (5/5; fails on main), expectation removed. fast/media, fast/dom/Window, css3/viewport-percentage-lengths, WPT css/mediaqueries + cssom-view: 585 tests, no new failures. API TestUIClient+TestWebKitWebView 57/57.
- Still failing (need window position/moveTo support in headless): fast/dom/Window/window-resize.html, window-resize-and-move-arguments.html, window-resize-nan.html (screenX stays 0; nan test gets 100 instead of 500).

### Q7 rework (headless screen)
- A WPEScreen subclass inherits WPEScreen's default get_sync_observer (DRM probe of /dev/dri/card*), so a headless
  screen must override it to return nullptr; then DisplayVBlankMonitor falls back to the timer.
- After the fix (812b077fe307): 8 targets --repeat-each=5, 40/40 pass.

## Q8 fast/canvas/toDataURL-supportedTypes.html (DROPPED: by design, needs a rebaseline, 2026-10-19)
- Reproduced on the base (main build, vibes/lt/q8-repro): only diff is `image/gif` -> "Used MIMEType: image/png,
  MIME types DIFFER". Skia has no GIF encoder, so GIF is not in the encodable MIME list; that is by design.
- GTK already has exactly this result as a platform baseline (LayoutTests/platform/gtk/fast/canvas/, since 2010).
  WPE has no baseline, hence platform/wpe/TestExpectations:712 [ Failure ]. A pass needs a wpe/glib -expected file
  (forbidden rebaseline); adding a GIF encoder is out of scope.

## Q9 fast/canvas/canvas-imageSmoothingQuality.html (DROPPED: no Skia sampling mapping can pass, 2026-10-19)
- Reproduces on main (82698439da79): low/medium/high all read pixel (0,0) = {0,0,0,255}, so the 3 "is not" checks fail.
- Test: 60x12 source, rows 1-2 opaque black, drawn at 1/4 scale to 15x3, reads dest (0,0) (= source 4x4 block rows 0-3).
  Expected to pass needs low != medium != high and alpha(low) >= alpha(medium) >= alpha(high) (written for CG).
- Mapping: GraphicsContextSkia.cpp toSkSamplingOptions(): Low/Medium/Default -> linear no mips, High -> CatmullRom cubic.
  Linear samples src (2,2) -> rows 1,2 -> alpha 255. CatmullRom (taps +-0.5: 0.5625, +-1.5: -0.0625) -> 1.125 -> 255.
  Mitchell -> 1.07 -> 255; B-spline (B=1) -> 244.
- Mipmaps (raster SkMipmapAccessor, SkMipmap::ComputeLevel has a -0.5 bias): scale 1/4 -> level 1.5.
  kNearest rounds to level 2 (15x3 box) -> alpha 128; kLinear lerps level 1 (30x6, sampled 0.5) and level 2 (0.5) -> 128.
  Aniso on raster falls back to trilinear. So medium = mip nearest and high = trilinear give identical data (128):
  "mediumData is not highData" still fails; high = cubic gives 255 (> medium, breaks monotonicity, equals low).
- Passing would need a real area-averaging high-quality downscaler (cubic/Lanczos over a mip level or SkBitmapScaler-like
  resize), not available in current Skia; out of S-M scope. Using mips for Medium (as Chromium does) would be a
  quality change worth considering separately, but it does not fix this test and changes many canvas/image outputs.

## Q10 canvas-blend-{image,solid} (DROPPED: expectation is wrong, Skia is spec-correct)
- Reproduces locally on base (main worktree build, 2026-10-19): only 2 lines differ per test, both in "color-burn":
  "alpha on solid" pixelDataAtPoint(2)[1] = 64 (expected 0), "alpha on alpha" = 42 (expected 0). Both tests identical diff.
- Test note: testScenario [name, backdropAlpha, sourceAlpha]; "alpha on solid" = blue backdrop alpha 0.5, opaque green source.
- Compositing spec: Cs' = (1-ab)*Cs + ab*B(Cb,Cs); green Cs=128/255, Cb=0, B_colorburn(0,Cs)=0 -> Cs' = 0.5*0.502 -> 64.
  alpha on alpha: co = 0.5*0.251 = 0.1255, ao = 0.75 -> 42. Skia (SkRasterPipeline colorburn, SkBlendMode mapping in
  SkiaUtilities.cpp is correct) matches the spec. The generic expected values (from CG) are inconsistent: the red
  channel at point 0 (Cs=1, Cb=0, also B=0) expects 128, and darken (also B=0 for green) expects 64 at the same point.
- Fix would need changing the test's expected arrays / a rebaseline -> forbidden. Do not retry.
