#include "renegadedialogmgr.h"

#include "translatedb.h"

#if !defined(FREEDEDICATEDSERVER)
#include "ConsoleMode.h"
#include "dialogresource.h"
#include "dialogmgr.h"
#include "popupdialog.h"
#include "directinput.h"
#include "input.h"
#include "resource.h"
#endif

#include <algorithm>
#include <cwchar>

#if !defined(FREEDEDICATEDSERVER)
namespace {

class RenegadeUIInputClass final : public WWUIInputClass
{
public:
	const Vector3 &Get_Mouse_Pos(void) const override
	{
		DirectInput::Get_Cursor_Pos(&MousePos);
		return MousePos;
	}

	void Set_Mouse_Pos(const Vector3 &pos) override
	{
		MousePos = pos;
		DirectInput::Reset_Cursor_Pos(Vector2(pos.X, pos.Y));
	}

	bool Is_Button_Down(int vk_mouse_button_id) override
	{
		switch (vk_mouse_button_id) {
			case VK_LBUTTON:
				return Input::Is_Button_Down(DirectInput::BUTTON_MOUSE_LEFT);

			case VK_MBUTTON:
				return Input::Is_Button_Down(DirectInput::BUTTON_MOUSE_CENTER);

			case VK_RBUTTON:
				return Input::Is_Button_Down(DirectInput::BUTTON_MOUSE_RIGHT);

			default:
				break;
		}

		return false;
	}

	void Enter_Menu_Mode(void) override
	{
		Input::Menu_Enable(true);
	}

	void Exit_Menu_Mode(void) override
	{
		Input::Menu_Enable(false);
		DirectInput::Eat_Mouse_Held_States();
	}

private:
	mutable Vector3 MousePos;
};

} // namespace
#endif

DialogFactoryBaseClass *FactoryArray[FACTORY_COUNT] = {};
WWUIInputClass * _TheWWUIInput = nullptr;

int MyLoadStringW(UINT str_id, LPWSTR buffer, int buffer_len)
{
	if (buffer == nullptr || buffer_len <= 0) {
		return 0;
	}

	const WCHAR * source = TRANSLATE(str_id);
	if (source == nullptr) {
		source = L"";
	}

	const std::size_t count = std::min<std::size_t>(static_cast<std::size_t>(buffer_len - 1), std::wcslen(source));
	if (count > 0) {
		std::wmemcpy(buffer, source, count);
	}
	buffer[count] = 0;
	return static_cast<int>(count);
}

void RenegadeDialogMgrClass::Initialize(void)
{
	#if !defined(FREEDEDICATEDSERVER)
	const char *style_mgr_ini = "stylemgr.ini";

	if (_TheWWUIInput == nullptr) {
		_TheWWUIInput = new RenegadeUIInputClass;
		_TheWWUIInput->InitIME(MainWindow);
	}

	if (!ConsoleBox.Is_Exclusive()) {
		DialogMgrClass::Initialize(style_mgr_ini);
	}

	DialogMgrClass::Install_Input(_TheWWUIInput);
	#endif
}

void RenegadeDialogMgrClass::Shutdown(void)
{
	#if !defined(FREEDEDICATEDSERVER)
	DialogMgrClass::Shutdown();
	REF_PTR_RELEASE(_TheWWUIInput);
	#endif
}

void RenegadeDialogMgrClass::Do_Simple_Dialog(int dlg_res_id)
{
	#if !defined(FREEDEDICATEDSERVER)
	if (DialogMgrClass::Get_Dialog_Count() > 0) {
		return;
	}

	PopupDialogClass *dialog = new PopupDialogClass(dlg_res_id);
	dialog->Start_Dialog();
	REF_PTR_RELEASE(dialog);
	#endif
}

void RenegadeDialogMgrClass::Goto_Location(LOCATION location)
{
	#if !defined(FREEDEDICATEDSERVER)
	switch (location) {
		case LOC_MAIN_MENU:
			Do_Simple_Dialog(IDD_MENU_MAIN);
			break;

		default:
			break;
	}
	#endif
}