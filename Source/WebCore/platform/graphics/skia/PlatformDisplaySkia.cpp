/*
 * Copyright (C) 2024 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformDisplay.h"

#if USE(SKIA)
#include "FontRenderOptions.h"
#include "GLContext.h"

WTF_IGNORE_WARNINGS_IN_THIRD_PARTY_CODE_BEGIN
#include <skia/core/SkColorSpace.h>
#include <skia/core/SkExecutor.h>
#include <skia/core/SkTraceMemoryDump.h>
#include <skia/gpu/ganesh/GrBackendSurface.h>
#include <skia/gpu/ganesh/SkSurfaceGanesh.h>
#include <skia/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <skia/gpu/ganesh/gl/GrGLDirectContext.h>
#include <skia/gpu/ganesh/gl/GrGLInterface.h>

#if USE(LIBEPOXY)
#include <epoxy/egl.h>
#include <skia/gpu/ganesh/gl/epoxy/GrGLMakeEpoxyEGLInterface.h>
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <skia/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h>
#endif
WTF_IGNORE_WARNINGS_IN_THIRD_PARTY_CODE_END

#include <wtf/MemoryPressureHandler.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RAMSize.h>
#include <wtf/RunLoop.h>
#if USE(GLIB_EVENT_LOOP)
#include <wtf/glib/RunLoopSourcePriority.h>
#endif
#include <wtf/ThreadSafeWeakPtr.h>
#include <wtf/text/StringToIntegerConversion.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/MakeString.h>
#include <map>
#include <string_view>
#include <string>

#if USE(LIBDRM)
#include <fcntl.h>
#include <unistd.h>
#include <wtf/unix/UnixFileDescriptor.h>
#include <xf86drm.h>
#endif

namespace WebCore {

#if PLATFORM(GTK) || PLATFORM(WPE)
#if CPU(X86) || CPU(X86_64)
// On x86 ot x86_64 we need at least 8 samples for the antialiasing result to be similar
// to non MSAA.
static const unsigned s_defaultSampleCount = 8;
#else
// On embedded, we sacrifice a bit of antialiasing quality to save memory and improve
// performance.
static const unsigned s_defaultSampleCount = 4;
#endif
#else
// Disable MSAA by default.
static const unsigned s_defaultSampleCount = 0;
#endif

#if !(PLATFORM(PLAYSTATION) && USE(COORDINATED_GRAPHICS))
static sk_sp<const GrGLInterface> skiaGLInterface()
{
    static NeverDestroyed<sk_sp<const GrGLInterface>> grGLInterface {
#if USE(LIBEPOXY)
        GrGLInterfaces::MakeEpoxyEGL()
#else
        GrGLInterfaces::MakeEGL()
#endif
    };

    return grGLInterface.get();
}

static thread_local RefPtr<SkiaGLContext> s_skiaGLContext;

#if USE(LIBDRM)
static inline bool isNewIntelDevice()
{
    auto eglDisplay = eglGetCurrentDisplay();
    if (eglDisplay == EGL_NO_DISPLAY)
        return false;

    if (!GLContext::isExtensionSupported(eglQueryString(nullptr, EGL_EXTENSIONS), "EGL_EXT_device_query"))
        return false;

    EGLDeviceEXT eglDevice;
    if (!eglQueryDisplayAttribEXT(eglDisplay, EGL_DEVICE_EXT, reinterpret_cast<EGLAttrib*>(&eglDevice)))
        return false;

    if (!GLContext::isExtensionSupported(eglQueryDeviceStringEXT(eglDevice, EGL_EXTENSIONS), "EGL_EXT_device_drm"))
        return false;

    const char* device = eglQueryDeviceStringEXT(eglDevice, EGL_DRM_DEVICE_FILE_EXT);
    if (!device || !*device)
        return false;

    auto fd = UnixFileDescriptor { open(device, O_RDWR | O_CLOEXEC), UnixFileDescriptor::Adopt };
    if (!fd)
        return false;

    drmDevicePtr drmDevice;
    if (drmGetDevice2(fd.value(), 0, &drmDevice))
        return false;

    if (drmDevice->bustype != DRM_BUS_PCI) {
        drmFreeDevice(&drmDevice);
        return false;
    }

    auto vendorID = drmDevice->deviceinfo.pci->vendor_id;
    auto deviceID = drmDevice->deviceinfo.pci->device_id;
    drmFreeDevice(&drmDevice);

    static constexpr uint32_t intelVendorID = 0x8086;
    if (vendorID != intelVendorID)
        return false;

    // On pre-Ice Lake Intel GPUs MSAA performance is not acceptable.
    uint32_t maskedDeviceID = deviceID & 0xFF00;
    switch (maskedDeviceID) {
    case 0x2900: // Broadwater
    case 0x2A00: // Broadwater or Eaglelake
    case 0x2E00: // Eaglelake
    case 0x0000: // Ironlake
    case 0x0100: // Ivybridge, Baytrail or Sandybridge
    case 0x0F00: // Baytrail
    case 0x0A00: // Apollolake or Haswell
    case 0x0400: // Haswell
    case 0x0C00: // Haswell
    case 0x0D00: // Haswell
    case 0x2200: // Cherrytrail
    case 0x1600: // Broadwell
    case 0x5A00: // Apollolake or Cannonlake
    case 0x1900: // Skylake
    case 0x1A00: // Apollolake
    case 0x3100: // Geminilake
    case 0x5900: // Amberlake or Kabylake
    case 0x8700: // Kabylake or Coffeelake
    case 0x3E00: // Whiskeylake or Coffeelake
    case 0x9B00: // Cometlake
        return false;
    case 0x8A00: // Icelake
    case 0x4500: // Elkhartlake
    case 0x4E00: // Jasperlake
    case 0x9A00: // Tigerlake
    case 0x4c00: // Rocketlake
    case 0x4900: // DG1
    case 0x4600: // Alderlake
    case 0x4F00: // Alchemist
    case 0x5600: // Alchemist
    case 0xA700: // Raptorlake
    case 0x7D00: // Arrowlake or Meteorlake
    case 0xB600: // Arrowlake or Meteorlake
    case 0x6400: // Lunarlake
    case 0xE200: // Battlemage
    case 0xB000: // Pantherlake
        return true;
    default:
        break;
    }

    return false;
}
#endif

static bool shouldAllowMSAAOnNewIntel()
{
#if USE(LIBDRM)
    static std::once_flag onceFlag;
    static bool allowMSAAOnNewIntel;
    std::call_once(onceFlag, [] {
        allowMSAAOnNewIntel = isNewIntelDevice();
    });
    return allowMSAAOnNewIntel;
#else
    return false;
#endif
}

static unsigned initializeMSAASampleCount(GrDirectContext* grContext)
{
    static std::once_flag onceFlag;
    static int sampleCount = s_defaultSampleCount;

    std::call_once(onceFlag, [grContext] {
        // Let the user override the default sample count if they want to.
        String envString = String::fromLatin1(getenv("WEBKIT_SKIA_MSAA_SAMPLE_COUNT"));
        if (!envString.isEmpty())
            sampleCount = parseInteger<unsigned>(envString).value_or(0);

        if (sampleCount <= 1) {
            // Values of 0 or 1 mean disabling MSAA.
            sampleCount = 0;
            return;
        }

        // Skia checks internally whether MSAA is supported, but also disables it for several platforms where it
        // knows there are bugs. The only way to know whether our sample count will work is trying to create a
        // surface with that value and check whether it works.
        auto imageInfo = SkImageInfo::Make(512, 512, kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
        SkSurfaceProps properties = FontRenderOptions::singleton().createSurfaceProps();
        auto surface = SkSurfaces::RenderTarget(grContext, skgpu::Budgeted::kNo, imageInfo, sampleCount, kTopLeft_GrSurfaceOrigin, &properties);

        // If the creation of the surface failed, disable MSAA.
        if (!surface)
            sampleCount = 0;
    });

    return sampleCount;
}

static constexpr size_t s_lowEndDeviceMemoryThresholdBytes = 2 * GB;
static constexpr size_t s_highEndDeviceMemoryThresholdBytes = 4 * GB;

static constexpr size_t s_lowEndDeviceGlyphCacheTextureMaximumBytes = 2 * MB;
static constexpr size_t s_lowEndMaxResourceCacheBytes = 48 * MB;
static constexpr size_t s_maxResourceCacheBytes = 80 * MB;

static std::optional<size_t> glyphCacheTextureMaximumBytes()
{
    size_t memorySize = WTF::ramSize();
    if (memorySize < s_lowEndDeviceMemoryThresholdBytes)
        return s_lowEndDeviceGlyphCacheTextureMaximumBytes;
    return std::nullopt;
}

static size_t initializeMaxResourceCacheBytes(GrDirectContext* grContext)
{
    size_t memorySize = WTF::ramSize();
    if (memorySize < s_lowEndDeviceMemoryThresholdBytes)
        grContext->setResourceCacheLimit(s_lowEndMaxResourceCacheBytes);
    else if (memorySize < s_highEndDeviceMemoryThresholdBytes)
        grContext->setResourceCacheLimit(s_maxResourceCacheBytes);
    // Use GrDirectContext default (256MB) for high end devices.
    return grContext->getResourceCacheLimit();
}

class SkiaGLContext : public ThreadSafeRefCountedAndCanMakeThreadSafeWeakPtr<SkiaGLContext> {
public:
    static Ref<SkiaGLContext> create(PlatformDisplay& display)
    {
        return adoptRef(*new SkiaGLContext(display));
    }

    static Ref<SkiaGLContext> create(std::unique_ptr<GLContext>&& glContext)
    {
        return adoptRef(*new SkiaGLContext(WTF::move(glContext)));
    }

    ~SkiaGLContext()
    {
        auto glContext = WTF::move(m_skiaGLContext);
        auto grContext = WTF::move(m_skiaGrContext);
        if (!glContext)
            return;

        // The context of a thread is destroyed when the thread exits, which can happen while the main thread
        // terminates the display. Once it is terminated, drop the Skia resources without calling into GL.
        RefPtr display = glContext->display();
        bool didRelease = display && display->runIfNotTerminated([&](EGLDisplay) {
            glContext->makeContextCurrent();
            grContext->releaseResourcesAndAbandonContext();
            grContext = nullptr;
        });
        if (!didRelease)
            grContext->abandonContext();
    }

    GLContext* skiaGLContext() const
    {
        Locker locker { m_lock };
        return m_skiaGLContext.get();
    }

    GrDirectContext* skiaGrContext() const
    {
        Locker locker { m_lock };
        return m_skiaGrContext.get();
    }

    unsigned sampleCount() const
    {
        return m_sampleCount;
    }

    size_t maxResourceCacheBytes() const
    {
        return m_maxResourceCacheBytes;
    }

    void releaseUnusedResources(WTF::Critical critical)
    {
        Locker locker { m_lock };
        if (!m_skiaGLContext)
            return;

        GLContext::ScopedGLContextCurrent scopedCurrent(*m_skiaGLContext);
        switch (critical) {
        case Critical::No:
            m_skiaGrContext->purgeUnlockedResources(GrPurgeResourceOptions::kScratchResourcesOnly);
            break;
        case Critical::Yes:
            m_skiaGrContext->freeGpuResources();
            break;
        }
    }

    // Purges the resources that were not used for the cleanup delay. Rendering starts the timer if it
    // is not running, so it fires every cleanup delay while rendering goes on, and again while purgeable
    // resources remain. Without this, unused textures (tiles that were dropped, decoded images, atlases,
    // offscreen surfaces...) stay in the cache until it reaches its limit, which is 256MB per
    // thread on devices with 4GB of RAM or more, because nothing else releases them unless the
    // system is under memory pressure.
    void scheduleDeferredCleanup()
    {
        auto delay = resourceCleanupDelay();
        if (!delay)
            return;
        if (!m_deferredCleanupTimer) {
            m_deferredCleanupTimer = makeUnique<RunLoop::Timer>(RunLoop::currentSingleton(), "SkiaGLContext::DeferredCleanupTimer"_s, this, &SkiaGLContext::deferredCleanupTimerFired);
#if USE(GLIB_EVENT_LOOP)
            m_deferredCleanupTimer->setPriority(RunLoopSourcePriority::ReleaseUnusedResourcesTimer);
#endif
        }
        if (!m_deferredCleanupTimer->isActive())
            m_deferredCleanupTimer->startOneShot(*delay);
    }

private:
    static std::optional<Seconds> resourceCleanupDelay()
    {
        static std::optional<Seconds> delay = [] -> std::optional<Seconds> {
            String envString = String::fromLatin1(getenv("WEBKIT_SKIA_RESOURCE_CLEANUP_DELAY"));
            if (envString.isEmpty())
                return 5_s;
            auto seconds = parseInteger<unsigned>(envString).value_or(0);
            if (!seconds)
                return std::nullopt;
            return Seconds(seconds);
        }();
        return delay;
    }

    void deferredCleanupTimerFired()
    {
        Locker locker { m_lock };
        if (!m_skiaGLContext)
            return;

        GLContext::ScopedGLContextCurrent scopedCurrent(*m_skiaGLContext);
        auto delay = *resourceCleanupDelay();
        m_skiaGrContext->performDeferredCleanup(std::chrono::milliseconds(static_cast<int64_t>(delay.milliseconds())));
        // Resources that were used too recently to be purged now are checked again later.
        if (m_skiaGrContext->getResourceCachePurgeableBytes())
            m_deferredCleanupTimer->startOneShot(delay);
    }

    explicit SkiaGLContext(PlatformDisplay& display)
        : SkiaGLContext(GLContext::createOffscreen(display))
    {
    }

    explicit SkiaGLContext(std::unique_ptr<GLContext>&& glContext)
    {
        if (!glContext || !glContext->makeContextCurrent())
            return;

        // FIXME: Add shader cache.
        GrContextOptions options;
        options.fAllowMSAAOnNewIntel = shouldAllowMSAAOnNewIntel();
        // Skia defaults this to true, which alters the texture2D() calls in all generated shaders,
        // applying a so-called negative LOD bias, forcing texture2D() sampling into a LOD-computing
        // lookup which is a slow-path in certain tiled GPUs (such as Vivante/etnaviv). Since we
        // don't use any mipmapping, it's safe to disable that, and avoid performance hits.
        options.fSharpenMipmappedTextures = false;
        thread_local std::unique_ptr<SkExecutor> s_executor = SkExecutor::MakeFIFOThreadPool(2);
        options.fExecutor = s_executor.get();
        if (auto bytes = glyphCacheTextureMaximumBytes())
            options.fGlyphCacheTextureMaximumBytes = *bytes;

        if (auto grContext = GrDirectContexts::MakeGL(skiaGLInterface(), options)) {
            m_skiaGLContext = WTF::move(glContext);
            m_skiaGrContext = WTF::move(grContext);
            m_sampleCount = initializeMSAASampleCount(m_skiaGrContext.get());
            m_maxResourceCacheBytes = initializeMaxResourceCacheBytes(m_skiaGrContext.get());
        }
    }

    std::unique_ptr<GLContext> m_skiaGLContext WTF_GUARDED_BY_LOCK(m_lock);
    sk_sp<GrDirectContext> m_skiaGrContext WTF_GUARDED_BY_LOCK(m_lock);
    mutable Lock m_lock;
    std::unique_ptr<RunLoop::Timer> m_deferredCleanupTimer;
    unsigned m_sampleCount { 0 };
    size_t m_maxResourceCacheBytes { 0 };
};
#endif

GLContext* PlatformDisplay::skiaGLContext()
{
#if PLATFORM(GTK) || PLATFORM(WPE) || (PLATFORM(PLAYSTATION) && !USE(COORDINATED_GRAPHICS))
    if (!s_skiaGLContext) {
        s_skiaGLContext = SkiaGLContext::create(*this);
        m_skiaGLContexts.add(*s_skiaGLContext);
    }
    return s_skiaGLContext->skiaGLContext();
#else
    // The PlayStation OpenGL implementation does not dispatch to the context bound to
    // the current thread so Skia cannot use OpenGL with coordinated graphics.
    return nullptr;
#endif
}

void PlatformDisplay::setSkiaGLContextForCurrentThread(std::unique_ptr<GLContext>&& glContext)
{
    if (!glContext) {
        clearSkiaGLContext();
        return;
    }
    ASSERT(!s_skiaGLContext);
    s_skiaGLContext = SkiaGLContext::create(WTF::move(glContext));
    m_skiaGLContexts.add(*s_skiaGLContext);
}

GrDirectContext* PlatformDisplay::skiaGrContext() const
{
    RELEASE_ASSERT(s_skiaGLContext);
    return s_skiaGLContext->skiaGrContext();
}

unsigned PlatformDisplay::msaaSampleCount() const
{
    return s_skiaGLContext ? s_skiaGLContext->sampleCount() : 0;
}

size_t PlatformDisplay::maxSkiaResourceCacheBytes() const
{
    return s_skiaGLContext ? s_skiaGLContext->maxResourceCacheBytes() : 0;
}

void PlatformDisplay::skiaScheduleDeferredCleanup()
{
    if (s_skiaGLContext)
        s_skiaGLContext->scheduleDeferredCleanup();
}

void PlatformDisplay::skiaReleaseUnusedResources(WTF::Critical critical)
{
    if (s_skiaGLContext)
        s_skiaGLContext->releaseUnusedResources(critical);
}

void PlatformDisplay::clearSkiaGLContext()
{
    s_skiaGLContext = nullptr;
}

// GPU memory investigation: aggregates GrDirectContext::dumpMemoryStatistics() of the current
// thread's context by resource type, label and cache category.
class GPUMemoryStatistics final : public SkTraceMemoryDump {
public:
    void dumpNumericValue(const char* dumpName, const char* valueName, const char*, uint64_t value) final
    {
        auto& resource = m_resources[dumpName];
        if (std::string_view(valueName) == "size")
            resource.size = value;
        else if (std::string_view(valueName) == "purgeable_size")
            resource.purgeable = true;
    }

    void dumpStringValue(const char* dumpName, const char* valueName, const char* value) final
    {
        auto& resource = m_resources[dumpName];
        if (std::string_view(valueName) == "type")
            resource.type = value;
        else if (std::string_view(valueName) == "label")
            resource.label = value;
        else if (std::string_view(valueName) == "category")
            resource.category = value;
    }

    void dumpWrappedState(const char* dumpName, bool isWrapped) final { m_resources[dumpName].wrapped = isWrapped; }
    void setMemoryBacking(const char*, const char*, const char*) final { }
    void setDiscardableMemoryBacking(const char*, const SkDiscardableMemory&) final { }
    LevelOfDetail getRequestedDetails() const final { return kObjectsBreakdowns_LevelOfDetail; }

    String description() const
    {
        struct Group {
            uint64_t bytes { 0 };
            unsigned count { 0 };
        };
        std::map<std::string, Group> groups;
        uint64_t total = 0, purgeable = 0, wrapped = 0;
        for (const auto& [name, resource] : m_resources) {
            total += resource.size;
            if (resource.purgeable)
                purgeable += resource.size;
            if (resource.wrapped)
                wrapped += resource.size;
            auto key = resource.type + "/" + resource.label + "/" + resource.category + (resource.purgeable ? "/purgeable" : "") + (resource.wrapped ? "/wrapped" : "");
            auto& group = groups[key];
            group.bytes += resource.size;
            group.count++;
        }
        StringBuilder builder;
        builder.append("total="_s, total >> 10, "KiB purgeable="_s, purgeable >> 10, "KiB wrapped="_s, wrapped >> 10, "KiB"_s);
        Vector<std::pair<std::string, Group>> sorted;
        for (const auto& group : groups)
            sorted.append(group);
        std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second.bytes > b.second.bytes; });
        for (const auto& [key, group] : sorted)
            builder.append("\n    "_s, String::fromUTF8(key.c_str()), ": "_s, group.bytes >> 10, "KiB ("_s, group.count, ')');
        return builder.toString();
    }

private:
    struct Resource {
        uint64_t size { 0 };
        std::string type;
        std::string label;
        std::string category;
        bool purgeable { false };
        bool wrapped { false };
    };
    std::map<std::string, Resource> m_resources;
};

bool PlatformDisplay::gpuMemoryDebugLoggingEnabled()
{
    static bool enabled = !!getenv("WEBKIT_GPU_MEMORY_DEBUG");
    return enabled;
}

String PlatformDisplay::skiaGPUMemoryDescription()
{
    if (!s_skiaGLContext)
        return "no context"_s;
    auto* grContext = s_skiaGLContext->skiaGrContext();
    if (!grContext)
        return "no context"_s;
    GPUMemoryStatistics statistics;
    grContext->dumpMemoryStatistics(&statistics);
    int count;
    size_t bytes;
    grContext->getResourceCacheUsage(&count, &bytes);
    return makeString("budgeted="_s, bytes >> 10, "KiB limit="_s, grContext->getResourceCacheLimit() >> 10, "KiB "_s, statistics.description());
}

} // namespace WebCore

#endif // USE(SKIA)
