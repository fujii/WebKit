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

#include "config.h"
#include "VulkanTypes.h"

#if USE(VULKAN)
#include "Logging.h"
#include "VulkanUtilities.h"
#include <wtf/text/CStringView.h>

namespace WebCore {
namespace Vulkan {

ApplicationInfo::ApplicationInfo(const char* applicationName, uint32_t apiVersion)
{
    m_inner.pApplicationName = applicationName;
    m_inner.apiVersion = apiVersion;
}

InstanceCreateInfo::InstanceCreateInfo(const ApplicationInfo& applicationInfo, std::span<const char* const> enabledLayers, std::span<const char* const> enabledExtensions)
{
    m_inner.pApplicationInfo = applicationInfo.ptr();

    m_inner.enabledLayerCount = enabledLayers.size();
    m_inner.ppEnabledLayerNames = enabledLayers.data();

    m_inner.enabledExtensionCount = enabledExtensions.size();
    m_inner.ppEnabledExtensionNames = enabledExtensions.data();
}

void PhysicalDevice::fillProperties(PhysicalDeviceProperties& properties) const
{
    vkGetPhysicalDeviceProperties2(m_inner, properties.ptr());
}

const Vector<VkLayerProperties>& Instance::availableLayers()
{
    static const auto layerProperties = ([]() -> Vector<VkLayerProperties> {
        uint32_t layerCount;
        if (auto result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr); result != VK_SUCCESS) {
            RELEASE_LOG_ERROR(Vulkan, "Cannot enumerate instance layers: %s", resultString(result));
            return { };
        }

        Vector<VkLayerProperties> result(layerCount);
        auto resultSpan = result.mutableSpan();
        ASSERT(resultSpan.size() == layerCount);

        if (auto result = vkEnumerateInstanceLayerProperties(&layerCount, resultSpan.data()); result != VK_SUCCESS) {
            RELEASE_LOG_ERROR(Vulkan, "Cannot enumerate instance layers: %s", resultString(result));
            return { };
        }

        for (const auto& item : result)
            RELEASE_LOG_DEBUG(Vulkan, "Available layer: %s (spec %" PRIu32 ", version %" PRIu32 ")", item.layerName, item.specVersion, item.implementationVersion);

        return result;
    })();
    return layerProperties;
}

bool Instance::hasLayers(std::span<const char* const> layerNames)
{
    return std::ranges::all_of(layerNames, [](auto* layerName) -> bool {
        return availableLayers().containsIf([layerName](const auto& layer) -> bool {
            return CStringView::unsafeFromUTF8(layerName) == CStringView::unsafeFromUTF8(layer.layerName);
        });
    });
}

bool Instance::hasLayer(const char* const layerName)
{
    std::array<const char* const, 1> layerNames = { layerName };
    return hasLayers(std::span(layerNames));
}

Result<Vector<VkExtensionProperties>> Instance::availableExtensions(const char* layerName)
{
    uint32_t extensionCount;
    if (auto result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr); result != VK_SUCCESS)
        return makeUnexpected(result);

    Vector<VkExtensionProperties> result(extensionCount);
    auto resultSpan = result.mutableSpan();
    ASSERT(resultSpan.size() == extensionCount);

    if (auto result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, resultSpan.data()); result != VK_SUCCESS)
        return makeUnexpected(result);

    return result;
}

bool Instance::hasExtensions(const Vector<VkExtensionProperties>& availableExtensions, std::span<const char* const> extensionNames)
{
    return std::ranges::all_of(extensionNames, [&availableExtensions](auto* extensionName) -> bool {
        return availableExtensions.containsIf([extensionName](const auto& extension) -> bool {
            return CStringView::unsafeFromUTF8(extensionName) == CStringView::unsafeFromUTF8(extension.extensionName);
        });
    });
}

bool Instance::hasExtension(const Vector<VkExtensionProperties>& availableExtensions, const char* const extensionName)
{
    std::array<const char* const, 1> extensionNames = { extensionName };
    return hasExtensions(availableExtensions, std::span(extensionNames));
}

bool Instance::hasExtensions(std::span<const char* const> extensionNames, const char* layerName)
{
    if (auto extensions = availableExtensions(layerName))
        return hasExtensions(*extensions, extensionNames);
    return false;
}

bool Instance::hasExtension(const char* const extensionName, const char *layerName)
{
    std::array<const char* const, 1> extensionNames = { extensionName };
    return hasExtensions(std::span(extensionNames), layerName);
}

Instance Instance::s_sharedInstance { nullptr };

void Instance::setSharedInstance(Instance&& instance)
{
    if (s_sharedInstance.m_inner)
        RELEASE_ASSERT_NOT_REACHED_WITH_MESSAGE("Attempted to reset already initialized Vulkan shared instance");
    s_sharedInstance = WTF::move(instance);
}

Instance* Instance::sharedInstanceIfExists()
{
    return s_sharedInstance.m_inner ? &s_sharedInstance : nullptr;
}

Instance& Instance::sharedInstance()
{
    if (!s_sharedInstance.m_inner) [[unlikely]]
        RELEASE_ASSERT_NOT_REACHED_WITH_MESSAGE("Attempted to use Vulkan shared instance before its initialization");
    return s_sharedInstance;
}

Result<Instance> Instance::create(const InstanceCreateInfo& creationInfo)
{
    VkInstance instance;
    if (auto result = vkCreateInstance(creationInfo.ptr(), nullptr, &instance); result != VK_SUCCESS)
        return makeUnexpected(result);

    volkLoadInstanceOnly(instance);
    return Instance(instance);
}

Instance::~Instance()
{
#ifdef VK_EXT_debug_utils
    if (m_debugMessenger)
        vkDestroyDebugUtilsMessengerEXT(m_inner, m_debugMessenger, nullptr);
#endif // VK_EXT_debug_utils

    if (m_inner)
        vkDestroyInstance(m_inner, nullptr);
}

Result<Vector<PhysicalDevice>> Instance::availableDevices() const
{
    uint32_t deviceCount;
    if (auto result = vkEnumeratePhysicalDevices(m_inner, &deviceCount, nullptr); result != VK_SUCCESS)
        return makeUnexpected(result);

    Vector<PhysicalDevice> devices(deviceCount);
    static_assert(sizeof(PhysicalDevice) == sizeof(VkPhysicalDevice));
    auto devicesSpan = spanReinterpretCast<VkPhysicalDevice>(devices.mutableSpan());
    ASSERT(devicesSpan.size() == deviceCount);
    if (auto result = vkEnumeratePhysicalDevices(m_inner, &deviceCount, devicesSpan.data()); result != VK_SUCCESS)
        return makeUnexpected(result);

    return devices;
}

#ifdef VK_EXT_debug_utils
struct DebugUtilsMessengerCreateInfo : Structure<VkDebugUtilsMessengerCreateInfoEXT, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT> {
};

static VkBool32 debugUtilsMessengerHandleMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
{
    ASSERT(data->pMessage);

    const WTFLogLevel logLevel = [messageSeverity] {
        switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return WTFLogLevel::Error;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return WTFLogLevel::Info;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return WTFLogLevel::Warning;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return WTFLogLevel::Debug;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }();

    StringBuilder messageTypeString;
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        messageTypeString.append("General, "_s);
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        messageTypeString.append("Validation, "_s);
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        messageTypeString.append("Performance, "_s);
    RELEASE_ASSERT(messageTypeString.length() > 2);
    messageTypeString.shrink(messageTypeString.length() - 2);

    if (data->pMessageIdName)
        RELEASE_LOG_WITH_LEVEL(Vulkan, logLevel, "[%s: %s] %s", data->pMessageIdName, messageTypeString.toString().utf8().data(), data->pMessage);
    else
        RELEASE_LOG_WITH_LEVEL(Vulkan, logLevel, "[%s] %s", messageTypeString.toString().utf8().data(), data->pMessage);

    return VK_FALSE;
}

VkResult Instance::installDebugMessenger()
{
    if (m_debugMessenger)
        return VK_SUCCESS;

    DebugUtilsMessengerCreateInfo createInfo;
    createInfo->pfnUserCallback = debugUtilsMessengerHandleMessage;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    if (LOG_CHANNEL(Vulkan).level >= WTFLogLevel::Info)
        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    if (LOG_CHANNEL(Vulkan).level >= WTFLogLevel::Warning)
        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    if (LOG_CHANNEL(Vulkan).level >= WTFLogLevel::Debug)
        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    return vkCreateDebugUtilsMessengerEXT(m_inner, createInfo.ptr(), nullptr, &m_debugMessenger);
}
#else
VkResult Instance::installDebugMessenger() const
{
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
#endif // VK_EXT_debug_utils

} // namespace Vulkan
} // namespace WebCore

#endif // USE(VULKAN)
