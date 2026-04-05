#include "gamespyadmin.h"
#include "GameSpy_QnR.h"
#include "gamespyauthmgr.h"
#include "GameSpyBanList.h"
#include "CDKeyAuth.h"
#include "WebBrowser.h"
#include "wolgmode.h"
#include "AutoStart.h"
#include "ConsoleMode.h"
#include "nat.h"
#include "registry.h"
#include "slavemaster.h"
#include "_globals.h"

#include "wwstring.h"
SlaveServerClass::SlaveServerClass(void) :
    Port(0),
    Enable(false),
    IsRunning(false),
    ControlPort(0),
    Bandwidth(0xffffffff)
{
    NickName[0] = '\0';
    Serial[0] = '\0';
    Password[0] = '\0';
    SettingsFileName[0] = '\0';
    ProcessInfo.hProcess = NULL;
    ProcessInfo.hThread = NULL;
    ProcessInfo.dwProcessId = 0;
    ProcessInfo.dwThreadId = 0;
}

SlaveServerClass::~SlaveServerClass(void) = default;

void SlaveServerClass::Set(bool enable, char *nick, char *serial, uint16_t port, char *settings_file, int bandwidth, char *password)
{
    Enable = enable;
    Port = port;
    Bandwidth = bandwidth;
    IsRunning = false;
    ControlPort = 0;

    if (nick != NULL) {
        std::strncpy(NickName, nick, sizeof(NickName) - 1);
        NickName[sizeof(NickName) - 1] = '\0';
    } else {
        NickName[0] = '\0';
    }

    if (serial != NULL) {
        std::strncpy(Serial, serial, sizeof(Serial) - 1);
        Serial[sizeof(Serial) - 1] = '\0';
    } else {
        Serial[0] = '\0';
    }

    if (password != NULL) {
        std::strncpy(Password, password, sizeof(Password) - 1);
        Password[sizeof(Password) - 1] = '\0';
    } else {
        Password[0] = '\0';
    }

    if (settings_file != NULL) {
        std::strncpy(SettingsFileName, settings_file, sizeof(SettingsFileName) - 1);
        SettingsFileName[sizeof(SettingsFileName) - 1] = '\0';
    } else {
        SettingsFileName[0] = '\0';
    }
}

void SlaveServerClass::Get(bool &enable, char *nick, char *serial, uint16_t &port, char *settings_file, int &bandwidth, char *password)
{
    enable = Enable;
    port = Port;
    bandwidth = Bandwidth;

    if (nick != NULL) {
        std::strcpy(nick, NickName);
    }

    if (serial != NULL) {
        std::strcpy(serial, Serial);
    }

    if (settings_file != NULL) {
        std::strcpy(settings_file, SettingsFileName);
    }

    if (password != NULL) {
        std::strcpy(password, Password);
    }
}

SlaveMasterClass SlaveMaster;

SlaveMasterClass::SlaveMasterClass(void) :
    NumSlaveServers(0),
    SlaveMode(false)
{
}

SlaveMasterClass::~SlaveMasterClass(void) = default;

void SlaveMasterClass::Startup_Slaves(void)
{
}

void SlaveMasterClass::Shutdown_Slaves(void)
{
    for (int index = 0; index < NumSlaveServers; ++index) {
        SlaveServers[index].IsRunning = false;
    }
}

bool SlaveMasterClass::Shutdown_Slave(char *slave_login)
{
    if (slave_login == NULL) {
        return false;
    }

    for (int index = 0; index < NumSlaveServers; ++index) {
        if (std::strcmp(SlaveServers[index].NickName, slave_login) == 0) {
            SlaveServers[index].IsRunning = false;
            return true;
        }
    }

    return false;
}

char *SlaveMasterClass::Get_Slave_Info(char *buffer, int buflen)
{
    if (buffer == NULL || buflen <= 0) {
        return buffer;
    }

    std::snprintf(buffer, buflen, "single-process mode: slave orchestration disabled");
    return buffer;
}

void SlaveMasterClass::Load(void)
{
}

void SlaveMasterClass::Save(void)
{
}

void SlaveMasterClass::Reset(void)
{
    NumSlaveServers = 0;
    for (int index = 0; index < MAX_SLAVES; ++index) {
        SlaveServers[index] = SlaveServerClass();
    }
}

int SlaveMasterClass::Get_Num_Enabled_Slaves(void)
{
    int enabled_count = 0;
    for (int index = 0; index < NumSlaveServers; ++index) {
        if (SlaveServers[index].Enable) {
            ++enabled_count;
        }
    }
    return enabled_count;
}

void SlaveMasterClass::Add_Slave(bool enable, char *nick, char *serial, uint16_t port, char *settings_file, int bandwidth, char *password)
{
    if (NumSlaveServers >= MAX_SLAVES) {
        return;
    }

    SlaveServers[NumSlaveServers].Set(enable, nick, serial, port, settings_file, bandwidth, password);
    ++NumSlaveServers;
}

SlaveServerClass *SlaveMasterClass::Get_Slave(int index)
{
    if (index < 0 || index >= NumSlaveServers) {
        return NULL;
    }

    return &SlaveServers[index];
}

#include "widestring.h"

#include <SDL3/SDL_misc.h>

#include <cstring>
#include <cstdarg>
#include <cstdio>

namespace {

struct WebPageEntry {
    const char *name;
    const char *embedded_url;
    const char *external_url;
};

const WebPageEntry kDefaultWebPages[] = {
    {"BattleClans", "http://renchat2.westwood.com/cgi-bin/cgiclient?ren_clan_manager&request=expand_template&Template=index.html&SKU=3072&LANGCODE=0&embedded=1", "http://renchat2.westwood.com/cgi-bin/cgiclient?ren_clan_manager&request=expand_template&Template=index.html&SKU=3072&LANGCODE=0"},
    {"Ladder", "http://renchat2.westwood.com/renegade_embedded/index.html", "http://renchat2.westwood.com/renegade/index.html"},
    {"NetStatus", "http://battleclans.westwood.com/cgi-bin/cgiclient?rosetta&request=do_netstatus&LANGCODE=0&SKU=3072&embedded=1", "http://battleclans.westwood.com/cgi-bin/cgiclient?rosetta&request=do_netstatus&LANGCODE=0&SKU=3072"},
    {"News", "http://battleclans.westwood.com/cgi-bin/cgiclient?rosetta&request=do_news&LANGCODE=0&SKU=3072&embedded=1", "http://battleclans.westwood.com/cgi-bin/cgiclient?rosetta&request=do_news&LANGCODE=0&SKU=3072"},
    {"Signup", "http://games2.westwood.com/cgi-bin/cgiclient?ren_reg2&request=expand_template&Template=newreg_menu.html&LANGCODE=0&embedded=1&SKU=3072", "http://games2.westwood.com/cgi-bin/cgiclient?ren_reg2&request=expand_template&Template=newreg_menu.html&LANGCODE=0"},
    {nullptr, nullptr, nullptr},
};

const WebPageEntry *Find_Default_Web_Page(const char *page)
{
    if (page == NULL) {
        return nullptr;
    }

    for (const WebPageEntry *entry = kDefaultWebPages; entry->name != nullptr; ++entry) {
        if (std::strcmp(entry->name, page) == 0) {
            return entry;
        }
    }

    return nullptr;
}

bool Resolve_Web_Page_URL(const char *page, char *url, int url_size)
{
    if (page == NULL || url == NULL || url_size <= 0) {
        return false;
    }

    const WebPageEntry *default_entry = Find_Default_Web_Page(page);
    const char *default_url = (default_entry != nullptr) ? default_entry->external_url : page;

    RegistryClass registry(APPLICATION_SUB_KEY_NAME_URL, false);
    StringClass value_name(page, true);
    value_name += "X";
    registry.Get_String(value_name.Peek_Buffer(), url, url_size, default_url);

    return url[0] != '\0';
}

}

#if !RENEGADE_WITH_GAMESPY

bool cGameSpyAdmin::IsUnderGamespyMenuing = false;
bool cGameSpyAdmin::IsLaunchFromGamespyRequested = false;
bool cGameSpyAdmin::IsLaunchedFromGamespy = false;
bool cGameSpyAdmin::IsServerGamespyListed = false;
uint32_t cGameSpyAdmin::GameHostIp = 0;
uint16_t cGameSpyAdmin::GameHostPort = 0;
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

void cGameSpyAdmin::Set_Game_Host_Ip(uint32_t ip)
{
    GameHostIp = ip;
}

void cGameSpyAdmin::Set_Game_Host_Port(uint16_t port)
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

int32_t CGameSpyQnR::Parse_HeartBeat_List(const char *)
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

void CGameSpyQnR::Enable_Reporting(int32_t enable)
{
    m_GSEnabled = enable;
}

int32_t CGameSpyQnR::IsEnabled(void)
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

void cGameSpyBanList::Ban_User(const char *, const char *, uint32_t)
{
}

bool cGameSpyBanList::Is_User_Banned(const char *, const char *, uint32_t)
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

void CCDKeyAuth::AuthenticateUser(int, uint32_t, char *, char *)
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

#if !defined(_WIN32) || defined(FREEDEDICATEDSERVER)

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
    return true;
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

bool WebBrowser::ShowWebPage(char *page)
{
    char url[512];
    if (!Resolve_Web_Page_URL(page, url, static_cast<int>(sizeof(url)))) {
        return false;
    }

    return LaunchExternal(url);
}

bool WebBrowser::LaunchExternal(const char *url)
{
    if (url == nullptr || url[0] == '\0') {
        return false;
    }

    const bool launched = SDL_OpenURL(url);
    mVisible = false;
    return launched;
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
void WolGameModeClass::Ban_Player(const wchar_t *, uint32_t) {}
bool WolGameModeClass::Is_Banned(const char *, uint32_t) { return false; }
void WolGameModeClass::Read_Kick_List(void) {}
void WolGameModeClass::Auto_Kick(void) {}
void WolGameModeClass::System_Timer_Reset(void) {}
void WolGameModeClass::Set_Quiet_Mode(bool) {}
bool WolGameModeClass::Post_Game_Check(void) { return false; }
void WolGameModeClass::Game_Start_Timeout_Callback(void) {}

#endif
