#include "gamespyadmin.h"
#include "GameSpy_QnR.h"
#include "gamespyauthmgr.h"
#include "GameSpyBanList.h"
#include "CDKeyAuth.h"
#include "WebBrowser.h"
#include "wolgmode.h"
#include "AutoStart.h"
#include "ConsoleMode.h"
#include "gamemode.h"
#include "nat.h"

#include "wwstring.h"
#include "widestring.h"

#include <algorithm>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <vector>

namespace {

std::vector<GameModeClass *> gGameModes;
unsigned gHiddenRenderFrames = 0;

}

#if !RENEGADE_WITH_GAMESPY

bool cGameSpyAdmin::IsUnderGamespyMenuing = false;
bool cGameSpyAdmin::IsLaunchFromGamespyRequested = false;
bool cGameSpyAdmin::IsLaunchedFromGamespy = false;
bool cGameSpyAdmin::IsServerGamespyListed = false;
ULONG cGameSpyAdmin::GameHostIp = 0;
USHORT cGameSpyAdmin::GameHostPort = 0;
WideStringClass cGameSpyAdmin::PasswordAttempt;

void cGameSpyAdmin::Think(void)
{
}

void cGameSpyAdmin::Reset(void)
{
    IsUnderGamespyMenuing = false;
    IsLaunchFromGamespyRequested = false;
    IsLaunchedFromGamespy = false;
    IsServerGamespyListed = false;
    GameHostIp = 0;
    GameHostPort = 0;
    PasswordAttempt = L"";
}

void cGameSpyAdmin::Set_Game_Host_Ip(ULONG ip)
{
    GameHostIp = ip;
}

void cGameSpyAdmin::Set_Game_Host_Port(USHORT port)
{
    GameHostPort = port;
}

bool cGameSpyAdmin::Is_Gamespy_Game(void)
{
    return false;
}

bool cGameSpyAdmin::Is_Nickname_Collision(WideStringClass &)
{
    return false;
}

void cGameSpyAdmin::Set_Password_Attempt(WideStringClass &password)
{
    PasswordAttempt = password;
}

WideStringClass &cGameSpyAdmin::Get_Password_Attempt(void)
{
    return PasswordAttempt;
}

CGameSpyQnR GameSpyQnR;

CGameSpyQnR::CGameSpyQnR() :
    m_GSEnabled(false)
{
}

CGameSpyQnR::~CGameSpyQnR() = default;

void CGameSpyQnR::Init(void)
{
    m_GSEnabled = false;
}

void CGameSpyQnR::LaunchArcade(void)
{
}

void CGameSpyQnR::TrackUsage(void)
{
}

void CGameSpyQnR::Shutdown(void)
{
    m_GSEnabled = false;
}

BOOL CGameSpyQnR::Parse_HeartBeat_List(const char *)
{
    return false;
}

const char *CGameSpyQnR::Get_GameSpy_GameName(void)
{
    return "gamespy-disabled";
}

const char *CGameSpyQnR::Get_Default_HeartBeat_List(void)
{
    return "";
}

void CGameSpyQnR::Enable_Reporting(BOOL enable)
{
    m_GSEnabled = enable;
}

BOOL CGameSpyQnR::IsEnabled(void)
{
    return m_GSEnabled;
}

void CGameSpyQnR::Think(void)
{
}

void CGameSpyQnR::basic_callback(char *, int)
{
}

void CGameSpyQnR::info_callback(char *, int)
{
}

void CGameSpyQnR::rules_callback(char *, int)
{
}

void CGameSpyQnR::players_callback(char *, int)
{
}

#endif

void cGameSpyAuthMgr::Think(void)
{
}

void cGameSpyAuthMgr::Initiate_Auth_Rejection(int)
{
}

LPCSTR cGameSpyAuthMgr::Describe_Auth_State(GAMESPY_AUTH_STATE_ENUM state)
{
    switch (state) {
    case GAMESPY_AUTH_STATE_INITIAL:
        return "initial";
    case GAMESPY_AUTH_STATE_CHALLENGED:
        return "challenged";
    case GAMESPY_AUTH_STATE_VALIDATING:
        return "validating";
    case GAMESPY_AUTH_STATE_ACCEPTED:
        return "accepted";
    case GAMESPY_AUTH_STATE_REJECTING:
        return "rejecting";
    case GAMESPY_AUTH_STATE_REJECTED:
        return "rejected";
    default:
        return "unknown";
    }
}

BanEntry::BanEntry(const char *name, const char *, const char *hash_id, const char *, bool rtype) :
    ipaddress(0xffffffff),
    ipmask(0xffffffff),
    ruletype(rtype)
{
    nickname[0] = '\0';
    hashid[0] = '\0';

    if (name != nullptr) {
        std::strncpy(nickname, name, sizeof(nickname) - 1);
        nickname[sizeof(nickname) - 1] = '\0';
    }

    if (hash_id != nullptr) {
        std::strncpy(hashid, hash_id, sizeof(hashid) - 1);
        hashid[sizeof(hashid) - 1] = '\0';
    }
}

cGameSpyBanList GameSpyBanList;

cGameSpyBanList::cGameSpyBanList() :
    BanList(nullptr)
{
}

cGameSpyBanList::~cGameSpyBanList() = default;

bool cGameSpyBanList::Final_Player_Kick(int)
{
    return false;
}

bool cGameSpyBanList::Begin_Player_Kick(int)
{
    return false;
}

void cGameSpyBanList::Strip_Escapes(char *)
{
}

void cGameSpyBanList::Think(void)
{
}

void cGameSpyBanList::Ban_User(const char *, const char *, ULONG)
{
}

bool cGameSpyBanList::Is_User_Banned(const char *, const char *, ULONG)
{
    return false;
}

void cGameSpyBanList::LoadBans(void)
{
}

void CCDKeyAuth::GetSerialNum(StringClass &serial)
{
    serial = "";
}

void CCDKeyAuth::DisconnectUser(int)
{
}

void CCDKeyAuth::AuthenticateUser(int, ULONG, char *, char *)
{
}

char *CCDKeyAuth::GenChallenge(int)
{
    static char empty_challenge[] = "";
    return empty_challenge;
}

void CCDKeyAuth::AuthSerial(const char *, StringClass &resp)
{
    resp = "";
}

Vector3 GameModeManager::BackgroundColor;
int GameMajorModeClass::NumActiveMajorModes = 0;

void GameModeClass::Activate()
{
    if (State == GAME_MODE_ACTIVE) {
        return;
    }
    State = GAME_MODE_ACTIVE;
    Init();
}

void GameModeClass::Deactivate()
{
    if (State == GAME_MODE_INACTIVE) {
        return;
    }
    Shutdown();
    State = GAME_MODE_INACTIVE;
}

void GameModeClass::Safely_Deactivate()
{
    if (State == GAME_MODE_INACTIVE_PENDING) {
        Deactivate();
    }
}

void GameModeClass::Suspend()
{
    if (State == GAME_MODE_ACTIVE) {
        State = GAME_MODE_SUSPENDED;
    }
}

void GameModeClass::Resume()
{
    if (State == GAME_MODE_SUSPENDED) {
        State = GAME_MODE_ACTIVE;
    }
}

void GameMajorModeClass::Activate()
{
    if (State != GAME_MODE_ACTIVE) {
        ++NumActiveMajorModes;
    }
    GameModeClass::Activate();
}

void GameMajorModeClass::Deactivate()
{
    if (State == GAME_MODE_ACTIVE && NumActiveMajorModes > 0) {
        --NumActiveMajorModes;
    }
    GameModeClass::Deactivate();
}

GameModeClass *GameModeManager::Add(GameModeClass *mode)
{
    if (mode != nullptr) {
        gGameModes.push_back(mode);
    }
    return mode;
}

void GameModeManager::Remove(GameModeClass *mode)
{
    gGameModes.erase(std::remove(gGameModes.begin(), gGameModes.end(), mode), gGameModes.end());
}

int GameModeManager::Count()
{
    return static_cast<int>(gGameModes.size());
}

void GameModeManager::Destroy(GameModeClass *mode)
{
    Remove(mode);
    delete mode;
}

void GameModeManager::Destroy_All(void)
{
    for (GameModeClass *mode : gGameModes) {
        delete mode;
    }
    gGameModes.clear();
}

void GameModeManager::List_Active_Game_Modes(void)
{
}

void GameModeManager::Think(void)
{
    for (GameModeClass *mode : gGameModes) {
        if (mode != nullptr && !mode->Is_Inactive() && !mode->Is_Suspended()) {
            mode->Think();
        }
    }
}

void GameModeManager::Render(void)
{
    if (gHiddenRenderFrames > 0) {
        --gHiddenRenderFrames;
        return;
    }

    for (GameModeClass *mode : gGameModes) {
        if (mode != nullptr && !mode->Is_Inactive()) {
            mode->Render();
        }
    }
}

GameModeClass *GameModeManager::Find(const char *name)
{
    for (GameModeClass *mode : gGameModes) {
        if (mode != nullptr && name != nullptr && std::strcmp(mode->Name(), name) == 0) {
            return mode;
        }
    }
    return nullptr;
}

void GameModeManager::Safely_Deactivate()
{
    for (GameModeClass *mode : gGameModes) {
        if (mode != nullptr) {
            mode->Safely_Deactivate();
        }
    }
}

void GameModeManager::Hide_Render_Frames(unsigned frame_count)
{
    gHiddenRenderFrames = frame_count;
}

#if !defined(_WIN32)

ConsoleModeClass ConsoleBox;

ConsoleModeClass::ConsoleModeClass(void) :
    ConsoleInputHandle(0),
    ConsoleOutputHandle(0),
    ConsoleWindow(0),
    LastKeypressTime(0),
    Pos(0),
    IsExclusive(false),
    ProfileMode(false),
    LastProfileCRC(0),
    LastProfilePrint(0)
{
    Title[0] = 0;
}

ConsoleModeClass::~ConsoleModeClass(void) = default;

void ConsoleModeClass::Init(void)
{
}

void ConsoleModeClass::Set_Title(char *name, char *settings)
{
    StringClass title = Compose_Window_Title(name, settings, false);
    std::snprintf(Title, sizeof(Title), "%s", title.Peek_Buffer());
}

void ConsoleModeClass::Think(void)
{
}

void ConsoleModeClass::Wait_For_Keypress(void)
{
}

void ConsoleModeClass::Print(char const * string, ...)
{
    if (string == NULL) {
        return;
    }

    char buffer[4096];
    va_list args;
    va_start(args, string);
    std::vsnprintf(buffer, sizeof(buffer), string, args);
    va_end(args);
    std::fputs(buffer, stdout);
}

void ConsoleModeClass::Print_Maybe(char const * string, ...)
{
    if (string == NULL) {
        return;
    }

    char buffer[4096];
    va_list args;
    va_start(args, string);
    std::vsnprintf(buffer, sizeof(buffer), string, args);
    va_end(args);
    std::fputs(buffer, stdout);
}

void ConsoleModeClass::Static_Print_Maybe(char const * string, ...)
{
    if (string == NULL) {
        return;
    }

    char buffer[4096];
    va_list args;
    va_start(args, string);
    std::vsnprintf(buffer, sizeof(buffer), string, args);
    va_end(args);
    std::fputs(buffer, stdout);
}

void ConsoleModeClass::Add_Message(WideStringClass *, Vector3 *, bool)
{
}

void ConsoleModeClass::Update_Profile(StringClass)
{
}

void ConsoleModeClass::Handle_Profile_Key(int)
{
}

HWND ConsoleModeClass::Get_Slave_Window_By_Title(char *, char *)
{
    return 0;
}

StringClass ConsoleModeClass::Compose_Window_Title(char *name, char *settings, bool slave)
{
    StringClass title;
    if (name != NULL) {
        title = name;
    }
    if (settings != NULL && settings[0] != 0) {
        if (title.Is_Empty() == false) {
            title += " - ";
        }
        title += settings;
    }
    if (slave) {
        if (title.Is_Empty() == false) {
            title += " ";
        }
        title += "(slave)";
    }
    return title;
}

void ConsoleModeClass::cprintf(char const * string, ...)
{
    if (string == NULL) {
        return;
    }

    char buffer[4096];
    va_list args;
    va_start(args, string);
    std::vsnprintf(buffer, sizeof(buffer), string, args);
    va_end(args);
    std::fputs(buffer, stdout);
}

void ConsoleModeClass::Log_To_Disk(const char *)
{
}

const char *ConsoleModeClass::Get_Log_File_Name(void)
{
    static const char kLogFileName[] = "renegade.log";
    return kLogFileName;
}

#endif

#if !RENEGADE_WITH_LEGACY_WOL

AutoRestartClass AutoRestart;

FirewallHelperClass FirewallHelper;
WOLNATInterfaceClass WOLNATInterface;

WebBrowser *WebBrowser::_mInstance = nullptr;

#ifdef _DEBUG
bool WebBrowser::InstallPrerequisites(void)
{
    return false;
}
#endif

bool WebBrowser::IsWebPageDisplayed(void)
{
    return false;
}

WebBrowser *WebBrowser::CreateInstance(HWND)
{
    if (_mInstance == nullptr) {
        _mInstance = new WebBrowser();
    }
    return _mInstance;
}

bool WebBrowser::IsExternalBrowserRunning(void) const
{
    return false;
}

bool WebBrowser::ShowWebPage(char *)
{
    return false;
}

bool WebBrowser::LaunchExternal(const char *)
{
    return false;
}

void WebBrowser::Show(void)
{
    mVisible = true;
}

void WebBrowser::Hide(void)
{
    mVisible = false;
}

WebBrowser::WebBrowser() :
    mVisible(false)
{
}

WebBrowser::~WebBrowser() = default;

WolGameModeClass::WolGameModeClass() = default;
WolGameModeClass::~WolGameModeClass() = default;
void WolGameModeClass::Init(void) {}
void WolGameModeClass::Shutdown(void) {}
void WolGameModeClass::Think(void) {}
void WolGameModeClass::Create_Game(cGameData *) {}
void WolGameModeClass::Leave_Game(void) {}
void WolGameModeClass::Start_Game(cGameData *) {}
void WolGameModeClass::End_Game(void) {}
void WolGameModeClass::Accept_Actions(void) {}
void WolGameModeClass::Refusal_Actions(void) {}
void WolGameModeClass::Init_WOL_Player(cPlayer *) {}
RefPtr<WWOnline::UserData> WolGameModeClass::Get_WOL_User_Data(const wchar_t *) { return RefPtr<WWOnline::UserData>(); }
void WolGameModeClass::Page_WOL_User(const wchar_t *, const wchar_t *) {}
void WolGameModeClass::Reply_Last_Page(const wchar_t *) {}
void WolGameModeClass::Locate_WOL_User(const wchar_t *) {}
void WolGameModeClass::Invite_WOL_User(const wchar_t *, const wchar_t *) {}
void WolGameModeClass::Join_WOL_User(const wchar_t *) {}
bool WolGameModeClass::Kick_Player(const wchar_t *) { return false; }
void WolGameModeClass::Ban_Player(const wchar_t *, unsigned long) {}
bool WolGameModeClass::Is_Banned(const char *, unsigned long) { return false; }
void WolGameModeClass::Read_Kick_List(void) {}
void WolGameModeClass::Auto_Kick(void) {}
void WolGameModeClass::System_Timer_Reset(void) {}
void WolGameModeClass::Set_Quiet_Mode(bool) {}
bool WolGameModeClass::Post_Game_Check(void) { return false; }
void WolGameModeClass::Game_Start_Timeout_Callback(void) {}

#endif
