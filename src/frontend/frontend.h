#pragma once

#if defined(HELIAGE_FRONTEND_SDL)
#include "sdl.h"
#elif defined(HELIAGE_FRONTEND_IMGUI)
#include "imgui.h"
#else
#include "null.h"
#endif
