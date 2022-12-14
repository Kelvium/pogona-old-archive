/*
 * Copyright (c) 2022, Nikita Romanyuk <kelvium@yahoo.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <config.h>

#ifdef POGONA_VULKAN_SUPPORT

#include "vulkan.h"

#include <pogona/window.h>

i32 vulkanCreateSurface(Window* window, VkSurfaceKHR* surface);
i32 vulkanDestroySurface(void);

#endif
