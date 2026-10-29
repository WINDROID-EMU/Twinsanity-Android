#pragma once

#include "common.h"
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace VulkanRenderer
{
// Requirements: Vulkan 1.1 Minimum
constexpr uint32_t MinVulkanApiVersion = VK_API_VERSION_1_1;

struct VulkanContext
{
    VkInstance instance{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkQueue presentQueue{VK_NULL_HANDLE};
    uint32_t graphicsQueueFamilyIndex{0};
    uint32_t presentQueueFamilyIndex{0};

    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkSwapchainKHR swapchain{VK_NULL_HANDLE};
    VkFormat swapchainFormat{VK_FORMAT_UNDEFINED};
    VkExtent2D swapchainExtent{0, 0};

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass{VK_NULL_HANDLE};
    VkCommandPool commandPool{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphores[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphores[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkFence inFlightFences[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    uint32_t currentFrame{0};
    bool initialized{false};
};

bool InitializeVulkan(struct SDL_Window* window);
void ShutdownVulkan();
bool BeginFrame();
void EndFrame();
VulkanContext& GetContext();

} // namespace VulkanRenderer

