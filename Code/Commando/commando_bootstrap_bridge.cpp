#include "commando_bootstrap_bridge.h"

#include "gamespyadmin.h"
#include "GameSpy_QnR.h"
#include "gamespyauthmgr.h"
#include "wolgmode.h"
#include "WebBrowser.h"

const char *Renegade_Commando_Bootstrap_Summary()
{
    (void)cGameSpyAdmin::Get_Is_Server_Gamespy_Listed();
    (void)GameSpyQnR.IsEnabled();
    (void)cGameSpyAuthMgr::Describe_Auth_State(GAMESPY_AUTH_STATE_INITIAL);
    (void)WebBrowser::IsWebPageDisplayed();

#if RENEGADE_WITH_GAMESPY
#if RENEGADE_WITH_LEGACY_WOL
    static const char *kSummary = "Commando offline/LAN slice linked (GameSpy=1, WOL=1)";
#else
    static const char *kSummary = "Commando offline/LAN slice linked (GameSpy=1, WOL=0)";
#endif
#else
#if RENEGADE_WITH_LEGACY_WOL
    static const char *kSummary = "Commando offline/LAN slice linked (GameSpy=0, WOL=1)";
#else
    static const char *kSummary = "Commando offline/LAN slice linked (GameSpy=0, WOL=0)";
#endif
#endif
    return kSummary;
}
