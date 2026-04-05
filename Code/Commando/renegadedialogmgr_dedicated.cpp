#include "renegadedialogmgr.h"

#include "translatedb.h"

#if !defined(FREEDEDICATEDSERVER)
#include "ConsoleMode.h"
#include "dialogbase.h"
#include "dialogresource.h"
#include "dialogmgr.h"
#include "dlgcncreference.h"
#include "dlgcharacteroptions.h"
#include "dlgcheatoptions.h"
#include "dlgcontrols.h"
#include "dlgcredits.h"
#include "dlgevaencyclopedia.h"
#include "dlghelpscreen.h"
#include "dlgloadspgame.h"
#include "dlgmainmenu.h"
#include "dlgmovieoptions.h"
#include "dlgpreviewoptions.h"
#include "dlgsavegame.h"
#include "dlgtechoptions.h"
#include "menu_dialog_subset.h"
#include "popupdialog.h"
#include "directinput.h"
#include "gamemode.h"
#include "input.h"
#include "resource.h"
#endif

#include <algorithm>
#include <cwchar>

extern void Stop_Main_Loop(int);

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

#if !defined(FREEDEDICATEDSERVER)
namespace {

bool CALLBACK Default_On_Command(DialogBaseClass *dialog, int ctrl_id, int mesage_id, uint32_t param)
{
	bool handled = true;

	if (ctrl_id >= DIALOG_LINK_FIRST && ctrl_id < DIALOG_LINK_LAST) {
		DialogFactoryBaseClass *factory = FactoryArray[ctrl_id - DIALOG_LINK_FIRST];
		if (factory != nullptr) {
			factory->Do_Dialog();
		}
	}

	switch (ctrl_id) {
		case IDC_MENU_BACK_BUTTON:
		case IDC_BACK:
		case IDCANCEL:
			dialog->End_Dialog();
			break;

		case IDC_QUIT:
			Stop_Main_Loop(EXIT_SUCCESS);
			break;

		default:
			handled = false;
			break;
	}

	return handled;
}

template <typename T>
void Install_Factory(int ctrl_id)
{
	const int index = ctrl_id - DIALOG_LINK_FIRST;
	if (index >= 0 && index < FACTORY_COUNT && FactoryArray[index] == nullptr) {
		FactoryArray[index] = new DialogFactoryClass<T>;
	}
}

void Initialize_Factories()
{
	Install_Factory<ClientStartSPGameDialogClass>(IDC_MENU_START_SP_GAME_BUTTON);
	Install_Factory<ClientInternetMenuDialogClass>(IDC_MENU_START_MP_GAME_BUTTON);
	Install_Factory<ClientOptionsMenuClass>(IDC_MENU_OPTIONS_BUTTON);
	Install_Factory<ClientDifficultyMenuClass>(IDC_MENU_START_CAMPAIGN_BUTTON);
	Install_Factory<LoadSPGameMenuClass>(IDC_MENU_LOAD_SP_GAME_BUTTON);
	Install_Factory<ControlsMenuClass>(IDC_MENU_CONTROLS_BUTTON);
	Install_Factory<CharacterOptionsMenuClass>(IDC_MENU_CHARACTER_BUTTON);
	Install_Factory<CheatOptionsMenuClass>(IDC_MENU_CHEATS_BUTTON);
	Install_Factory<TechOptionsMenuClass>(IDC_MENU_TECH_BUTTON);
	Install_Factory<MovieOptionsMenuClass>(IDC_MENU_MOVIES_BUTTON);
	Install_Factory<PreviewOptionsMenuClass>(IDC_MENU_PREVIEWS_BUTTON);
	Install_Factory<CreditsMenuClass>(IDC_MENU_CREDITS_BUTTON);
	Install_Factory<ClientQuitVerificationDialogClass>(IDC_MENU_QUIT_BUTTON);
	Install_Factory<MainMenuDialogClass>(IDC_MENU_MAIN_MENU_BUTTON);
	Install_Factory<SaveGameMenuClass>(IDC_MENU_SAVE_SP_GAME_BUTTON);
	Install_Factory<ClientLanMenuDialogClass>(IDC_MENU_MP_LAN_BUTTON);
	Install_Factory<ClientInternetMenuDialogClass>(IDC_MENU_MP_INTERNET_BUTTON);
	Install_Factory<ClientInternetMenuDialogClass>(IDC_MENU_MP_INTERNET_GAME_BUTTON);
	Install_Factory<ClientLanMenuDialogClass>(IDC_MENU_MP_LAN_GAME_BUTTON);
}

void Shutdown_Factories()
{
	for (int index = 0; index < FACTORY_COUNT; ++index) {
		delete FactoryArray[index];
		FactoryArray[index] = nullptr;
	}
}

} // namespace
#endif

int MyLoadStringW(uint32_t str_id, LPWSTR buffer, int buffer_len)
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
	Initialize_Factories();

	if (_TheWWUIInput == nullptr) {
		_TheWWUIInput = new RenegadeUIInputClass;
		_TheWWUIInput->InitIME(MainWindow);
	}

	if (!ConsoleBox.Is_Exclusive()) {
		DialogBaseClass::Set_Default_Command_Handler(Default_On_Command);
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
	Shutdown_Factories();
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
			MainMenuDialogClass::Display();
			break;

		case LOC_INTERNET_MAIN:
		case LOC_INTERNET_GAME_LIST:
		case LOC_GAMESPY_MAIN:
		{
			ClientInternetMenuDialogClass *dialog = new ClientInternetMenuDialogClass;
			dialog->Start_Dialog();
			REF_PTR_RELEASE(dialog);
			break;
		}

		case LOC_LAN_MAIN:
		{
			ClientLanMenuDialogClass *dialog = new ClientLanMenuDialogClass;
			dialog->Start_Dialog();
			REF_PTR_RELEASE(dialog);
			break;
		}

		case LOC_ENCYCLOPEDIA:
			EVAEncyclopediaMenuClass::Display();
			break;

		case LOC_OBJECTIVES:
			EVAEncyclopediaMenuClass::Display(EVAEncyclopediaMenuClass::TAB_OBJECTIVES);
			break;

		case LOC_MAP:
			EVAEncyclopediaMenuClass::Display(EVAEncyclopediaMenuClass::TAB_MAP);
			break;

		case LOC_CNC_REFERENCE:
			CnCReferenceMenuClass::Display();
			break;

		case LOC_LOAD_GAME:
			LoadSPGameMenuClass::Display();
			break;

		case LOC_IN_GAME_HELP:
			HelpScreenDialogClass::Display();
			break;

		default:
			break;
	}

	GameModeClass *menu_game_mode = GameModeManager::Find("Menu");
	if (menu_game_mode != nullptr && menu_game_mode->Is_Active() == false) {
		menu_game_mode->Activate();
	}
	#endif
}
