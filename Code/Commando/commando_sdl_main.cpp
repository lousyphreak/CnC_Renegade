#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "buildnum.h"
#include "commando_bootstrap_bridge.h"
#include "Except.h"
#include "../Combat/directinput.h"
#include "../Combat/input.h"
#include "gamemode.h"
#include "init.h"
#include "mainloop.h"
#include "msgloop.h"
#include "../ww3d2/ww3d.h"
#include "../wwui/dialogmgr.h"
#include "renegade_build_config.h"
#include "renegadedialogmgr.h"
#include "singletoninstancekeeper.h"
#include "useroptions.h"
#include "win.h"
#include "wwperfmon.h"

// disable leak detection
#ifdef __cplusplus
extern "C"
#endif
const char* __asan_default_options() { return "detect_leaks=0"; }

namespace {

struct CommandoAppState {
    bool smoke_test = false;
    bool perfmon_configured = false;
    bool thread_registered = false;
    bool handlers_installed = false;
    bool game_loop_initialized = false;
    int exit_code = EXIT_SUCCESS;
    SDL_Window *window = nullptr;
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

void PrintCommandoBanner()
{
    std::cout
        << "Renegade Commando executable\n"
        << "  platform target: " << RENEGADE_BOOTSTRAP_PLATFORM << '\n'
        << "  SDL version pin: " << RENEGADE_SDL3_VERSION << '\n'
        << "  x86 asm enabled: " << RENEGADE_WITH_X86_ASM << '\n'
        << "  stacktrace backend: std::stacktrace with log fallback\n"
        << "  dx8 renderer enabled: " << RENEGADE_WITH_DX8_RENDERER << '\n'
        << "  bgfx renderer enabled: " << RENEGADE_WITH_BGFX_RENDERER << '\n'
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

SDL_Window *Get_Main_Window()
{
    return reinterpret_cast<SDL_Window *>(MainWindow);
}

void Ensure_Window_Visible(SDL_Window *window)
{
    if (window == nullptr) {
        return;
    }

    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    SDL_SyncWindow(window);
    SDL_PumpEvents();
}

bool Query_Window_Pixel_Size(SDL_Window *window, int &pixel_width, int &pixel_height)
{
    pixel_width = 0;
    pixel_height = 0;

    if (window == nullptr) {
        return false;
    }

    if (!SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height) || pixel_width <= 0 || pixel_height <= 0) {
        if (!SDL_GetWindowSize(window, &pixel_width, &pixel_height) || pixel_width <= 0 || pixel_height <= 0) {
            return false;
        }
    }

    return true;
}

void Sync_Window_Size_To_Renderer(SDL_Window *window)
{
    int pixel_width = 0;
    int pixel_height = 0;
    if (!Query_Window_Pixel_Size(window, pixel_width, pixel_height)) {
        return;
    }

    const bool windowed = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == 0;

    if (WW3D::Is_Initted()) {
        int current_width = 0;
        int current_height = 0;
        int current_bits = 0;
        bool current_windowed = true;
        WW3D::Get_Device_Resolution(current_width, current_height, current_bits, current_windowed);
        if (current_width == pixel_width && current_height == pixel_height && current_windowed == windowed) {
            return;
        }
    }

    WW3D::Set_Device_Resolution(pixel_width, pixel_height, -1, windowed ? 1 : 0, false);
}

bool Is_Main_Window_Display_Event(const SDL_Event &event)
{
    SDL_Window *window = Get_Main_Window();
    if (window == nullptr) {
        return false;
    }

    const SDL_DisplayID main_display_id = SDL_GetDisplayForWindow(window);
    return main_display_id != 0 && event.display.displayID == main_display_id;
}

bool Is_Main_Window_Event(const SDL_Event &event)
{
    SDL_Window *window = Get_Main_Window();
    if (window == nullptr) {
        return false;
    }

    const SDL_WindowID main_window_id = SDL_GetWindowID(window);
    switch (event.type) {
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_WINDOW_HIT_TEST:
        case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
        case SDL_EVENT_WINDOW_OCCLUDED:
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        case SDL_EVENT_WINDOW_DESTROYED:
        case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
            return event.window.windowID == main_window_id;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            return event.key.windowID == main_window_id;

        case SDL_EVENT_TEXT_INPUT:
            return event.text.windowID == main_window_id;

        default:
            break;
    }

    return false;
}

void Handle_Window_Focus_Gained()
{
    GameInFocus = true;
    GameModeManager::Hide_Render_Frames(1);
}

void Handle_Window_Focus_Lost()
{
    // Keep the simulation/render loop active when the SDL window loses focus.
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

void Sync_Main_Window_State()
{
    Sync_Window_Size_To_Renderer(Get_Main_Window());
    Sync_Text_Input_State();
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

    for (const uint8_t *cursor = reinterpret_cast<const uint8_t *>(text); *cursor != 0; ++cursor) {
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

bool Handle_Window_Event(const SDL_Event &event)
{
    SDL_Window *window = Get_Main_Window();
    if (window == nullptr || !Is_Main_Window_Event(event)) {
        return false;
    }

    switch (event.type) {
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            Sync_Window_Size_To_Renderer(window);
            return true;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            Handle_Window_Focus_Gained();
            return true;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            Handle_Window_Focus_Lost();
            return true;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            Stop_Main_Loop(EXIT_SUCCESS);
            return true;

        default:
            break;
    }

    return false;
}

bool Handle_Display_Event(const SDL_Event &event)
{
    switch (event.type) {
        case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
        case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
        case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
        case SDL_EVENT_DISPLAY_USABLE_BOUNDS_CHANGED:
            break;

        default:
            return false;
    }

    if (!Is_Main_Window_Display_Event(event)) {
        return false;
    }

    Sync_Window_Size_To_Renderer(Get_Main_Window());
    return true;
}

bool Handle_System_Key_Event(const SDL_Event &event)
{
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return false;
    }

    if ((event.key.mod & SDL_KMOD_ALT) == 0) {
        return false;
    }

    if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
        WW3D::Toggle_Windowed();
        return true;
    }

    return false;
}

bool Handle_Main_Loop_Event(SDL_Event &event)
{
    if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_TERMINATING) {
        Stop_Main_Loop(EXIT_SUCCESS);
        return true;
    }

    if (Handle_Window_Event(event)) {
        return true;
    }

    if (Handle_Display_Event(event)) {
        return true;
    }

    if (Handle_System_Key_Event(event)) {
        return true;
    }

    Dispatch_Runtime_Event(event);

    return true;
}

SDL_AppResult Get_App_Result_From_Exit_Code(int exit_code)
{
    return exit_code == EXIT_SUCCESS ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
}

} // namespace

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    CommandoAppState *app = new CommandoAppState();
    *appstate = app;

    app->smoke_test = HasArgument(argc, argv, "--headless-smoke");

    PrintCommandoBanner();

    Set_Working_Directory_From_Executable(argv);

    // Let POSIX signal delivery keep its default semantics so tools like
    // `kill` can terminate the process immediately even if the game loop is
    // not pumping SDL events.
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

    const std::string command_line = Build_Command_Line(argc, argv);
    if (!cUserOptions::Parse_Command_Line(command_line.c_str())) {
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    if (!app->instance_keeper.Verify_Safe_To_Execute()) {
        app->exit_code = EXIT_SUCCESS;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    WWPerfMonClass::Configure_From_Environment();
    app->perfmon_configured = true;

    SDL_PropertiesID window_props = SDL_CreateProperties();
    if (window_props == 0) {
        std::cerr << "SDL_CreateProperties failed: " << SDL_GetError() << '\n';
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Renegade");
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, app->smoke_test);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, !app->smoke_test);
#ifdef __EMSCRIPTEN__
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_EMSCRIPTEN_CANVAS_ID_STRING, "#canvas");
#else
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
#endif

    app->window = SDL_CreateWindowWithProperties(window_props);
    SDL_DestroyProperties(window_props);
    if (app->window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    if (!app->smoke_test) {
        Ensure_Window_Visible(app->window);
    }

    std::cout << "  SDL runtime platform: " << SDL_GetPlatform() << '\n';
    if (const char *video_driver = SDL_GetCurrentVideoDriver(); video_driver != nullptr) {
        std::cout << "  SDL video driver: " << video_driver << '\n';
    }

    ProgramInstance = nullptr;
    MainWindow = reinterpret_cast<HWND>(app->window);
    GameInFocus = true;
    Sync_Window_Size_To_Renderer(app->window);

    if (app->smoke_test) {
        SDL_PumpEvents();
        SDL_Delay(16);
        app->exit_code = EXIT_SUCCESS;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    Register_Thread_ID(GetCurrentThreadId(), "Main Thread", true);
    app->thread_registered = true;
    Register_Application_Exception_Callback(&Application_Exception_Callback);
    Register_Application_Version_Callback(&BuildInfoClass::Composite_Build_Info);

    Message_Intercept_Handler = Handle_Main_Loop_Event;
    Message_Pre_Poll_Handler = Sync_Main_Window_State;
    app->handlers_installed = true;

    if (!Game_Main_Loop_Initialize()) {
        app->exit_code = Get_Main_Loop_Exit_Code();
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    app->game_loop_initialized = true;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    CommandoAppState *app = static_cast<CommandoAppState *>(appstate);
    if (app == nullptr) {
        return SDL_APP_FAILURE;
    }

    if (!app->game_loop_initialized) {
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    Sync_Main_Window_State();
    Game_Main_Loop_Iterate();

    if (!Is_Main_Loop_Running()) {
        app->exit_code = Get_Main_Loop_Exit_Code();
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    CommandoAppState *app = static_cast<CommandoAppState *>(appstate);
    if (app == nullptr || event == nullptr) {
        return SDL_APP_FAILURE;
    }

    if (!app->game_loop_initialized) {
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

    Handle_Main_Loop_Event(*event);
    if (Is_Main_Loop_Running()) {
        return SDL_APP_CONTINUE;
    }

    app->exit_code = Get_Main_Loop_Exit_Code();
    return Get_App_Result_From_Exit_Code(app->exit_code);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;

    CommandoAppState *app = static_cast<CommandoAppState *>(appstate);
    if (app == nullptr) {
        return;
    }

    if (app->game_loop_initialized) {
        app->exit_code = Game_Main_Loop_Shutdown();
        app->game_loop_initialized = false;
    }

    if (app->handlers_installed) {
        Message_Pre_Poll_Handler = nullptr;
        Message_Intercept_Handler = nullptr;
        app->handlers_installed = false;
    }

    if (app->thread_registered) {
        Unregister_Thread_ID(GetCurrentThreadId(), "Main Thread");
        app->thread_registered = false;
    }

    ProgramInstance = nullptr;
    MainWindow = nullptr;
    GameInFocus = false;

    if (app->perfmon_configured) {
        WWPerfMonClass::Shutdown();
        app->perfmon_configured = false;
    }

    if (app->window != nullptr) {
        SDL_DestroyWindow(app->window);
        app->window = nullptr;
    }

    delete app;
}
