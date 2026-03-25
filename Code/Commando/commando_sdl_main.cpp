#include <SDL3/SDL.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "commando_bootstrap_bridge.h"
#include "mainloop.h"
#include "msgloop.h"
#include "renegade_build_config.h"
#include "singletoninstancekeeper.h"
#include "useroptions.h"
#include "win.h"

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
        << "  combat input backend: SDL3\n"
        << "  commando slice: " << Renegade_Commando_Bootstrap_Summary() << '\n';
}

void Set_Working_Directory_From_Executable(char **argv)
{
    if (argv == nullptr || argv[0] == nullptr || argv[0][0] == '\0') {
        return;
    }

    std::error_code error;
    const std::filesystem::path executable_path = std::filesystem::weakly_canonical(argv[0], error);
    const std::filesystem::path working_directory = error ? std::filesystem::path(argv[0]).parent_path() : executable_path.parent_path();
    if (working_directory.empty()) {
        return;
    }

    std::filesystem::current_path(working_directory, error);
}

std::string Build_Command_Line(int argc, char **argv)
{
    std::ostringstream command_line;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr || argv[i][0] == '\0') {
            continue;
        }

        if (command_line.tellp() > 0) {
            command_line << ' ';
        }

        command_line << argv[i];
    }

    return command_line.str();
}

bool Handle_Main_Loop_Event(SDL_Event &event)
{
    if (event.type == SDL_EVENT_QUIT) {
        Stop_Main_Loop(EXIT_SUCCESS);
        return true;
    }

    return false;
}

} // namespace

int main(int argc, char **argv)
{
    const bool smoke_test = HasArgument(argc, argv, "--headless-smoke");

    PrintCommandoBanner();

    Set_Working_Directory_From_Executable(argv);

    const std::string command_line = Build_Command_Line(argc, argv);
    if (!cUserOptions::Parse_Command_Line(command_line.c_str())) {
        return EXIT_FAILURE;
    }

    SingletonInstanceKeeperClass instance_keeper;
    if (!instance_keeper.Verify_Safe_To_Execute()) {
        return EXIT_SUCCESS;
    }

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

    ProgramInstance = nullptr;
    MainWindow = reinterpret_cast<HWND>(window);
    GameInFocus = true;

    if (smoke_test) {
        SDL_PumpEvents();
        SDL_Delay(16);
        MainWindow = nullptr;
        GameInFocus = false;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_SUCCESS;
    }

    Message_Intercept_Handler = Handle_Main_Loop_Event;
    const int exit_code = Game_Main_Loop();
    Message_Intercept_Handler = nullptr;

    MainWindow = nullptr;
    GameInFocus = false;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}