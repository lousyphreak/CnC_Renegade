#include "BINKMovie.h"

#include <cstdio>

namespace {
bool g_bink_stub_logged = false;

void Log_Bink_Stub_Message()
{
    if (!g_bink_stub_logged) {
        std::fputs("BINKMovie: Bink playback is unavailable in this build.\n", stderr);
        g_bink_stub_logged = true;
    }
}
}

void BINKMovie::Play(const char *, const char *, FontCharsClass *)
{
    Log_Bink_Stub_Message();
}

void BINKMovie::Stop()
{
}

void BINKMovie::Update()
{
}

void BINKMovie::Render()
{
}

void BINKMovie::Init()
{
}

void BINKMovie::Shutdown()
{
}

bool BINKMovie::Is_Complete()
{
    return true;
}
