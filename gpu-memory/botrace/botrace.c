// LD_PRELOAD shim that attributes amdgpu buffer object allocations to their callers.
//
// Interposes libdrm's amdgpu_bo_alloc()/amdgpu_bo_free(), which Mesa's radeonsi uses for every
// buffer object it gets from the kernel (textures, renderbuffers, buffers, slabs, ...). Buffers
// that Mesa reuses from its own BO cache don't call amdgpu_bo_alloc() again, so they stay
// attributed to the first allocation.
//
// Every BOTRACE_INTERVAL seconds (default 5), writes $BOTRACE_DIR/botrace-<pid>.txt with the live
// allocations grouped by the first BOTRACE_FRAMES (default 6) caller frames outside Mesa/libdrm/
// libEGL/libgallium, sorted by size.
//
// Build: gcc -O1 -g -fPIC -shared -o libbotrace.so botrace.c -ldl -lpthread
// Use:   LD_PRELOAD=/path/libbotrace.so BOTRACE_DIR=/tmp/dir <command>

#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct amdgpu_bo_alloc_request {
    uint64_t alloc_size;
    uint64_t phys_alignment;
    uint32_t preferred_heap;
    uint64_t flags;
};
typedef void* amdgpu_bo_handle;
typedef void* amdgpu_device_handle;

#define MAX_FRAMES 48
#define TABLE_SIZE (1 << 16)

struct entry {
    amdgpu_bo_handle handle;
    uint64_t size;
    uint32_t heap;
    int frameCount;
    void* frames[MAX_FRAMES];
};

static struct entry* table;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int (*realAlloc)(amdgpu_device_handle, struct amdgpu_bo_alloc_request*, amdgpu_bo_handle*);
static int (*realFree)(amdgpu_bo_handle);
static __thread int inHook;

static size_t slotFor(amdgpu_bo_handle handle)
{
    return ((uintptr_t)handle >> 4) * 2654435761u % TABLE_SIZE;
}

static void* reporter(void*);

static void initialize(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    void init(void)
    {
        // Mesa loads libdrm_amdgpu with RTLD_LOCAL, so RTLD_NEXT can't find it.
        void* libdrm = dlopen("libdrm_amdgpu.so.1", RTLD_LAZY | RTLD_LOCAL);
        realAlloc = dlsym(libdrm, "amdgpu_bo_alloc");
        realFree = dlsym(libdrm, "amdgpu_bo_free");
        table = calloc(TABLE_SIZE, sizeof(struct entry));
        if (getenv("BOTRACE_DIR")) {
            pthread_t thread;
            pthread_create(&thread, NULL, reporter, NULL);
            pthread_detach(thread);
        }
    }
    pthread_once(&once, init);
}

int amdgpu_bo_alloc(amdgpu_device_handle dev, struct amdgpu_bo_alloc_request* request, amdgpu_bo_handle* handle)
{
    initialize();
    int result = realAlloc(dev, request, handle);
    if (result || !table || inHook)
        return result;
    inHook = 1;
    void* frames[MAX_FRAMES];
    int frameCount = backtrace(frames, MAX_FRAMES);
    pthread_mutex_lock(&lock);
    size_t slot = slotFor(*handle);
    for (size_t i = 0; i < TABLE_SIZE; ++i) {
        struct entry* entry = &table[(slot + i) % TABLE_SIZE];
        if (!entry->handle || entry->handle == (void*)1) {
            entry->handle = *handle;
            entry->size = request->alloc_size;
            entry->heap = request->preferred_heap;
            entry->frameCount = frameCount;
            memcpy(entry->frames, frames, sizeof(void*) * frameCount);
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    inHook = 0;
    return result;
}

int amdgpu_bo_free(amdgpu_bo_handle handle)
{
    initialize();
    if (table) {
        pthread_mutex_lock(&lock);
        size_t slot = slotFor(handle);
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            struct entry* entry = &table[(slot + i) % TABLE_SIZE];
            if (!entry->handle)
                break;
            if (entry->handle == handle) {
                entry->handle = (void*)1; // Tombstone.
                break;
            }
        }
        pthread_mutex_unlock(&lock);
    }
    return realFree(handle);
}

static int isDriverFrame(const char* module)
{
    static const char* driverModules[] = { "libgallium", "libdrm", "libEGL", "libGLESv2", "libGLdispatch", "libepoxy", "libbotrace", "libgbm", "dri_gbm", "libc.so", "drv_video", "libva", NULL };
    for (int i = 0; driverModules[i]; ++i) {
        if (strstr(module, driverModules[i]))
            return 1;
    }
    return 0;
}

static void describeFrames(struct entry* entry, int wanted, char* buffer, size_t bufferSize)
{
    size_t length = 0;
    int written = 0;
    buffer[0] = 0;
    for (int i = 1; i < entry->frameCount && written < wanted; ++i) {
        Dl_info info;
        if (!dladdr(entry->frames[i], &info) || !info.dli_fname)
            continue;
        if (isDriverFrame(info.dli_fname))
            continue;
        // module+offset, symbolized offline (dladdr() doesn't see hidden symbols).
        const char* module = strrchr(info.dli_fname, '/');
        module = module ? module + 1 : info.dli_fname;
        length += snprintf(buffer + length, bufferSize - length, "%s%s+0x%lx", written ? " <- " : "", module,
            (unsigned long)((uintptr_t)entry->frames[i] - (uintptr_t)info.dli_fbase - 1));
        if (length >= bufferSize)
            break;
        written++;
    }
}

struct group {
    char key[2048];
    uint64_t size;
    unsigned count;
};

static int compareGroups(const void* a, const void* b)
{
    const struct group* groupA = a;
    const struct group* groupB = b;
    return groupA->size < groupB->size ? 1 : groupA->size > groupB->size ? -1 : 0;
}

static void* reporter(void* unused)
{
    (void)unused;
    const char* directory = getenv("BOTRACE_DIR");
    int interval = getenv("BOTRACE_INTERVAL") ? atoi(getenv("BOTRACE_INTERVAL")) : 5;
    int frames = getenv("BOTRACE_FRAMES") ? atoi(getenv("BOTRACE_FRAMES")) : 6;
    static struct group groups[4096];
    char path[4096];
    snprintf(path, sizeof(path), "%s/botrace-%d.txt", directory, getpid());
    for (;;) {
        sleep(interval);
        inHook = 1;
        size_t groupCount = 0;
        uint64_t total = 0;
        unsigned count = 0;
        pthread_mutex_lock(&lock);
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            struct entry* entry = &table[i];
            if (!entry->handle || entry->handle == (void*)1)
                continue;
            char key[2048];
            describeFrames(entry, frames, key, sizeof(key));
            total += entry->size;
            count++;
            size_t g;
            for (g = 0; g < groupCount; ++g) {
                if (!strcmp(groups[g].key, key))
                    break;
            }
            if (g == groupCount) {
                if (groupCount == sizeof(groups) / sizeof(groups[0]))
                    continue;
                strcpy(groups[g].key, key);
                groups[g].size = 0;
                groups[g].count = 0;
                groupCount++;
            }
            groups[g].size += entry->size;
            groups[g].count++;
        }
        pthread_mutex_unlock(&lock);
        qsort(groups, groupCount, sizeof(groups[0]), compareGroups);
        FILE* file = fopen(path, "w");
        if (file) {
            fprintf(file, "total %.1f MiB in %u BOs\n", total / 1048576.0, count);
            for (size_t g = 0; g < groupCount; ++g)
                fprintf(file, "%8.1f MiB %5u  %s\n", groups[g].size / 1048576.0, groups[g].count, groups[g].key);
            fclose(file);
        }
        inHook = 0;
    }
    return NULL;
}
