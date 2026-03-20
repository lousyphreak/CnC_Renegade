#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "commando_bootstrap_bridge.h"
#include "renegade_build_config.h"

namespace {

bool HasArgument(int argc, char **argv, std::string_view needle)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && needle == argv[i]) {
            return true;
        }
    }

    return false;
}

void PrintCommandoBanner()
{
    std::cout
        << "Renegade Commando executable\n"
        << "  platform target: " << RENEGADE_BOOTSTRAP_PLATFORM << '\n'
        << "  SDL version pin: " << RENEGADE_SDL3_VERSION << '\n'
        << "  x86 asm enabled: " << RENEGADE_WITH_X86_ASM << '\n'
        << "  stacktrace backend: std::stacktrace with log fallback\n"
        << "  renderer enabled: " << RENEGADE_WITH_DX8_RENDERER << '\n'
        << "  directinput enabled: " << RENEGADE_WITH_DIRECTINPUT << '\n'
        << "  commando slice: " << Renegade_Commando_Bootstrap_Summary() << '\n';
}

} // namespace

int main(int argc, char **argv)
{
    const bool smoke_test = HasArgument(argc, argv, "--headless-smoke");

    PrintCommandoBanner();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }

    const SDL_WindowFlags window_flags = smoke_test ? SDL_WINDOW_HIDDEN : SDL_WINDOW_RESIZABLE;
    SDL_Window *window = SDL_CreateWindow("Renegade", 1280, 720, window_flags);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    std::cout << "  SDL runtime platform: " << SDL_GetPlatform() << '\n';
    if (const char *video_driver = SDL_GetCurrentVideoDriver(); video_driver != nullptr) {
        std::cout << "  SDL video driver: " << video_driver << '\n';
    }

    if (smoke_test) {
        SDL_PumpEvents();
        SDL_Delay(16);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_SUCCESS;
    }

    std::cout << "Renegade window opened. Close the window to exit.\n";

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}