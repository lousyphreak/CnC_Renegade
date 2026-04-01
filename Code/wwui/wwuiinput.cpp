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
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/wwuiinput.cpp          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/08/02 8:41p                                               $*
 *                                                                                             *
 *                    $Revision:: 7                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "wwuiinput.h"
#include "dialogmgr.h"
#include "wwmemlog.h"

#include <SDL3/SDL_keyboard.h>

#include <cstring>

namespace {

uint32 SDL_Keycode_To_VKey(SDL_Keycode keycode)
{
	switch (keycode) {
		case SDLK_BACKSPACE:
			return VK_BACK;

		case SDLK_TAB:
			return VK_TAB;

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			return VK_RETURN;

		case SDLK_ESCAPE:
			return VK_ESCAPE;

		case SDLK_SPACE:
			return VK_SPACE;

		case SDLK_PAGEUP:
			return VK_PRIOR;

		case SDLK_PAGEDOWN:
			return VK_NEXT;

		case SDLK_END:
			return VK_END;

		case SDLK_HOME:
			return VK_HOME;

		case SDLK_LEFT:
			return VK_LEFT;

		case SDLK_UP:
			return VK_UP;

		case SDLK_RIGHT:
			return VK_RIGHT;

		case SDLK_DOWN:
			return VK_DOWN;

		case SDLK_DELETE:
			return VK_DELETE;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			return VK_SHIFT;

		case SDLK_LCTRL:
		case SDLK_RCTRL:
			return VK_CONTROL;

		default:
			break;
	}

	if (keycode >= SDLK_0 && keycode <= SDLK_9) {
		return static_cast<uint32>(keycode);
	}

	if (keycode >= SDLK_A && keycode <= SDLK_Z) {
		return static_cast<uint32>(keycode - SDLK_A + 'A');
	}

	return 0;
}

const char *Decode_UTF8_Code_Unit(const char *text, uint16 &unicode)
{
	unicode = 0;
	if (text == NULL) {
		return NULL;
	}

	const unsigned char lead = static_cast<unsigned char>(*text);
	if (lead == 0) {
		return text;
	}

	if ((lead & 0x80) == 0) {
		unicode = lead;
		return text + 1;
	}

	if ((lead & 0xE0) == 0xC0) {
		const unsigned char trail0 = static_cast<unsigned char>(text[1]);
		if ((trail0 & 0xC0) != 0x80) {
			return text + 1;
		}

		unicode = static_cast<uint16>(((lead & 0x1F) << 6) | (trail0 & 0x3F));
		return text + 2;
	}

	if ((lead & 0xF0) == 0xE0) {
		const unsigned char trail0 = static_cast<unsigned char>(text[1]);
		const unsigned char trail1 = static_cast<unsigned char>(text[2]);
		if ((trail0 & 0xC0) != 0x80 || (trail1 & 0xC0) != 0x80) {
			return text + 1;
		}

		unicode = static_cast<uint16>(((lead & 0x0F) << 12) | ((trail0 & 0x3F) << 6) | (trail1 & 0x3F));
		return text + 3;
	}

	return text + 1;
}

} // namespace


WWUIInputClass::WWUIInputClass(void) :
	mIMEManager(NULL)
{
}


WWUIInputClass::~WWUIInputClass(void)
{
	if (mIMEManager) {
		mIMEManager->Release_Ref();
	}
}


void WWUIInputClass::InitIME(HWND hwnd)
{
	(void)hwnd;
}


IME::IMEManager* WWUIInputClass::GetIME(void) const
{
	if (mIMEManager) {
		mIMEManager->Add_Ref();
	}

	return mIMEManager;
}


bool WWUIInputClass::ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
	WWMEMLOG(MEM_GAMEDATA);

	if (mIMEManager) {
		if (mIMEManager->ProcessMessage(hwnd, msg, wParam, lParam, result)) {
			return true;
		}
	}

	result = 0;

	switch (msg) {
		case WM_KEYDOWN:
			return DialogMgrClass::On_Key_Down(wParam, lParam);
			break;

		case WM_KEYUP:
			return DialogMgrClass::On_Key_Up(wParam);
			break;

		case WM_CHAR:
			DialogMgrClass::On_Unicode_Char((wchar_t)wParam);
			return true;
			break;

		default:
			break;
	}

	return false;
}


bool WWUIInputClass::ProcessEvent(const SDL_Event &event)
{
	switch (event.type) {
		case SDL_EVENT_KEY_DOWN:
		{
			const uint32 key_id = SDL_Keycode_To_VKey(event.key.key);
			return (key_id != 0) ? DialogMgrClass::On_Key_Down(key_id, 0) : false;
		}

		case SDL_EVENT_KEY_UP:
		{
			const uint32 key_id = SDL_Keycode_To_VKey(event.key.key);
			return (key_id != 0) ? DialogMgrClass::On_Key_Up(key_id) : false;
		}

		case SDL_EVENT_TEXT_INPUT:
		{
			bool handled = false;
			const char *cursor = event.text.text;
			while (cursor != NULL && *cursor != 0) {
				uint16 unicode = 0;
				const char *next = Decode_UTF8_Code_Unit(cursor, unicode);
				if (unicode != 0) {
					DialogMgrClass::On_Unicode_Char(static_cast<WCHAR>(unicode));
					handled = true;
				}
				if (next == cursor) {
					break;
				}
				cursor = next;
			}
			return handled;
		}

		default:
			break;
	}

	return false;
}


void WWUIInputClass::Update_Keyboard_State(BYTE *state) const
{
	if (state == NULL) {
		return;
	}

	std::memset(state, 0, 256);

	const SDL_Keymod modifiers = SDL_GetModState();
	if ((modifiers & SDL_KMOD_SHIFT) != 0) {
		state[VK_SHIFT] = VKEY_PRESSED;
	}
	if ((modifiers & SDL_KMOD_CTRL) != 0) {
		state[VK_CONTROL] = VKEY_PRESSED;
	}
}


void WWUIInputClass::HandleNotification(IME::UnicodeChar& unicode)
{
	DialogMgrClass::On_Unicode_Char(unicode.Subject());
}


void WWUIInputClass::HandleNotification(IME::IMEEvent& event)
	{
	if (IME::IME_LANGUAGECHANGED == event.GetAction())
		{
		const wchar_t* description = event.Subject()->GetDescription();
		DialogMgrClass::Show_IME_Message(description, 2500);
		}
	else if (IME::IME_GUIDELINE == event.GetAction())
		{
		wchar_t desc[255];
		unsigned long level = event.Subject()->GetGuideline(desc, sizeof(desc));

		if (GL_LEVEL_NOGUIDELINE != level)
			{
			DialogMgrClass::Show_IME_Message(desc, 30000);
			}
		}
	}
