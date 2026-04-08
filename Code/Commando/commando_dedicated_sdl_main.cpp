#include <SDL3/SDL.h>

#include <iostream>

#include "commando_bootstrap_bridge.h"
#include "commando_dedicated_bootstrap.h"
#include "renegade_build_config.h"

namespace {

void PrintDedicatedBanner()
{
    std::cout
        << "Renegade dedicated server executable\n"
        << "  platform target: " << RENEGADE_BOOTSTRAP_PLATFORM << '\n'
        << "  SDL version pin: " << RENEGADE_SDL3_VERSION << '\n'
        << "  x86 asm enabled: " << RENEGADE_WITH_X86_ASM << '\n'
        << "  stacktrace backend: std::stacktrace with log fallback\n"
        << "  dx8 renderer enabled: " << RENEGADE_WITH_DX8_RENDERER << '\n'
        << "  combat input backend: SDL3\n"
        << "  dedicated build define: 1\n"
        << "  commando slice: " << Renegade_Commando_Bootstrap_Summary() << '\n'
        << "  SDL runtime platform: " << SDL_GetPlatform() << '\n'
        << "  video/audio init: skipped for dedicated mode\n";
}

} // namespace

int main(int argc, char **argv)
{
    PrintDedicatedBanner();
    return Renegade_Dedicated_Bootstrap(argc, argv);
}