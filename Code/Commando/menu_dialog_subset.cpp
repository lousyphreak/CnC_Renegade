#include "menu_dialog_subset.h"

#include "campaign.h"
#include "dialogcontrol.h"
#include "dialogmgr.h"
#include "gameinitmgr.h"
#include "gamemode.h"
#include "god.h"
#include "string_ids.h"
#include "translatedb.h"
#include "useroptions.h"

ClientQuitVerificationDialogClass *ClientQuitVerificationDialogClass::_TheInstance = nullptr;

extern void Stop_Main_Loop(int exitCode);

namespace {

void Disable_If_Present(DialogBaseClass *dialog, int ctrl_id)
{
	if (dialog == nullptr) {
		return;
	}

	DialogControlClass *control = dialog->Get_Dlg_Item(ctrl_id);
	if (control != nullptr) {
		control->Enable(false);
	}
}

}

ClientQuitVerificationDialogClass::ClientQuitVerificationDialogClass() :
	PopupDialogClass(IDD_QUIT_TO_DESKTOP)
{
	_TheInstance = this;

	if (cUserOptions::SkipQuitConfirmDialog.Is_True()) {
		Stop_Main_Loop(EXIT_SUCCESS);
	}
}

ClientQuitVerificationDialogClass::~ClientQuitVerificationDialogClass()
{
	_TheInstance = nullptr;
}

ClientStartSPGameDialogClass::ClientStartSPGameDialogClass() :
	MenuDialogClass(IDD_MENU_START_SP)
{
}

void ClientStartSPGameDialogClass::On_Init_Dialog()
{
	Disable_If_Present(this, IDC_MENU_LOAD_SP_GAME_BUTTON);
	MenuDialogClass::On_Init_Dialog();
}

void ClientStartSPGameDialogClass::On_Command(int ctrl_id, int message_id, DWORD param)
{
	if (ctrl_id == IDC_MENU_START_TUTORIAL_BUTTON) {
		const char *tutorial_map_name = "M00_Tutorial.mix";
		const int tutorial_load_menu_number = 90;

		cGod::Reset_Inventory();
		CampaignManager::Reset();
		CampaignManager::Select_Backdrop_Number(tutorial_load_menu_number);
		GameInitMgrClass::Initialize_SP();
		GameInitMgrClass::Start_Game(tutorial_map_name, -1, 0);
	} else {
		CampaignManager::Select_Backdrop_Number(0);
	}

	MenuDialogClass::On_Command(ctrl_id, message_id, param);
}

ClientDifficultyMenuClass::ClientDifficultyMenuClass() :
	MenuDialogClass(IDD_MENU_DIFFICULTY),
	CurrSel(-1)
{
}

void ClientDifficultyMenuClass::On_Frame_Update()
{
	int curr_sel = -1;
	DialogControlClass *curr_focus = DialogMgrClass::Get_Focus();

	if (curr_focus == Get_Dlg_Item(IDC_MENU_DIFFCULTY01_BUTTON)) {
		curr_sel = 0;
	} else if (curr_focus == Get_Dlg_Item(IDC_MENU_DIFFCULTY02_BUTTON)) {
		curr_sel = 1;
	} else if (curr_focus == Get_Dlg_Item(IDC_MENU_DIFFCULTY03_BUTTON)) {
		curr_sel = 2;
	}

	if (curr_sel != CurrSel) {
		CurrSel = curr_sel;

		if (CurrSel == -1) {
			Set_Dlg_Item_Text(IDC_AIM_TEXT, TRANSLATE(IDS_MENU_NA));
			Set_Dlg_Item_Text(IDC_PLAYER_HEALTH_TEXT, TRANSLATE(IDS_MENU_NA));
			Set_Dlg_Item_Text(IDC_SUPPLIES_TEXT, TRANSLATE(IDS_MENU_NA));
			Set_Dlg_Item_Text(IDC_REINFORCEMENTS_TEXT, TRANSLATE(IDS_MENU_NA));
			Set_Dlg_Item_Text(IDC_BODY_ARMOR_TEXT, TRANSLATE(IDS_MENU_NA));
		} else if (CurrSel == 0) {
			Set_Dlg_Item_Text(IDC_AIM_TEXT, TRANSLATE(IDS_MENU_ON));
			Set_Dlg_Item_Text(IDC_PLAYER_HEALTH_TEXT, TRANSLATE(IDS_MENU_MAXIMUM));
			Set_Dlg_Item_Text(IDC_SUPPLIES_TEXT, TRANSLATE(IDS_MENU_PLENTIFUL));
			Set_Dlg_Item_Text(IDC_REINFORCEMENTS_TEXT, TRANSLATE(IDS_MENU_FEW));
			Set_Dlg_Item_Text(IDC_BODY_ARMOR_TEXT, TRANSLATE(IDS_MENU_LIGHT));
		} else if (CurrSel == 1) {
			Set_Dlg_Item_Text(IDC_AIM_TEXT, TRANSLATE(IDS_MENU_OFF));
			Set_Dlg_Item_Text(IDC_PLAYER_HEALTH_TEXT, TRANSLATE(IDS_MENU_ENHANCED));
			Set_Dlg_Item_Text(IDC_SUPPLIES_TEXT, TRANSLATE(IDS_MENU_SUFFICIENT));
			Set_Dlg_Item_Text(IDC_REINFORCEMENTS_TEXT, TRANSLATE(IDS_MENU_MANY));
			Set_Dlg_Item_Text(IDC_BODY_ARMOR_TEXT, TRANSLATE(IDS_MENU_STANDARD));
		} else if (CurrSel == 2) {
			Set_Dlg_Item_Text(IDC_AIM_TEXT, TRANSLATE(IDS_MENU_OFF));
			Set_Dlg_Item_Text(IDC_PLAYER_HEALTH_TEXT, TRANSLATE(IDS_MENU_NORMAL));
			Set_Dlg_Item_Text(IDC_SUPPLIES_TEXT, TRANSLATE(IDS_MENU_SCARCE));
			Set_Dlg_Item_Text(IDC_REINFORCEMENTS_TEXT, TRANSLATE(IDS_MENU_MAXIMUM));
			Set_Dlg_Item_Text(IDC_BODY_ARMOR_TEXT, TRANSLATE(IDS_MENU_HEAVY));
		}
	}

	MenuDialogClass::On_Frame_Update();
}

void ClientDifficultyMenuClass::On_Command(int ctrl_id, int message_id, DWORD param)
{
	switch (ctrl_id) {
		case IDC_MENU_DIFFCULTY01_BUTTON:
		case IDC_MENU_DIFFCULTY02_BUTTON:
		case IDC_MENU_DIFFCULTY03_BUTTON:
		case IDC_MENU_DIFFCULTY04_BUTTON:
		{
			const int difficulty = ctrl_id - IDC_MENU_DIFFCULTY01_BUTTON;
			if (ReplayFilename.Is_Empty()) {
				GameInitMgrClass::Initialize_SP();
				CampaignManager::Start_Campaign(difficulty);
			} else {
				if (GameModeManager::Find("Combat")->Is_Suspended()) {
					GameInitMgrClass::End_Game();
					GameModeManager::Safely_Deactivate();
				}

				GameInitMgrClass::Initialize_SP();
				CampaignManager::Replay_Level(ReplayFilename, difficulty);
			}
			break;
		}

		default:
			break;
	}

	MenuDialogClass::On_Command(ctrl_id, message_id, param);
}

ClientOptionsMenuClass::ClientOptionsMenuClass() :
	MenuDialogClass(IDD_MENU_OPTIONS)
{
}

void ClientOptionsMenuClass::On_Init_Dialog()
{
	MenuDialogClass::On_Init_Dialog();
}

ClientInternetMenuDialogClass::ClientInternetMenuDialogClass() :
	MenuDialogClass(IDD_MENU_MAIN_MULTIPLAY)
{
}

void ClientInternetMenuDialogClass::On_Init_Dialog()
{
	Disable_If_Present(this, IDC_MENU_MP_INTERNET_WOL);
	Disable_If_Present(this, IDC_MENU_MP_INTERNET_GAMESPY);
	MenuDialogClass::On_Init_Dialog();
}

ClientLanMenuDialogClass::ClientLanMenuDialogClass() :
	MenuDialogClass(IDD_MP_LAN_GAME_LIST)
{
}

void ClientLanMenuDialogClass::On_Init_Dialog()
{
	Disable_If_Present(this, IDC_JOIN_GAME_BUTTON);
	Disable_If_Present(this, IDC_MENU_MP_LAN_HOST_BUTTON);
	MenuDialogClass::On_Init_Dialog();
}
