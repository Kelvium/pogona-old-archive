/*
 * Copyright (c) 2022, Nikita Romanyuk <kelvium@yahoo.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <config.h>

#ifdef POGONA_VULKAN_SUPPORT

#include "vulkan_core.h"

#include <pch.h>

VulkanCore gVulkanCore = { 0 };

static VkBool32 VKAPI_PTR sDebugCallback(VkDebugUtilsMessageSeverityFlagsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* data)
{
	(void) severity;
	(void) type;
	(void) data;

	switch (callbackData->messageIdNumber) {
	// Until https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/4526 is fixed:
	case 0x3e33626b:
	case 0x3a56b425:
		return VK_FALSE;
	}

	LOGGER_DEBUG("validation: %s\n", callbackData->pMessage);
	return VK_FALSE;
}

static i32 sCreateInstance(void)
{
	// FIXME: remove these hacks
#ifndef NDEBUG
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	const u32 layersCount = 1;

	const char* extensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
	const u32 extensionsCount = 1;
#else
	const char* layers[] = {};
	const u32 layersCount = 0;

	const char* extensions[] = {};
	const u32 extensionsCount = 0;
#endif

	VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.apiVersion = VK_API_VERSION_1_1,
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0), /* TODO: */
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),      /* TODO: */
		.pApplicationName = "",                         /* TODO: */
		.pEngineName = "pogona",
	};
	VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
		.enabledLayerCount = layersCount,
		.enabledExtensionCount = extensionsCount,
		.ppEnabledLayerNames = layers,
		.ppEnabledExtensionNames = extensions,
	};

#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pfnUserCallback = sDebugCallback,
		.pUserData = NULL,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
	};
	instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
#endif

	PVK_VERIFY(vkCreateInstance(&instanceCreateInfo, NULL, &gVulkanCore.instance));
	volkLoadInstance(gVulkanCore.instance);

#ifndef NDEBUG
	PVK_VERIFY(vkCreateDebugUtilsMessengerEXT(
			gVulkanCore.instance, &debugUtilsMessengerCreateInfo, NULL, &gVulkanCore.debugUtilsMessenger));
#endif
	return 0;
}

static i32 sEnumeratePhysicalDevices(void)
{
	{
		u32 physicalDeviceGroupCount = 0;
		VkPhysicalDeviceGroupProperties* physicalDeviceGroupPropertieses = NULL; /* such a good name lol */

		PVK_VERIFY(vkEnumeratePhysicalDeviceGroups(gVulkanCore.instance, &physicalDeviceGroupCount, NULL));
		physicalDeviceGroupPropertieses = calloc(physicalDeviceGroupCount, sizeof(VkPhysicalDeviceGroupProperties));
		PVK_VERIFY(vkEnumeratePhysicalDeviceGroups(
				gVulkanCore.instance, &physicalDeviceGroupCount, physicalDeviceGroupPropertieses));

		// TODO: implement fallback device picking
		VkPhysicalDevice pickedPhysicalDevice = NULL;
		for (u32 i = 0; i < physicalDeviceGroupCount; ++i) {
			VkPhysicalDeviceGroupProperties physicalDeviceGroupProperties = physicalDeviceGroupPropertieses[i];
			for (u32 j = 0; j < physicalDeviceGroupProperties.physicalDeviceCount; ++j) {
				VkPhysicalDevice physicalDevice = physicalDeviceGroupProperties.physicalDevices[j];

				VkPhysicalDeviceProperties physicalDeviceProperties;
				vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

				// TODO: check if physicalDevice supports Vulkan 1.1

				if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					LOGGER_INFO("picked a physical device: `%s`\n", physicalDeviceProperties.deviceName);
					pickedPhysicalDevice = physicalDevice;
					break;
				}
			}
		}
		free(physicalDeviceGroupPropertieses);

		if (!pickedPhysicalDevice) {
			LOGGER_ERROR("could not pick a physical device\n");
			return -1;
		}
		gVulkanCore.physicalDevice.physicalDevice = pickedPhysicalDevice;
	}

	{
		u32 queueFamilyPropertiesCount = 0;
		VkQueueFamilyProperties* queueFamilyPropertieses = NULL;

		vkGetPhysicalDeviceQueueFamilyProperties(
				gVulkanCore.physicalDevice.physicalDevice, &queueFamilyPropertiesCount, NULL);
		queueFamilyPropertieses = calloc(queueFamilyPropertiesCount, sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(
				gVulkanCore.physicalDevice.physicalDevice, &queueFamilyPropertiesCount, queueFamilyPropertieses);

		gVulkanCore.physicalDevice.queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		for (u32 i = 0; i < queueFamilyPropertiesCount; ++i) {
			VkQueueFamilyProperties queueFamilyProperties = queueFamilyPropertieses[i];
			if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				gVulkanCore.physicalDevice.queueFamilyIndex = i;
				break;
			}
		}
		free(queueFamilyPropertieses);

		if (gVulkanCore.physicalDevice.queueFamilyIndex == VK_QUEUE_FAMILY_IGNORED) {
			LOGGER_ERROR("could not pick a queue family with graphics bit set\n");
			return -1;
		}
	}
	return 0;
}

i32 vulkanInit()
{
	if (sCreateInstance() < 0) {
		LOGGER_ERROR("could not create an instance\n");
		return -1;
	}

	if (sEnumeratePhysicalDevices() < 0) {
		LOGGER_ERROR("could not enumerate physical devices\n");
		return -1;
	}
	return 0;
}

i32 vulkanFini()
{
	vkDestroyDebugUtilsMessengerEXT(gVulkanCore.instance, gVulkanCore.debugUtilsMessenger, NULL);
	vkDestroyInstance(gVulkanCore.instance, NULL);
	return 0;
}

#endif
