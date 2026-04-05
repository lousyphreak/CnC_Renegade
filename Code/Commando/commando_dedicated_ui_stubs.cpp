#include "cdverify.h"
#include "DlgMessageBox.h"
#include "DlgMPConnectionRefused.h"
#include "DlgMPConnect.h"
#include "clienthintmanager.h"
#include "clientpingmanager.h"
#include "dlgcontrols.h"
#include "dlgmpchangelannickname.h"
#include "dialogtests.h"
#include "dlgcncbattleinfo.h"
#include "dlgcncserverinfo.h"
#include "dlgcncteaminfo.h"
#include "dlgconfigvideotab.h"
#include "dlgcncpurchasemainmenu.h"
#include "gamechannel.h"
#include "gamechanlist.h"
#include "dlgloadspgame.h"
#include "dlgmainmenu.h"
#include "dlgmpingamechat.h"
#include "textdisplay.h"
#include "radiocommanddisplay.h"
#include "menudialog.h"
#include "popupdialog.h"
#include "wwstring.h"

int DlgMsgBox::CurrentCount = 0;
#if defined(FREEDEDICATEDSERVER)
int DlgConfigVideoTabClass::GammaLevel = GAMMA_SLIDER_DEFAULT;
int DlgConfigVideoTabClass::BrightnessLevel = BRIGHTNESS_SLIDER_DEFAULT;
int DlgConfigVideoTabClass::ContrastLevel = CONTRAST_SLIDER_DEFAULT;
ControlsMenuClass *ControlsMenuClass::_TheInstance = nullptr;
#endif
#if defined(FREEDEDICATEDSERVER)
LoadSPGameMenuClass *LoadSPGameMenuClass::_TheInstance = nullptr;
MainMenuDialogClass *MainMenuDialogClass::_TheInstance = nullptr;
bool MainMenuDialogClass::Animated = false;
#endif
bool CNCPurchaseMainMenuClass::SecretsEnabled = false;
float RadioCommandDisplayClass::DisplayTimer = 0.0f;
bool RadioCommandDisplayClass::IsDisplayed = false;
TextWindowClass *RadioCommandDisplayClass::TextWindow = nullptr;
TextDisplayGameModeClass *TextDisplayGameModeClass::Instance = nullptr;

#if defined(FREEDEDICATEDSERVER)
const char *VALUE_NAME_DYN_LOD = "Dynamic_LOD_Budget";
const char *VALUE_NAME_STATIC_LOD = "Static_LOD_Budget";
const char *VALUE_NAME_DYN_SHADOWS = "Dynamic_Projectors";
const char *VALUE_NAME_SHADOW_MODE = "Shadow_Mode";
const char *VALUE_NAME_STATIC_SHADOWS = "Static_Projectors";
const char *VALUE_NAME_TEXTURE_RES = "Texture_Resolution";
const char *VALUE_NAME_PARTICLE_DETAIL = "Particle_Detail";
#endif

int DlgMpChangeLanNickname::DialogCount = 0;
int cClientPingManager::PingNumber = 0;
uint32_t cClientPingManager::TimeSentMs = 0;
uint32_t cClientPingManager::LastRoundTripPingMs = 0;
uint32_t cClientPingManager::AvgRoundTripPingMs = 0;
bool cClientPingManager::IsAwaitingResponse = false;
uint32_t cClientPingManager::RoundTripPingSamplesMs[cClientPingManager::MAX_SAMPLES] = {};
SList<cGameChannel> cGameChannelList::ChanList;

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

bool DlgMsgBox::DoDialog(const WCHAR *, const WCHAR *, DlgMsgBox::Type, Observer<DlgMsgBoxEvent> *, uint32_t)
{
    return false;
}

bool DlgMsgBox::DoDialog(int, int, DlgMsgBox::Type, Observer<DlgMsgBoxEvent> *, uint32_t)
{
    return false;
}

bool DlgMPConnectionRefused::DoDialog(const WCHAR *, bool)
{
    return false;
}

DlgMPConnect::DlgMPConnect(int teamChoice, uint32_t clanID)
    : PopupDialogClass(0),
      mTeamChoice(teamChoice),
      mClanID(clanID),
      mTheGame(nullptr),
      mFailed(false)
{
}

DlgMPConnect::~DlgMPConnect() = default;

bool DlgMPConnect::DoDialog(int, uint32_t)
{
    return false;
}

void DlgMPConnect::Connected(cGameData *theGame)
{
    mTheGame = theGame;
    mFailed = false;
}

void DlgMPConnect::Failed_To_Connect(void)
{
    mFailed = true;
}

void DlgMPConnect::On_Command(int, int, uint32_t)
{
}

void DlgMPConnect::On_Periodic(void)
{
}

bool DlgMpChangeLanNickname::DoDialog(void)
{
    return false;
}

DlgMpChangeLanNickname::DlgMpChangeLanNickname()
    : PopupDialogClass(0)
{
}

DlgMpChangeLanNickname::~DlgMpChangeLanNickname() = default;

void DlgMpChangeLanNickname::On_Init_Dialog(void)
{
}

void DlgMpChangeLanNickname::On_Command(int, int, uint32_t)
{
}

void DlgMpChangeLanNickname::On_EditCtrl_Change(EditCtrlClass *, int)
{
}

void DlgMpChangeLanNickname::On_EditCtrl_Enter_Pressed(EditCtrlClass *, int)
{
}

#if defined(FREEDEDICATEDSERVER)
void ControlsMenuClass::Reload(void)
{
}

void ControlsMenuClass::Apply_Changes(void)
{
}
#endif

void cClientPingManager::Init(void)
{
    PingNumber = 0;
    TimeSentMs = 0;
    LastRoundTripPingMs = 0;
    AvgRoundTripPingMs = 0;
    IsAwaitingResponse = false;
}

void cClientPingManager::Think(void)
{
}

uint32_t cClientPingManager::Get_Last_Round_Trip_Ping_Ms(void)
{
    return LastRoundTripPingMs;
}

uint32_t cClientPingManager::Get_Avg_Round_Trip_Ping_Ms(void)
{
    return AvgRoundTripPingMs;
}

void cClientPingManager::Response_Received(int)
{
}

void cClientPingManager::Compute_Average_Round_Trip_Ping_Ms(void)
{
}

void cClientHintManager::Think(void)
{
}

int __cdecl cClientHintManager::Priority_Compare(const void **, const void **)
{
    return 0;
}

void cGameChannelList::Add_Channel(cGameData *, const RefPtr<WWOnline::ChannelData> &)
{
}

void cGameChannelList::Remove_Channel(const WideStringClass &)
{
}

void cGameChannelList::Remove_All(void)
{
}

cGameChannel *cGameChannelList::Find_Channel(const WideStringClass &)
{
    return nullptr;
}

void DlgMsgBox::SetResourceType(DlgMsgBox::Type)
{
}

void DlgMsgBox::End_Dialog(void)
{
    PopupDialogClass::End_Dialog();
}

void DlgMsgBox::On_Command(int, int, uint32_t)
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
void MPIngameChatPopupClass::On_Command(int, int, uint32_t) {}
void MPIngameChatPopupClass::Render(void) {}

#if defined(FREEDEDICATEDSERVER)
void LoadSPGameMenuClass::Set_Game_Rank(const char *, int)
{
}
#endif

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

void StatisticsDisplayManager::Set_Stat(const char *, const char *, uint32_t, const Vector2 &)
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

#if defined(FREEDEDICATEDSERVER)
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

void MainMenuDialogClass::On_Command(int, int, uint32_t)
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
#endif

EditWheeledVehicleDialogClass::EditWheeledVehicleDialogClass(WheeledVehicleDefClass *def, float wheel_radius)
        : PopupDialogClass(0),
            VehicleDef(def),
            WheelRadius(wheel_radius)
{
}

EditWheeledVehicleDialogClass::~EditWheeledVehicleDialogClass(void) = default;
void EditWheeledVehicleDialogClass::On_Init_Dialog(void) {}
void EditWheeledVehicleDialogClass::On_Command(int, int, uint32_t) {}

EditTrackedVehicleDialogClass::EditTrackedVehicleDialogClass(TrackedVehicleDefClass *def, float wheel_radius)
        : PopupDialogClass(0),
            VehicleDef(def),
            WheelRadius(wheel_radius)
{
}

EditTrackedVehicleDialogClass::~EditTrackedVehicleDialogClass(void) = default;
void EditTrackedVehicleDialogClass::On_Init_Dialog(void) {}
void EditTrackedVehicleDialogClass::On_Command(int, int, uint32_t) {}

void DeathOptionsPopupClass::On_Init_Dialog(void) {}
void DeathOptionsPopupClass::On_Command(int, int, uint32_t) {}
void FailedOptionsPopupClass::On_Init_Dialog(void) {}
void FailedOptionsPopupClass::On_Command(int, int, uint32_t) {}
