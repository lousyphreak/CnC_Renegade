#include <SDL3/SDL.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "commando_bootstrap_bridge.h"
#include "../Combat/input.h"
#include "mainloop.h"
#include "msgloop.h"
#include "../wwui/dialogmgr.h"
#include "renegade_build_config.h"
#include "renegadedialogmgr.h"
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

SDL_Window *Get_Text_Input_Window()
{
    SDL_Window *window = SDL_GetKeyboardFocus();
    if (window == nullptr) {
        window = SDL_GetMouseFocus();
    }
    return window;
}

void Sync_Text_Input_State()
{
    SDL_Window *window = Get_Text_Input_Window();
    if (window == nullptr) {
        return;
    }

    const bool wants_text_input = Input::Is_Console_Enabled() || (DialogMgrClass::Get_Dialog_Count() > 0);
    const bool text_input_active = SDL_TextInputActive(window);

    if (wants_text_input && !text_input_active) {
        SDL_StartTextInput(window);
    } else if (!wants_text_input && text_input_active) {
        SDL_ClearComposition(window);
        SDL_StopTextInput(window);
    }
}

int Map_Console_Key(const SDL_Event &event)
{
    if (event.type != SDL_EVENT_KEY_DOWN) {
        return 0;
    }

    switch (event.key.key) {
        case SDLK_BACKSPACE:
            return 8;

        case SDLK_TAB:
            return 9;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return 13;

        case SDLK_ESCAPE:
            return 27;

        default:
            break;
    }

    return 0;
}

void Dispatch_Console_Text(const char *text)
{
    if (text == nullptr) {
        return;
    }

    for (const unsigned char *cursor = reinterpret_cast<const unsigned char *>(text); *cursor != 0; ++cursor) {
        if (*cursor < 0x80) {
            Input::Console_Add_Key(*cursor);
        }
    }
}

void Dispatch_Runtime_Event(const SDL_Event &event)
{
    if (Input::Is_Console_Enabled()) {
        if (const int console_key = Map_Console_Key(event); console_key != 0) {
            Input::Console_Add_Key(console_key);
            return;
        }

        if (event.type == SDL_EVENT_TEXT_INPUT) {
            Dispatch_Console_Text(event.text.text);
        }

        return;
    }

    if (_TheWWUIInput != nullptr) {
        _TheWWUIInput->ProcessEvent(event);
    }
}

bool Handle_Main_Loop_Event(SDL_Event &event)
{
    if (event.type == SDL_EVENT_QUIT) {
        Stop_Main_Loop(EXIT_SUCCESS);
        return true;
    }

    Dispatch_Runtime_Event(event);

    return true;
}

} // namespace

int main(int argc, char **argv)
{
    const bool smoke_test = HasArgument(argc, argv, "--headless-smoke");

    PrintCommandoBanner();

    Set_Working_Directory_From_Executable(argv);

    // Let POSIX signal delivery keep its default semantics so tools like
    // `kill` can terminate the process immediately even if the game loop is
    // not pumping SDL events.
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

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
    Message_Pre_Poll_Handler = Sync_Text_Input_State;
    const int exit_code = Game_Main_Loop();
    Message_Pre_Poll_Handler = nullptr;
    Message_Intercept_Handler = nullptr;

    MainWindow = nullptr;
    GameInFocus = false;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}