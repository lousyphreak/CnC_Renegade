#pragma once

#include <cstdint>

#include "compat/Mss.H"

bool WWAudio_Get_Audio_Info_From_Memory(const void *data, size_t data_len, AILSOUNDINFO *info, uint32_t *duration_ms);
