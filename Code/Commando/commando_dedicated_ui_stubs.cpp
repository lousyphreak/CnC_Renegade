#include "cdverify.h"
#include "DlgMessageBox.h"
#include "DlgMPConnectionRefused.h"
#include "dialogtests.h"
#include "dlgcncbattleinfo.h"
#include "dlgcncserverinfo.h"
#include "dlgcncteaminfo.h"
#include "dlgconfigvideotab.h"
#include "dlgcncpurchasemainmenu.h"
#include "dlgloadspgame.h"
#include "dlgmainmenu.h"
#include "dlgmpingamechat.h"
#include "textdisplay.h"
#include "radiocommanddisplay.h"
#include "menudialog.h"
#include "popupdialog.h"
#include "wwstring.h"

int DlgMsgBox::CurrentCount = 0;
int DlgConfigVideoTabClass::GammaLevel = GAMMA_SLIDER_DEFAULT;
int DlgConfigVideoTabClass::BrightnessLevel = BRIGHTNESS_SLIDER_DEFAULT;
int DlgConfigVideoTabClass::ContrastLevel = CONTRAST_SLIDER_DEFAULT;
LoadSPGameMenuClass *LoadSPGameMenuClass::_TheInstance = nullptr;
MainMenuDialogClass *MainMenuDialogClass::_TheInstance = nullptr;
bool MainMenuDialogClass::Animated = false;
bool CNCPurchaseMainMenuClass::SecretsEnabled = false;
bool g_is_loading = false;
float RadioCommandDisplayClass::DisplayTimer = 0.0f;
bool RadioCommandDisplayClass::IsDisplayed = false;
TextWindowClass *RadioCommandDisplayClass::TextWindow = nullptr;
TextDisplayGameModeClass *TextDisplayGameModeClass::Instance = nullptr;

const char *VALUE_NAME_DYN_LOD = "Dynamic_LOD_Budget";
const char *VALUE_NAME_STATIC_LOD = "Static_LOD_Budget";
const char *VALUE_NAME_DYN_SHADOWS = "Dynamic_Projectors";
const char *VALUE_NAME_SHADOW_MODE = "Shadow_Mode";
const char *VALUE_NAME_STATIC_SHADOWS = "Static_Projectors";
const char *VALUE_NAME_TEXTURE_RES = "Texture_Resolution";
const char *VALUE_NAME_PARTICLE_DETAIL = "Particle_Detail";

bool CDVerifyClass::Get_CD_Path(StringClass &drive_path)
{
    drive_path = "";
    return false;
}

void CDVerifyClass::Display_UI(Observer<CDVerifyEvent> *)
{
}

DlgMsgBox::DlgMsgBox() : PopupDialogClass(0), mUserData(0)
{
}

DlgMsgBox::~DlgMsgBox() = default;

bool DlgMsgBox::DoDialog(const WCHAR *, const WCHAR *, DlgMsgBox::Type, Observer<DlgMsgBoxEvent> *, unsigned long)
{
    return false;
}

bool DlgMsgBox::DoDialog(int, int, DlgMsgBox::Type, Observer<DlgMsgBoxEvent> *, unsigned long)
{
    return false;
}

bool DlgMPConnectionRefused::DoDialog(const WCHAR *, bool)
{
    return false;
}

void DlgMsgBox::SetResourceType(DlgMsgBox::Type)
{
}

void DlgMsgBox::End_Dialog(void)
{
    PopupDialogClass::End_Dialog();
}

void DlgMsgBox::On_Command(int, int, DWORD)
{
}

CNCServerInfoDialogClass::CNCServerInfoDialogClass(void) : MenuDialogClass(0)
{
}

CNCServerInfoDialogClass::~CNCServerInfoDialogClass(void) = default;
void CNCServerInfoDialogClass::On_Init_Dialog(void) {}
void CNCServerInfoDialogClass::On_Frame_Update(void) {}

CNCTeamInfoDialogClass::CNCTeamInfoDialogClass(void) : MenuDialogClass(0)
{
}

CNCTeamInfoDialogClass::~CNCTeamInfoDialogClass(void) = default;
void CNCTeamInfoDialogClass::On_Init_Dialog(void) {}
void CNCTeamInfoDialogClass::On_Frame_Update(void) {}

CNCBattleInfoDialogClass::CNCBattleInfoDialogClass(void) : MenuDialogClass(0)
{
}

CNCBattleInfoDialogClass::~CNCBattleInfoDialogClass(void) = default;
void CNCBattleInfoDialogClass::On_Init_Dialog(void) {}
void CNCBattleInfoDialogClass::On_Frame_Update(void) {}

MPIngameChatPopupClass::MPIngameChatPopupClass(void)
    : PopupDialogClass(0),
      DefaultType(TEXT_MESSAGE_PUBLIC),
      ChatModule(nullptr)
{
}

MPIngameChatPopupClass::~MPIngameChatPopupClass(void) = default;
void MPIngameChatPopupClass::On_Init_Dialog(void) {}
void MPIngameChatPopupClass::On_Command(int, int, DWORD) {}
void MPIngameChatPopupClass::Render(void) {}

void LoadSPGameMenuClass::Set_Game_Rank(const char *, int)
{
}

void TextDebugDisplayHandlerClass::Display_Text(const char *, const Vector4 &)
{
}

void TextDebugDisplayHandlerClass::Display_Text(const WideStringClass &, const Vector4 &)
{
}

void TextDisplayGameModeClass::Init()
{
    Instance = this;
}

void TextDisplayGameModeClass::Shutdown()
{
    if (Instance == this) {
        Instance = nullptr;
    }
}

void TextDisplayGameModeClass::Think()
{
}

void TextDisplayGameModeClass::Render()
{
}

void TextDisplayGameModeClass::Flush(void)
{
}

void TextDisplayGameModeClass::Print(const char *, const Vector4 &)
{
}

void TextDisplayGameModeClass::Print(const WideStringClass &, const Vector4 &)
{
}

void TextDisplayGameModeClass::Print(const char *, const Vector3 &)
{
}

void TextDisplayGameModeClass::Print(const WideStringClass &, const Vector3 &)
{
}

void TextDisplayGameModeClass::Print_System(const char *, ...)
{
}

void TextDisplayGameModeClass::Print_System(const WideStringClass &)
{
}

void TextDisplayGameModeClass::Print_Informational(const char *, ...)
{
}

void TextDisplayGameModeClass::Print_Informational(const WideStringClass &)
{
}

void StatisticsDisplayManager::Render(Render2DTextClass *)
{
}

void StatisticsDisplayManager::Set_Display(const char *)
{
}

bool StatisticsDisplayManager::Is_Current_Display(const char *)
{
    return false;
}

void StatisticsDisplayManager::Set_Stat(const char *, const char *, unsigned long, const Vector2 &)
{
}

void RadioCommandDisplayClass::Initialize(void) {}
void RadioCommandDisplayClass::Shutdown(void) {}
void RadioCommandDisplayClass::Display(bool onoff, DISPLAY_TYPE)
{
    IsDisplayed = onoff;
}
void RadioCommandDisplayClass::Render(void) {}
void RadioCommandDisplayClass::Check_Keys(void) {}
void RadioCommandDisplayClass::Update(DISPLAY_TYPE) {}

MainMenuDialogClass::MainMenuDialogClass(void)
    : MenuDialogClass(0),
      LogoModel(nullptr),
      TitleTransModel(nullptr),
      GizmoModel(nullptr),
      IsStartingPractice(false)
{
    _TheInstance = this;
}

MainMenuDialogClass::~MainMenuDialogClass(void)
{
    if (_TheInstance == this) {
        _TheInstance = nullptr;
    }
}

void MainMenuDialogClass::Display(void)
{
}

void MainMenuDialogClass::On_Command(int, int, DWORD)
{
}

DialogTransitionClass *MainMenuDialogClass::Get_Transition_In(DialogBaseClass *)
{
    return nullptr;
}

DialogTransitionClass *MainMenuDialogClass::Get_Transition_Out(DialogBaseClass *)
{
    return nullptr;
}

void MainMenuDialogClass::On_Init_Dialog(void)
{
}

void MainMenuDialogClass::On_Menu_Activate(bool)
{
}

void MainMenuDialogClass::Update_Version_Number(void)
{
}

StringClass MainMenuDialogClass::Choose_Skirmish_Map(void)
{
    return StringClass(true);
}

EditWheeledVehicleDialogClass::EditWheeledVehicleDialogClass(WheeledVehicleDefClass *def, float wheel_radius)
        : PopupDialogClass(0),
            VehicleDef(def),
            WheelRadius(wheel_radius)
{
}

EditWheeledVehicleDialogClass::~EditWheeledVehicleDialogClass(void) = default;
void EditWheeledVehicleDialogClass::On_Init_Dialog(void) {}
void EditWheeledVehicleDialogClass::On_Command(int, int, DWORD) {}

EditTrackedVehicleDialogClass::EditTrackedVehicleDialogClass(TrackedVehicleDefClass *def, float wheel_radius)
        : PopupDialogClass(0),
            VehicleDef(def),
            WheelRadius(wheel_radius)
{
}

EditTrackedVehicleDialogClass::~EditTrackedVehicleDialogClass(void) = default;
void EditTrackedVehicleDialogClass::On_Init_Dialog(void) {}
void EditTrackedVehicleDialogClass::On_Command(int, int, DWORD) {}

void DeathOptionsPopupClass::On_Init_Dialog(void) {}
void DeathOptionsPopupClass::On_Command(int, int, DWORD) {}
void FailedOptionsPopupClass::On_Init_Dialog(void) {}
void FailedOptionsPopupClass::On_Command(int, int, DWORD) {}
