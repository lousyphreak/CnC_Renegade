#pragma once

#include <cstdint>

#include "dialogresource.h"
#include "menudialog.h"
#include "popupdialog.h"
#include "resource.h"

class ClientQuitVerificationDialogClass : public PopupDialogClass
{
public:
	ClientQuitVerificationDialogClass();
	~ClientQuitVerificationDialogClass() override;

	static ClientQuitVerificationDialogClass *Get_Instance() { return _TheInstance; }

private:
	static ClientQuitVerificationDialogClass *_TheInstance;
};

class ClientStartSPGameDialogClass : public MenuDialogClass
{
public:
	ClientStartSPGameDialogClass();

	void On_Init_Dialog() override;
	void On_Command(int ctrl_id, int message_id, uint32_t param) override;
};

class ClientDifficultyMenuClass : public MenuDialogClass
{
public:
	ClientDifficultyMenuClass();
	void Set_Replay(const char *filename) { ReplayFilename = filename; }

	void On_Frame_Update() override;
	void On_Command(int ctrl_id, int message_id, uint32_t param) override;

private:
	StringClass ReplayFilename;
	int CurrSel;
};

class ClientOptionsMenuClass : public MenuDialogClass
{
public:
	ClientOptionsMenuClass();

	void On_Init_Dialog() override;
};

class ClientInternetMenuDialogClass : public MenuDialogClass
{
public:
	ClientInternetMenuDialogClass();

	void On_Init_Dialog() override;
};

class ClientLanMenuDialogClass : public MenuDialogClass
{
public:
	ClientLanMenuDialogClass();

	void On_Init_Dialog() override;
};
