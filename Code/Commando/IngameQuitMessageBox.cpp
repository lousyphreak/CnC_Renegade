#include "IngameQuitMessageBox.h"

#include "resource.h"
#include "string_ids.h"
#include "translatedb.h"

namespace {

const int kStaticControlId = -1;

}

bool IngameQuitMessageBoxClass::DoDialog(Observer<DlgMsgBoxEvent> *observer)
{
	IngameQuitMessageBoxClass *popup = new IngameQuitMessageBoxClass;

	if (popup != NULL) {
		if (observer != NULL) {
			popup->AddObserver(*observer);
		}

		popup->Start_Dialog();
		popup->Release_Ref();
	}

	return (popup != NULL);
}

IngameQuitMessageBoxClass::IngameQuitMessageBoxClass() :
	DlgMsgBox()
{
	DialogResID = IDD_QUIT_TO_DESKTOP;
}

void IngameQuitMessageBoxClass::On_Init_Dialog(void)
{
	PopupDialogClass::On_Init_Dialog();
	Set_Title(TRANSLATE(IDS_MENU_TEXT054));
	Set_Dlg_Item_Text(kStaticControlId, TRANSLATE(IDS_EXIT_GAME_VERIFICATION));
}

void IngameQuitMessageBoxClass::On_Command(int ctrl, int message, uint32_t param)
{
	switch (ctrl) {
		case IDC_QUIT:
		{
			Add_Ref();
			DlgMsgBoxEvent event(DlgMsgBoxEvent::Yes, this, Get_User_Data());
			NotifyObservers(event);
			Release_Ref();
			End_Dialog();
			break;
		}

		case IDC_BACK:
		case IDCANCEL:
		{
			Add_Ref();
			DlgMsgBoxEvent event(DlgMsgBoxEvent::No, this, Get_User_Data());
			NotifyObservers(event);
			Release_Ref();
			End_Dialog();
			break;
		}

		default:
			PopupDialogClass::On_Command(ctrl, message, param);
			break;
	}
}
