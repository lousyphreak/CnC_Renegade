#pragma once

#include "win.h"

#include <SDL3/SDL_misc.h>

inline HINSTANCE ShellExecute(HWND, const char *, const char * file, const char *, const char *, int)
{
	if (file == nullptr || file[0] == '\0') {
		return reinterpret_cast<HINSTANCE>(31);
	}

	return SDL_OpenURL(file) ? reinterpret_cast<HINSTANCE>(33) : reinterpret_cast<HINSTANCE>(31);
}