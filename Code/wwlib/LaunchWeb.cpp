#include "LaunchWeb.h"

#include <SDL3/SDL_misc.h>

bool LaunchWebBrowser(const char * url)
{
	return (url != nullptr) && (url[0] != '\0') && SDL_OpenURL(url);
}
