#include "vulkan_context.h"
#include "native.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace VulkanRenderer
{
namespace
{
VulkanContext g_VkContext;

bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            return false;
        }
    }
    return true;
}
} // namespace

VulkanContext& GetContext()
{
    return g_VkContext;
}

bool InitializeVulkan(SDL_Window* window)
{
    if (g_VkContext.initialized)
    {
        return true;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Crash Twinsanity";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Twinsanity Native";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // Explicit Vulkan 1.1 Minimum
    appInfo.apiVersion = VK_API_VERSION_1_1;

    uint32_t extensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!sdlExtensions)
    {
        Native::Log("[vulkan] failed to get SDL Vulkan instance extensions: %s", SDL_GetError());
        return false;
    }

    std::vector<const char*> extensions(sdlExtensions, sdlExtensions + extensionCount);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &g_VkContext.instance) != VK_SUCCESS)
    {
        Native::Log("[vulkan] failed to create Vulkan 1.1 instance");
        return false;
    }

    // Create window surface
    if (!SDL_Vulkan_CreateSurface(window, g_VkContext.instance, nullptr, &g_VkContext.surface))
    {
        Native::Log("[vulkan] failed to create Vulkan surface: %s", SDL_GetError());
        return false;
    }

    // Pick Physical Device (Vulkan 1.1+)
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_VkContext.instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        Native::Log("[vulkan] no Vulkan capable physical device found");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(g_VkContext.instance, &deviceCount, devices.data());

    for (const auto& dev : devices)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        if (props.apiVersion >= VK_API_VERSION_1_1)
        {
            g_VkContext.physicalDevice = dev;
            Native::Log("[vulkan] selected device: %s (Vulkan %d.%d)",
                        props.deviceName,
                        VK_API_VERSION_MAJOR(props.apiVersion),
                        VK_API_VERSION_MINOR(props.apiVersion));
            break;
        }
    }

    if (g_VkContext.physicalDevice == VK_NULL_HANDLE)
    {
        Native::Log("[vulkan] device does not support Vulkan 1.1 minimum");
        return false;
    }

    // Find queue family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_VkContext.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(g_VkContext.physicalDevice, &queueFamilyCount, queueFamilies.data());

    bool foundQueue = false;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_VkContext.physicalDevice, i, g_VkContext.surface, &presentSupport);

        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
        {
            g_VkContext.graphicsQueueFamilyIndex = i;
            g_VkContext.presentQueueFamilyIndex = i;
            foundQueue = true;
            break;
        }
    }

    if (!foundQueue)
    {
        Native::Log("[vulkan] could not find suitable graphics & presentation queue");
        return false;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = g_VkContext.graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(g_VkContext.physicalDevice, &deviceCreateInfo, nullptr, &g_VkContext.device) != VK_SUCCESS)
    {
        Native::Log("[vulkan] failed to create logical device");
        return false;
    }

    vkGetDeviceQueue(g_VkContext.device, g_VkContext.graphicsQueueFamilyIndex, 0, &g_VkContext.graphicsQueue);
    vkGetDeviceQueue(g_VkContext.device, g_VkContext.presentQueueFamilyIndex, 0, &g_VkContext.presentQueue);

    g_VkContext.initialized = true;
    Native::Log("[vulkan] successfully initialized Vulkan 1.1+ context");
    return true;
}

void ShutdownVulkan()
{
    if (!g_VkContext.initialized)
    {
        return;
    }

    if (g_VkContext.device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(g_VkContext.device);
        vkDestroyDevice(g_VkContext.device, nullptr);
        g_VkContext.device = VK_NULL_HANDLE;
    }

    if (g_VkContext.surface != VK_NULL_HANDLE && g_VkContext.instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(g_VkContext.instance, g_VkContext.surface, nullptr);
        g_VkContext.surface = VK_NULL_HANDLE;
    }

    if (g_VkContext.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(g_VkContext.instance, nullptr);
        g_VkContext.instance = VK_NULL_HANDLE;
    }

    g_VkContext.initialized = false;
}

} // namespace VulkanRenderer

