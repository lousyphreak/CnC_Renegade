#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "ConsoleMode.h"
#include "commando_bootstrap_bridge.h"
#include "mainloop.h"
#include "msgloop.h"
#include "renegade_build_config.h"
#include "ServerSettings.h"
#include "singletoninstancekeeper.h"
#include "slavemaster.h"
#include "useroptions.h"
#include "../wwlib/osdep.h"

// disable leak detection
#ifdef __cplusplus
extern "C"
#endif
const char* __asan_default_options() { return "detect_leaks=0"; }

namespace {

struct CommandoDedicatedAppState {
    bool smoke_test = false;
    bool message_handler_installed = false;
    bool game_loop_initialized = false;
    int exit_code = EXIT_SUCCESS;
    SingletonInstanceKeeperClass instance_keeper;
};

bool HasArgument(int argc, char **argv, std::string_view needle)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && needle == argv[i]) {
            return true;
        }
    }

    return false;
}

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

bool StartsWithServerConfigPrefix(const std::string &argument)
{
    return argument.rfind("STARTSERVER=", 0) == 0
        || argument.rfind("/STARTSERVER=", 0) == 0
        || argument.rfind("--STARTSERVER=", 0) == 0
        || argument.rfind("--startserver=", 0) == 0
        || argument.rfind("--server-config=", 0) == 0;
}

std::string Detect_Server_Config_Path(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }

        const std::string argument = argv[i];
        if (StartsWithServerConfigPrefix(argument)) {
            const std::size_t delimiter = argument.find('=');
            if (delimiter != std::string::npos && delimiter + 1 < argument.size()) {
                return argument.substr(delimiter + 1);
            }
        }

        if ((argument == "--server-config" || argument == "--startserver") && (i + 1) < argc && argv[i + 1] != nullptr) {
            return argv[i + 1];
        }
    }

    return "server.ini";
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

void Apply_Dedicated_Defaults()
{
    ConsoleBox.Set_Exclusive(true);

    if (!SlaveMaster.Am_I_Slave() && !ServerSettingsClass::Is_Server_Settings_File_Set()) {
        char default_settings[] = "STARTSERVER=server.ini";
        cUserOptions::Set_Server_INI_File(default_settings);
    }
}

bool Handle_Dedicated_Main_Loop_Event(SDL_Event &event)
{
    if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_TERMINATING) {
        Stop_Main_Loop(EXIT_SUCCESS);
        return true;
    }

    return true;
}

SDL_AppResult Get_App_Result_From_Exit_Code(int exit_code)
{
    return exit_code == EXIT_SUCCESS ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
}

} // namespace

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    CommandoDedicatedAppState *app = new CommandoDedicatedAppState();
    *appstate = app;

    app->smoke_test = HasArgument(argc, argv, "--headless-smoke");

    PrintDedicatedBanner();

    Set_Working_Directory_From_Executable(argv);

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

    const std::string command_line = Build_Command_Line(argc, argv);
    if (!cUserOptions::Parse_Command_Line(command_line.c_str())) {
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    Apply_Dedicated_Defaults();

    if (!app->instance_keeper.Verify_Safe_To_Execute()) {
        app->exit_code = EXIT_SUCCESS;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    const std::string server_config = ServerSettingsClass::Is_Server_Settings_File_Set()
        ? std::string(ServerSettingsClass::Get_Settings_File_Name())
        : Detect_Server_Config_Path(argc, argv);

    if (!app->smoke_test && !renegade_osdep::Path_Exists(server_config)) {
        std::cerr
            << "Dedicated server configuration file not found: " << server_config << '\n'
            << "Provide STARTSERVER=<file>, /STARTSERVER=<file>, --startserver=<file>, or --server-config <file>.\n";
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    std::cout << "Dedicated server mode enabled (FREEDEDICATEDSERVER).\n";
    std::cout << "  exclusive console mode: on\n";
    std::cout << "  resolved server config: " << server_config << '\n';

    if (!SDL_Init(SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    Message_Intercept_Handler = Handle_Dedicated_Main_Loop_Event;
    app->message_handler_installed = true;

    if (app->smoke_test) {
        SDL_PumpEvents();
        SDL_Delay(16);
        std::cout << "  headless smoke: success\n";
        app->exit_code = EXIT_SUCCESS;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    if (!Game_Main_Loop_Initialize()) {
        app->exit_code = Get_Main_Loop_Exit_Code();
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    app->game_loop_initialized = true;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    CommandoDedicatedAppState *app = static_cast<CommandoDedicatedAppState *>(appstate);
    if (app == nullptr) {
        return SDL_APP_FAILURE;
    }

    if (!app->game_loop_initialized) {
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    Game_Main_Loop_Iterate();

    if (!Is_Main_Loop_Running()) {
        app->exit_code = Get_Main_Loop_Exit_Code();
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    CommandoDedicatedAppState *app = static_cast<CommandoDedicatedAppState *>(appstate);
    if (app == nullptr || event == nullptr) {
        return SDL_APP_FAILURE;
    }

    if (!app->game_loop_initialized) {
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    Handle_Dedicated_Main_Loop_Event(*event);
    if (Is_Main_Loop_Running()) {
        return SDL_APP_CONTINUE;
    }

    app->exit_code = Get_Main_Loop_Exit_Code();
    return Get_App_Result_From_Exit_Code(app->exit_code);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;

    CommandoDedicatedAppState *app = static_cast<CommandoDedicatedAppState *>(appstate);
    if (app == nullptr) {
        return;
    }

    if (app->game_loop_initialized) {
        app->exit_code = Game_Main_Loop_Shutdown();
        app->game_loop_initialized = false;
    }

    if (app->message_handler_installed) {
        Message_Intercept_Handler = nullptr;
        app->message_handler_installed = false;
    }

    delete app;
}
