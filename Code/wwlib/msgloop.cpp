#include "always.h"
#include "msgloop.h"

#include <SDL3/SDL_events.h>

bool (*Message_Intercept_Handler)(SDL_Event &event) = NULL;

void Windows_Message_Handler(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (Message_Intercept_Handler != NULL && Message_Intercept_Handler(event)) {
			continue;
		}
	}
}
