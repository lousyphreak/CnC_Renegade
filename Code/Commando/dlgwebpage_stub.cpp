#include "DlgWebPage.h"

#include "WebBrowser.h"

#include "renegade_build_config.h"

#if !RENEGADE_WITH_LEGACY_WOL

void DlgWebPage::DoDialog(char *page)
{
    WebBrowser *browser = WebBrowser::CreateInstance(nullptr);
    if (browser != nullptr) {
        browser->ShowWebPage(page);
    }
}

#endif
