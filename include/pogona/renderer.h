/*
 * Copyright (c) 2022, Nikita Romanyuk <kelvium@yahoo.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <pogona/types.h>
#include <pogona/window.h>

typedef void* RendererApi;

typedef enum {
	RENDERER_API_TYPE_NONE,

#ifdef POGONA_VULKAN_SUPPORT
	RENDERER_API_TYPE_VULKAN,
#endif

	RENDERER_API_TYPE_ANY,
} RendererApiType;

const char* rendererApiTypeToString(RendererApiType apiType);

typedef struct {
	Window* window;
	RendererApiType apiType;
} Renderer;

i32 rendererCreate(Renderer* self, RendererApiType apiType, Window* window);
i32 rendererDraw(Renderer* self);
i32 rendererDestroy(Renderer* self);

#ifdef __cplusplus
}
#endif
