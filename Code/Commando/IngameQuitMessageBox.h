#pragma once

#include "DlgMessageBox.h"

class IngameQuitMessageBoxClass : public DlgMsgBox
{
public:
	static bool DoDialog(Observer<DlgMsgBoxEvent> *observer = NULL);

protected:
	IngameQuitMessageBoxClass();

	void On_Init_Dialog(void) override;
	void On_Command(int ctrl, int message, uint32_t param) override;
};
