/*
 * Copyright (C) 2026 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if USE(VULKAN)
#include <expected>
#include <volk.h>
#include <wtf/Noncopyable.h>

namespace WebCore {
namespace Vulkan {

template <typename Type>
using Result = std::expected<Type, VkResult>;

template <typename VulkanType>
struct BaseStruct {
    WTF_MAKE_NONCOPYABLE(BaseStruct)

    using Base = BaseStruct<VulkanType>;

    BaseStruct()
    {
        zeroBytes(m_inner);
    }

    BaseStruct(VulkanType&& inner)
        : m_inner(WTF::move(inner))
    {
    }

    ~BaseStruct() = default;

    BaseStruct(BaseStruct&& other)
        : BaseStruct()
    {
        std::swap(m_inner, other.m_inner);
    }

    BaseStruct& operator=(BaseStruct&& other)
    {
        if (this != &other) [[likely]]
            std::swap(m_inner, other.m_inner);
        return *this;
    }

    const VulkanType* LIFETIME_BOUND ptr() const { return &m_inner; }
    VulkanType* LIFETIME_BOUND ptr() { return &m_inner; }

protected:
    VulkanType m_inner;
};

template <typename VulkanType, VkStructureType vulkanStructureType>
struct Structure : BaseStruct<VulkanType> {
    using Type = VulkanType;
    using Base = BaseStruct<VulkanType>;

    static constexpr VkStructureType typeCode = vulkanStructureType;

    Structure()
    {
        this->m_inner.sType = typeCode;
    }

    Structure(VulkanType&& inner)
        : Base(WTF::move(inner))
    {
        ASSERT(this->m_inner.sType == typeCode);
    }

    Structure(Structure&& other)
        : Structure(WTF::move(other.m_inner))
    {
    }

    // Create another structure, make it the "next" to this, and return it.
    // This enables the following handy idiom to create chained Structure
    // subtype instances:
    //
    //   struct A : Structure<...> { };
    //   struct B : Structure<...> { };
    //   struct C : Structure<...> { };
    //
    //   A first;
    //   auto second = first.next<B>();
    //   auto third = second.next<C>();
    //
    // Then the resulting chain of structures is: first -> second -> third.
    //
    template <typename OtherType, typename... Args>
    requires std::derived_from<OtherType, Structure<typename OtherType::Type, OtherType::typeCode>>
    [[nodiscard]] OtherType next(Args&&... params)
    {
        OtherType nextStructure { std::forward<Args>(params)... };
        this->m_inner.pNext = nextStructure.ptr();
        return nextStructure;
    }

    const VulkanType* LIFETIME_BOUND operator->() const { return this->ptr(); }
    VulkanType* LIFETIME_BOUND operator->() { return this->ptr(); }
};

struct ApplicationInfo : Structure<VkApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO> {
    ApplicationInfo(const String& applicationName, uint32_t apiVersion = VK_API_VERSION_1_3)
        : ApplicationInfo(applicationName.utf8().data(), apiVersion)
    {
    }

private:
    ApplicationInfo(const char* applicationName, uint32_t apiVersion);
};

struct InstanceCreateInfo : Structure<VkInstanceCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO> {
    InstanceCreateInfo(const ApplicationInfo& applicationInfo LIFETIME_BOUND, std::span<const char* const> enabledLayers LIFETIME_BOUND = { }, std::span<const char* const> enabledExtensions LIFETIME_BOUND = { });
};

struct PhysicalDeviceProperties : Structure<VkPhysicalDeviceProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2> {
    const VkPhysicalDeviceProperties* LIFETIME_BOUND operator->() const { return &m_inner.properties; }
    VkPhysicalDeviceProperties* LIFETIME_BOUND operator->() { return &m_inner.properties; }
};

struct PhysicalDeviceDRMProperties : Structure<VkPhysicalDeviceDrmPropertiesEXT, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT> {
};

struct PhysicalDevice : BaseStruct<VkPhysicalDevice> {
    void fillProperties(PhysicalDeviceProperties&) const;
};

struct Instance : BaseStruct<VkInstance> {
    [[nodiscard]] static const Vector<VkLayerProperties>& availableLayers();
    [[nodiscard]] static bool hasLayers(std::span<const char* const> layerNames);
    [[nodiscard]] static bool hasLayer(const char* const layerName);

    [[nodiscard]] static Result<Vector<VkExtensionProperties>> availableExtensions(const char* layerName = nullptr);
    [[nodiscard]] static bool hasExtensions(const Vector<VkExtensionProperties>&, std::span<const char* const> extensionNames);
    [[nodiscard]] static bool hasExtension(const Vector<VkExtensionProperties>&, const char* const extensionName);
    [[nodiscard]] static bool hasExtensions(std::span<const char* const> extensionNames, const char* layerName = nullptr);
    [[nodiscard]] static bool hasExtension(const char* const extensionName, const char* layerName = nullptr);

    static void setSharedInstance(Instance&&);
    [[nodiscard]] static Instance* sharedInstanceIfExists();
    [[nodiscard]] static Instance& sharedInstance();

    [[nodiscard]] static Result<Instance> create(const InstanceCreateInfo&);
    ~Instance();

    Instance(Instance&& other)
        : Base()
    {
        *this = WTF::move(other);
    }

    Instance& operator=(Instance&& other)
    {
        if (this != &other) {
            std::swap(m_inner, other.m_inner);
#ifdef VK_EXT_debug_utils
            std::swap(m_debugMessenger, other.m_debugMessenger);
#endif
        }
        return *this;
    }

    [[nodiscard]] Result<Vector<PhysicalDevice>> availableDevices() const;
    [[nodiscard]] VkResult installDebugMessenger();

private:
    Instance(VkInstance inner)
        : Base(WTF::move(inner))
    {
    }

    static Instance s_sharedInstance;

#ifdef VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT m_debugMessenger { nullptr };
#endif
};

} // namespace Vulkan
} // namespace WebCore

#endif // USE(VULKAN)
