#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cctype>
#include <cstdlib>
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
#include "../ww3d2/bgfxrenderer.h"
#include "../ww3d2/ww3d.h"
#include "../wwui/dialogmgr.h"
#include "renegade_build_config.h"
#include "renegadedialogmgr.h"
#include "singletoninstancekeeper.h"
#include "useroptions.h"
#include "win.h"
#include "wwperfmon.h"
#include "../compat/dinput.h"

// disable leak detection
#ifdef __cplusplus
extern "C"
#endif
const char* __asan_default_options() { return "detect_leaks=0"; }

extern void _Force_Link_RenegadePlayerTerminal(void);

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

constexpr std::string_view kGameDataDirectorySwitches[] = {
    "--game-data-directory",
    "--game-data-dir",
    "--gamedata-directory",
    "/GAMEDATADIRECTORY",
    "GAMEDATADIRECTORY",
};

char ToLowerAscii(char ch)
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (ToLowerAscii(lhs[i]) != ToLowerAscii(rhs[i])) {
            return false;
        }
    }

    return true;
}

bool StartsWithIgnoreCase(std::string_view value, std::string_view prefix)
{
    return value.size() >= prefix.size() && EqualsIgnoreCase(value.substr(0, prefix.size()), prefix);
}

int ConsumeGameDataDirectoryArgument(int argc, char **argv, int argument_index, std::string &value)
{
    if (argv == nullptr || argument_index <= 0 || argument_index >= argc || argv[argument_index] == nullptr) {
        return 0;
    }

    const std::string_view argument(argv[argument_index]);
    for (const std::string_view option : kGameDataDirectorySwitches) {
        if (EqualsIgnoreCase(argument, option)) {
            if ((argument_index + 1) >= argc || argv[argument_index + 1] == nullptr || argv[argument_index + 1][0] == '\0') {
                std::cerr << "Game data directory option requires a folder path.\n";
                return -1;
            }

            value = argv[argument_index + 1];
            return 2;
        }

        if (argument.size() > option.size() && argument[option.size()] == '=' && StartsWithIgnoreCase(argument, option)) {
            value.assign(argument.substr(option.size() + 1));
            if (value.empty()) {
                std::cerr << "Game data directory option requires a folder path.\n";
                return -1;
            }

            return 1;
        }
    }

    return 0;
}

bool ParseGameDataDirectoryArguments(int argc, char **argv)
{
    cUserOptions::Set_Game_Data_Directory(NULL);

    for (int i = 1; i < argc; ++i) {
        std::string value;
        const int consumed_arguments = ConsumeGameDataDirectoryArgument(argc, argv, i, value);
        if (consumed_arguments < 0) {
            return false;
        }
        if (consumed_arguments == 0) {
            continue;
        }

        cUserOptions::Set_Game_Data_Directory(value.c_str());
        i += consumed_arguments - 1;
    }

    return true;
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

void Force_Link_Client_UI()
{
    _Force_Link_RenegadePlayerTerminal();
}

std::string Build_Command_Line(int argc, char **argv)
{
    std::ostringstream command_line;

    for (int i = 1; i < argc; ++i) {
        std::string ignored_game_data_path;
        const int consumed_game_data_arguments = ConsumeGameDataDirectoryArgument(argc, argv, i, ignored_game_data_path);
        if (consumed_game_data_arguments > 0) {
            i += consumed_game_data_arguments - 1;
            continue;
        }

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

bool Is_Runtime_Platform(std::string_view platform_name)
{
    const char *platform = SDL_GetPlatform();
    return platform != nullptr && platform_name == platform;
}

bool Is_Current_Video_Driver(std::string_view driver_name)
{
    const char *video_driver = SDL_GetCurrentVideoDriver();
    return video_driver != nullptr && driver_name == video_driver;
}

void Configure_Renderer_Window_Properties(SDL_PropertiesID window_props)
{
    bgfx::RendererType::Enum requested_renderer = BgfxRenderer::Get_Requested_Renderer();
    if (requested_renderer == bgfx::RendererType::Count && Is_Runtime_Platform("Emscripten")) {
        requested_renderer = bgfx::RendererType::OpenGLES;
    }

    std::cout << "  bgfx renderer request: " << BgfxRenderer::Get_Requested_Renderer_Name() << '\n';
    SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "0");
    if (requested_renderer == bgfx::RendererType::Count) {
        return;
    }

    if (requested_renderer == bgfx::RendererType::Vulkan) {
        SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
        return;
    }

    if (requested_renderer == bgfx::RendererType::Metal) {
        SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_METAL_BOOLEAN, true);
        return;
    }

    const bool needs_opengl_window = requested_renderer == bgfx::RendererType::OpenGL
        || requested_renderer == bgfx::RendererType::OpenGLES;
    if (needs_opengl_window) {
        if (requested_renderer == bgfx::RendererType::OpenGLES && Is_Runtime_Platform("Emscripten")) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        }

        // bgfx uses EGL on Linux, so X11 needs an EGL-compatible SDL visual.
        // Wayland is the exception because bgfx creates its own wl_egl_window.
        if (Is_Runtime_Platform("Linux") && Is_Current_Video_Driver("wayland")) {
            return;
        }

        if (Is_Runtime_Platform("Linux") && Is_Current_Video_Driver("x11")) {
            SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "1");
        }

        SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    }
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

bool Handle_Console_Key_Event(const SDL_Event &event)
{
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat || Input::Is_Console_Enabled()) {
        return false;
    }

    const int primary = Input::Get_Primary_Key_For_Function(INPUT_FUNCTION_BEGIN_CONSOLE);
    const int secondary = Input::Get_Secondary_Key_For_Function(INPUT_FUNCTION_BEGIN_CONSOLE);
    const SDL_Scancode scancode = event.key.scancode;
    const bool matches_primary =
        (scancode == SDL_SCANCODE_F8 && primary == DIK_F8) ||
        (scancode == SDL_SCANCODE_GRAVE && primary == DIK_GRAVE);
    const bool matches_secondary =
        (scancode == SDL_SCANCODE_F8 && secondary == DIK_F8) ||
        (scancode == SDL_SCANCODE_GRAVE && secondary == DIK_GRAVE);

    if (!matches_primary && !matches_secondary) {
        return false;
    }

    Input::Request_Begin_Console();
    return true;
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

    Handle_Console_Key_Event(event);
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
    Force_Link_Client_UI();

    if (!ParseGameDataDirectoryArguments(argc, argv)) {
        app->exit_code = EXIT_FAILURE;
        return Get_App_Result_From_Exit_Code(app->exit_code);
    }

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
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
    Configure_Renderer_Window_Properties(window_props);
#ifdef __EMSCRIPTEN__
    SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_EMSCRIPTEN_CANVAS_ID_STRING, "#canvas");
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
