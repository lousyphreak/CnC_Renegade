#include "always.h"
#include "msgloop.h"

#include <SDL3/SDL_events.h>

void (*Message_Pre_Poll_Handler)(void) = NULL;
bool (*Message_Intercept_Handler)(SDL_Event &event) = NULL;

void Windows_Message_Handler(void)
{
	if (Message_Pre_Poll_Handler != NULL) {
		Message_Pre_Poll_Handler();
	}

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (Message_Intercept_Handler != NULL && Message_Intercept_Handler(event)) {
			continue;
		}
	}
}
