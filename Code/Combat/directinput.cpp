/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/directinput.cpp                       $*
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 1/15/02 5:32p                                               $*
 *                                                                                             *
 *                    $Revision:: 25                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "directinput.h"

#include "debug.h"
#include "dinput.h"
#include "timemgr.h"
#include "win.h"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstring>
#include <mutex>

namespace {

struct KeyboardMapping {
	int DIk;
	SDL_Scancode Scancode;
};

#define MAP_KEY(dik, scancode) { dik, SDL_SCANCODE_##scancode }

static const KeyboardMapping KeyboardMappings[] = {
	MAP_KEY(DIK_ESCAPE, ESCAPE),
	MAP_KEY(DIK_1, 1),
	MAP_KEY(DIK_2, 2),
	MAP_KEY(DIK_3, 3),
	MAP_KEY(DIK_4, 4),
	MAP_KEY(DIK_5, 5),
	MAP_KEY(DIK_6, 6),
	MAP_KEY(DIK_7, 7),
	MAP_KEY(DIK_8, 8),
	MAP_KEY(DIK_9, 9),
	MAP_KEY(DIK_0, 0),
	MAP_KEY(DIK_MINUS, MINUS),
	MAP_KEY(DIK_EQUALS, EQUALS),
	MAP_KEY(DIK_BACK, BACKSPACE),
	MAP_KEY(DIK_TAB, TAB),
	MAP_KEY(DIK_Q, Q),
	MAP_KEY(DIK_W, W),
	MAP_KEY(DIK_E, E),
	MAP_KEY(DIK_R, R),
	MAP_KEY(DIK_T, T),
	MAP_KEY(DIK_Y, Y),
	MAP_KEY(DIK_U, U),
	MAP_KEY(DIK_I, I),
	MAP_KEY(DIK_O, O),
	MAP_KEY(DIK_P, P),
	MAP_KEY(DIK_LBRACKET, LEFTBRACKET),
	MAP_KEY(DIK_RBRACKET, RIGHTBRACKET),
	MAP_KEY(DIK_RETURN, RETURN),
	MAP_KEY(DIK_LCONTROL, LCTRL),
	MAP_KEY(DIK_A, A),
	MAP_KEY(DIK_S, S),
	MAP_KEY(DIK_D, D),
	MAP_KEY(DIK_F, F),
	MAP_KEY(DIK_G, G),
	MAP_KEY(DIK_H, H),
	MAP_KEY(DIK_J, J),
	MAP_KEY(DIK_K, K),
	MAP_KEY(DIK_L, L),
	MAP_KEY(DIK_SEMICOLON, SEMICOLON),
	MAP_KEY(DIK_APOSTROPHE, APOSTROPHE),
	MAP_KEY(DIK_GRAVE, GRAVE),
	MAP_KEY(DIK_LSHIFT, LSHIFT),
	MAP_KEY(DIK_BACKSLASH, BACKSLASH),
	MAP_KEY(DIK_Z, Z),
	MAP_KEY(DIK_X, X),
	MAP_KEY(DIK_C, C),
	MAP_KEY(DIK_V, V),
	MAP_KEY(DIK_B, B),
	MAP_KEY(DIK_N, N),
	MAP_KEY(DIK_M, M),
	MAP_KEY(DIK_COMMA, COMMA),
	MAP_KEY(DIK_PERIOD, PERIOD),
	MAP_KEY(DIK_SLASH, SLASH),
	MAP_KEY(DIK_RSHIFT, RSHIFT),
	MAP_KEY(DIK_MULTIPLY, KP_MULTIPLY),
	MAP_KEY(DIK_LALT, LALT),
	MAP_KEY(DIK_SPACE, SPACE),
	MAP_KEY(DIK_CAPITAL, CAPSLOCK),
	MAP_KEY(DIK_F1, F1),
	MAP_KEY(DIK_F2, F2),
	MAP_KEY(DIK_F3, F3),
	MAP_KEY(DIK_F4, F4),
	MAP_KEY(DIK_F5, F5),
	MAP_KEY(DIK_F6, F6),
	MAP_KEY(DIK_F7, F7),
	MAP_KEY(DIK_F8, F8),
	MAP_KEY(DIK_F9, F9),
	MAP_KEY(DIK_F10, F10),
	MAP_KEY(DIK_NUMLOCK, NUMLOCKCLEAR),
	MAP_KEY(DIK_SCROLL, SCROLLLOCK),
	MAP_KEY(DIK_NUMPAD7, KP_7),
	MAP_KEY(DIK_NUMPAD8, KP_8),
	MAP_KEY(DIK_NUMPAD9, KP_9),
	MAP_KEY(DIK_SUBTRACT, KP_MINUS),
	MAP_KEY(DIK_NUMPAD4, KP_4),
	MAP_KEY(DIK_NUMPAD5, KP_5),
	MAP_KEY(DIK_NUMPAD6, KP_6),
	MAP_KEY(DIK_ADD, KP_PLUS),
	MAP_KEY(DIK_NUMPAD1, KP_1),
	MAP_KEY(DIK_NUMPAD2, KP_2),
	MAP_KEY(DIK_NUMPAD3, KP_3),
	MAP_KEY(DIK_NUMPAD0, KP_0),
	MAP_KEY(DIK_DECIMAL, KP_PERIOD),
	MAP_KEY(DIK_F11, F11),
	MAP_KEY(DIK_F12, F12),
	MAP_KEY(DIK_F13, F13),
	MAP_KEY(DIK_F14, F14),
	MAP_KEY(DIK_F15, F15),
	MAP_KEY(DIK_NUMPADENTER, KP_ENTER),
	MAP_KEY(DIK_RCONTROL, RCTRL),
	MAP_KEY(DIK_DIVIDE, KP_DIVIDE),
	MAP_KEY(DIK_SYSRQ, PRINTSCREEN),
	MAP_KEY(DIK_RALT, RALT),
	MAP_KEY(DIK_HOME, HOME),
	MAP_KEY(DIK_UP, UP),
	MAP_KEY(DIK_PRIOR, PAGEUP),
	MAP_KEY(DIK_LEFT, LEFT),
	MAP_KEY(DIK_RIGHT, RIGHT),
	MAP_KEY(DIK_END, END),
	MAP_KEY(DIK_DOWN, DOWN),
	MAP_KEY(DIK_NEXT, PAGEDOWN),
	MAP_KEY(DIK_INSERT, INSERT),
	MAP_KEY(DIK_DELETE, DELETE),
	MAP_KEY(DIK_LWIN, LGUI),
	MAP_KEY(DIK_RWIN, RGUI),
	MAP_KEY(DIK_APPS, APPLICATION),
};

#undef MAP_KEY

std::mutex InputMutex;
bool KeyboardHeld[DirectInput::NUM_KEYBOARD_BUTTONS] = {};
bool KeyboardPressed[DirectInput::NUM_KEYBOARD_BUTTONS] = {};
bool KeyboardReleased[DirectInput::NUM_KEYBOARD_BUTTONS] = {};
bool MouseHeld[DirectInput::NUM_MOUSE_BUTTONS] = {};
bool MousePressed[DirectInput::NUM_MOUSE_BUTTONS] = {};
bool MouseReleased[DirectInput::NUM_MOUSE_BUTTONS] = {};
bool JoystickHeld[DirectInput::NUM_JOYSTICK_BUTTONS] = {};
int32_t PendingMouseAxis[DirectInput::NUM_MOUSE_AXIS] = {};
Vector3 PendingCursorPos(0.0f, 0.0f, 0.0f);
int PendingLastKeyPressed = 0;
SDL_Gamepad *ActiveGamepad = NULL;
bool EventWatchInstalled = false;
bool CapturedState = false;

constexpr char BUTTON_BIT_DOUBLE = 8;
constexpr float BUTTON_DOUBLE_THRESHHOLD = 0.25f;

char Build_Button_State(bool held, bool hit, bool released)
{
	char state = 0;
	if (held) {
		state |= DirectInput::DI_BUTTON_HELD;
	}
	if (hit) {
		state |= DirectInput::DI_BUTTON_HIT;
	}
	if (released) {
		state |= DirectInput::DI_BUTTON_RELEASED;
	}
	return state;
}

int SDL_Mouse_Button_To_Index(Uint8 button)
{
	switch (button) {
		case SDL_BUTTON_LEFT:
			return 0;
		case SDL_BUTTON_RIGHT:
			return 1;
		case SDL_BUTTON_MIDDLE:
			return 2;
		default:
			return -1;
	}
}

int SDL_To_DIK(SDL_Scancode scancode)
{
	for (const KeyboardMapping &mapping : KeyboardMappings) {
		if (mapping.Scancode == scancode) {
			return mapping.DIk;
		}
	}

	return 0;
}

void Flush_Locked()
{
	std::memset(KeyboardHeld, 0, sizeof(KeyboardHeld));
	std::memset(KeyboardPressed, 0, sizeof(KeyboardPressed));
	std::memset(KeyboardReleased, 0, sizeof(KeyboardReleased));
	std::memset(MouseHeld, 0, sizeof(MouseHeld));
	std::memset(MousePressed, 0, sizeof(MousePressed));
	std::memset(MouseReleased, 0, sizeof(MouseReleased));
	std::memset(JoystickHeld, 0, sizeof(JoystickHeld));
	std::memset(PendingMouseAxis, 0, sizeof(PendingMouseAxis));
	PendingCursorPos.Set(0.0f, 0.0f, 0.0f);
	PendingLastKeyPressed = 0;
}

void Close_Gamepad()
{
	if (ActiveGamepad != NULL) {
		SDL_CloseGamepad(ActiveGamepad);
		ActiveGamepad = NULL;
	}
}

void Ensure_Gamepad()
{
	if (ActiveGamepad != NULL && SDL_GamepadConnected(ActiveGamepad)) {
		return;
	}

	Close_Gamepad();

	int gamepad_count = 0;
	SDL_JoystickID *gamepads = SDL_GetGamepads(&gamepad_count);
	if (gamepads != NULL && gamepad_count > 0) {
		ActiveGamepad = SDL_OpenGamepad(gamepads[0]);
	}
	SDL_free(gamepads);
}

SDL_Window *Get_Input_Window()
{
	SDL_Window *window = SDL_GetKeyboardFocus();
	if (window == NULL) {
		window = SDL_GetMouseFocus();
	}
	return window;
}

void Convert_Window_Coordinates_To_Render(float x, float y, float &render_x, float &render_y)
{
	render_x = x;
	render_y = y;

	SDL_Window *window = Get_Input_Window();
	if (window == NULL) {
		return;
	}

	int window_width = 0;
	int window_height = 0;
	if (!SDL_GetWindowSize(window, &window_width, &window_height) || window_width <= 0 || window_height <= 0) {
		return;
	}

	int pixel_width = 0;
	int pixel_height = 0;
	if (!SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height) || pixel_width <= 0 || pixel_height <= 0) {
		pixel_width = window_width;
		pixel_height = window_height;
	}

	render_x = x * static_cast<float>(pixel_width) / static_cast<float>(window_width);
	render_y = y * static_cast<float>(pixel_height) / static_cast<float>(window_height);
}

void Convert_Render_Coordinates_To_Window(float x, float y, float &window_x, float &window_y)
{
	window_x = x;
	window_y = y;

	SDL_Window *window = Get_Input_Window();
	if (window == NULL) {
		return;
	}

	int window_width = 0;
	int window_height = 0;
	if (!SDL_GetWindowSize(window, &window_width, &window_height) || window_width <= 0 || window_height <= 0) {
		return;
	}

	int pixel_width = 0;
	int pixel_height = 0;
	if (!SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height) || pixel_width <= 0 || pixel_height <= 0) {
		pixel_width = window_width;
		pixel_height = window_height;
	}

	window_x = x * static_cast<float>(window_width) / static_cast<float>(pixel_width);
	window_y = y * static_cast<float>(window_height) / static_cast<float>(pixel_height);
}

void Set_Pending_Cursor_Position(float x, float y)
{
	Convert_Window_Coordinates_To_Render(x, y, PendingCursorPos.X, PendingCursorPos.Y);
}

bool SDLCALL DirectInput_Event_Watch(void *, SDL_Event *event)
{
	std::lock_guard<std::mutex> lock(InputMutex);

	switch (event->type) {
		case SDL_EVENT_WILL_ENTER_FOREGROUND:
		case SDL_EVENT_DID_ENTER_FOREGROUND:
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			GameInFocus = true;
			break;

		case SDL_EVENT_WILL_ENTER_BACKGROUND:
		case SDL_EVENT_DID_ENTER_BACKGROUND:
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			break;

		case SDL_EVENT_KEY_DOWN:
			if (!CapturedState || event->key.repeat) {
				break;
			}
			if (const int dik = SDL_To_DIK(event->key.scancode); dik > 0 && dik < DirectInput::NUM_KEYBOARD_BUTTONS) {
				if (!KeyboardHeld[dik]) {
					KeyboardPressed[dik] = true;
				}
				KeyboardHeld[dik] = true;
				PendingLastKeyPressed = dik;
			}
			break;

		case SDL_EVENT_KEY_UP:
			if (!CapturedState) {
				break;
			}
			if (const int dik = SDL_To_DIK(event->key.scancode); dik > 0 && dik < DirectInput::NUM_KEYBOARD_BUTTONS) {
				if (KeyboardHeld[dik]) {
					KeyboardReleased[dik] = true;
				}
				KeyboardHeld[dik] = false;
			}
			break;

		case SDL_EVENT_MOUSE_MOTION:
			if (!CapturedState) {
				Set_Pending_Cursor_Position(event->motion.x, event->motion.y);
				break;
			}
			PendingMouseAxis[DirectInput::MOUSE_X_AXIS] += static_cast<int32_t>(std::lround(event->motion.xrel));
			PendingMouseAxis[DirectInput::MOUSE_Y_AXIS] += static_cast<int32_t>(std::lround(event->motion.yrel));
			// WWUI hit-testing/rendering uses the drawable resolution even while
			// gameplay input remains driven by relative mouse axes.
			Set_Pending_Cursor_Position(event->motion.x, event->motion.y);
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (!CapturedState) {
				break;
			}
			if (const int index = SDL_Mouse_Button_To_Index(event->button.button); index >= 0) {
				if (!MouseHeld[index]) {
					MousePressed[index] = true;
				}
				MouseHeld[index] = true;
				Set_Pending_Cursor_Position(event->button.x, event->button.y);
			}
			break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (!CapturedState) {
				break;
			}
			if (const int index = SDL_Mouse_Button_To_Index(event->button.button); index >= 0) {
				if (MouseHeld[index]) {
					MouseReleased[index] = true;
				}
				MouseHeld[index] = false;
				Set_Pending_Cursor_Position(event->button.x, event->button.y);
			}
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			if (!CapturedState) {
				break;
			}
			{
				int scroll = event->wheel.integer_y;
				if (scroll == 0) {
					scroll = static_cast<int>(std::lround(event->wheel.y));
				}
				if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
					scroll = -scroll;
				}
				PendingMouseAxis[DirectInput::MOUSE_Z_AXIS] += scroll;
			}
			break;

		default:
			break;
	}

	return true;
}

} // namespace

char	DirectInput::DIKeyboardButtons[NUM_KEYBOARD_BUTTONS];
char	DirectInput::DIMouseButtons[NUM_MOUSE_BUTTONS];
int32_t	DirectInput::DIMouseAxis[NUM_MOUSE_AXIS];
char	DirectInput::DIJoystickButtons[NUM_JOYSTICK_BUTTONS];
int32_t	DirectInput::DIJoystickAxis[2];
float	DirectInput::ButtonLastHitTime[NUM_KEYBOARD_BUTTONS];
Vector3	DirectInput::CursorPos(0, 0, 0);
bool	DirectInput::EatMouseHeld = false;
bool	DirectInput::Captured = false;
int	DirectInput::LastKeyPressed = 0;

void DirectInput::Init( void )
{
	WWDEBUG_SAY(("DirectInput: Init\n"));

	if (!EventWatchInstalled) {
		EventWatchInstalled = SDL_AddEventWatch(DirectInput_Event_Watch, NULL);
	}

	{
		std::lock_guard<std::mutex> lock(InputMutex);
		Flush_Locked();
		std::memset(DIKeyboardButtons, 0, sizeof(DIKeyboardButtons));
		std::memset(DIMouseButtons, 0, sizeof(DIMouseButtons));
		std::memset(DIMouseAxis, 0, sizeof(DIMouseAxis));
		std::memset(DIJoystickButtons, 0, sizeof(DIJoystickButtons));
		std::memset(DIJoystickAxis, 0, sizeof(DIJoystickAxis));
		LastKeyPressed = 0;
		EatMouseHeld = false;
		for (int index = 0; index < NUM_KEYBOARD_BUTTONS; ++index) {
			ButtonLastHitTime[index] = 1000.0f;
		}
	}

	GameInFocus = (MainWindow != NULL);
	Acquire();
}

void DirectInput::Shutdown( void )
{
	WWDEBUG_SAY(("DirectInput: Shutdown\n"));

	Unacquire();
	Close_Gamepad();

	if (EventWatchInstalled) {
		SDL_RemoveEventWatch(DirectInput_Event_Watch, NULL);
		EventWatchInstalled = false;
	}

	Flush();
}

void DirectInput::Flush( void )
{
	std::lock_guard<std::mutex> lock(InputMutex);
	Flush_Locked();
	std::memset(DIKeyboardButtons, 0, sizeof(DIKeyboardButtons));
	std::memset(DIMouseButtons, 0, sizeof(DIMouseButtons));
	std::memset(DIMouseAxis, 0, sizeof(DIMouseAxis));
	std::memset(DIJoystickButtons, 0, sizeof(DIJoystickButtons));
	std::memset(DIJoystickAxis, 0, sizeof(DIJoystickAxis));
	LastKeyPressed = 0;
	EatMouseHeld = false;
}

void DirectInput::Acquire(void)
{
	if (Captured) {
		return;
	}

	Flush();

	float mouse_x = 0.0f;
	float mouse_y = 0.0f;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	Convert_Window_Coordinates_To_Render(mouse_x, mouse_y, CursorPos.X, CursorPos.Y);
	CursorPos.Z = 0.0f;

	{
		std::lock_guard<std::mutex> lock(InputMutex);
		PendingCursorPos = CursorPos;
	}

	if (SDL_Window *window = Get_Input_Window()) {
		SDL_SetWindowRelativeMouseMode(window, true);
	}
	Captured = true;
	CapturedState = true;
}

void DirectInput::Unacquire(void)
{
	if (!Captured) {
		return;
	}

	Captured = false;
	CapturedState = false;
	if (SDL_Window *window = Get_Input_Window()) {
		SDL_SetWindowRelativeMouseMode(window, false);
	}

	if (SDL_Window *window = Get_Input_Window()) {
		float window_x = CursorPos.X;
		float window_y = CursorPos.Y;
		Convert_Render_Coordinates_To_Window(CursorPos.X, CursorPos.Y, window_x, window_y);
		SDL_WarpMouseInWindow(window, window_x, window_y);
	}

	Flush();
}

void DirectInput::ReadKeyboard( void )
{
	std::lock_guard<std::mutex> lock(InputMutex);

	for (int index = 0; index < NUM_KEYBOARD_BUTTONS; ++index) {
		DIKeyboardButtons[index] = Build_Button_State(KeyboardHeld[index], KeyboardPressed[index], KeyboardReleased[index]);
		KeyboardPressed[index] = false;
		KeyboardReleased[index] = false;
	}

	DIKeyboardButtons[DIK_CONTROL] = DIKeyboardButtons[DIK_LCONTROL] | DIKeyboardButtons[DIK_RCONTROL];
	DIKeyboardButtons[DIK_SHIFT] = DIKeyboardButtons[DIK_LSHIFT] | DIKeyboardButtons[DIK_RSHIFT];
	DIKeyboardButtons[DIK_ALT] = DIKeyboardButtons[DIK_LALT] | DIKeyboardButtons[DIK_RALT];
	DIKeyboardButtons[DIK_WIN] = DIKeyboardButtons[DIK_LWIN] | DIKeyboardButtons[DIK_RWIN];

	LastKeyPressed = PendingLastKeyPressed;
	PendingLastKeyPressed = 0;
}

void DirectInput::ReadMouse( void )
{
	std::lock_guard<std::mutex> lock(InputMutex);

	for (int index = 0; index < NUM_MOUSE_BUTTONS; ++index) {
		DIMouseButtons[index] = Build_Button_State(MouseHeld[index], MousePressed[index], MouseReleased[index]);
		MousePressed[index] = false;
		MouseReleased[index] = false;
	}

	for (int axis = 0; axis < NUM_MOUSE_AXIS; ++axis) {
		DIMouseAxis[axis] = PendingMouseAxis[axis];
		PendingMouseAxis[axis] = 0;
	}

	CursorPos = PendingCursorPos;

	if (EatMouseHeld) {
		DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] &= ~DI_BUTTON_HELD;
		DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] &= ~DI_BUTTON_HIT;
		DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] |= DI_BUTTON_RELEASED;
	}

	if (DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] & DI_BUTTON_RELEASED) {
		EatMouseHeld = false;
	}
}

void DirectInput::ReadJoystick( void )
{
	Ensure_Gamepad();

	bool button_a_down = false;
	bool button_b_down = false;
	int32_t axis_x = 0;
	int32_t axis_y = 0;

	if (ActiveGamepad != NULL) {
		button_a_down = SDL_GetGamepadButton(ActiveGamepad, SDL_GAMEPAD_BUTTON_SOUTH);
		button_b_down = SDL_GetGamepadButton(ActiveGamepad, SDL_GAMEPAD_BUTTON_EAST);
		axis_x = static_cast<int32_t>(std::lround((SDL_GetGamepadAxis(ActiveGamepad, SDL_GAMEPAD_AXIS_LEFTX) * 1000.0f) / 32767.0f));
		axis_y = static_cast<int32_t>(std::lround((SDL_GetGamepadAxis(ActiveGamepad, SDL_GAMEPAD_AXIS_LEFTY) * 1000.0f) / 32767.0f));
	}

	DIJoystickButtons[0] = Build_Button_State(button_a_down, button_a_down && !JoystickHeld[0], !button_a_down && JoystickHeld[0]);
	DIJoystickButtons[1] = Build_Button_State(button_b_down, button_b_down && !JoystickHeld[1], !button_b_down && JoystickHeld[1]);
	JoystickHeld[0] = button_a_down;
	JoystickHeld[1] = button_b_down;

	DIJoystickAxis[JOYSTICK_X_AXIS] = axis_x;
	DIJoystickAxis[JOYSTICK_Y_AXIS] = axis_y;
}

void DirectInput::Read( void )
{
	SDL_PumpEvents();

	if (!Captured && GameInFocus) {
		Acquire();
	}

	if (!Captured) {
		return;
	}

	ReadKeyboard();
	ReadMouse();
	ReadJoystick();
	Update_Double_Clicks();
}

void DirectInput::Eat_Mouse_Held_States (void)
{
	if (	(DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] & DI_BUTTON_HELD) ||
			(DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] & DI_BUTTON_HIT))
	{
		EatMouseHeld = true;
	}
}

void DirectInput::Reset_Cursor_Pos (const Vector2 &pos)
{
	CursorPos.X = pos.X;
	CursorPos.Y = pos.Y;

	std::lock_guard<std::mutex> lock(InputMutex);
	PendingCursorPos.X = pos.X;
	PendingCursorPos.Y = pos.Y;
}

void	DirectInput::Update_Double_Clicks (void)
{
	const float time_delta = TimeManager::Get_Frame_Real_Seconds();
	for (int index = 0; index < NUM_KEYBOARD_BUTTONS; ++index) {
		ButtonLastHitTime[index] += time_delta;

		if (DIKeyboardButtons[index] & DI_BUTTON_HIT) {
			if (ButtonLastHitTime[index] <= BUTTON_DOUBLE_THRESHHOLD) {
				DIKeyboardButtons[index] |= BUTTON_BIT_DOUBLE;
			}
			ButtonLastHitTime[index] = 0.0f;
		}
	}
}
