#pragma once

#include "compat/Mss.H"

bool WWAudio_Get_Audio_Info_From_Memory(const void *data, size_t data_len, AILSOUNDINFO *info, unsigned long *duration_ms);
