/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef WWPERFMON_H
#define WWPERFMON_H

#include <SDL3/SDL_stdinc.h>

enum WWPerfSection
{
	WWPERF_SECTION_EVENT_PUMP = 0,
	WWPERF_SECTION_TIME_UPDATE,
	WWPERF_SECTION_INPUT,
	WWPERF_SECTION_PATHFIND,
	WWPERF_SECTION_THINK,
	WWPERF_SECTION_DIALOG,
	WWPERF_SECTION_NETWORK,
	WWPERF_SECTION_GAMESPY,
	WWPERF_SECTION_GAMESPY_ADMIN,
	WWPERF_SECTION_NETWORK_UPDATE,
	WWPERF_SECTION_RENDER,
	WWPERF_SECTION_AUTO_RESTART,
	WWPERF_SECTION_CONSOLE,
	WWPERF_SECTION_AUDIO,
	WWPERF_SECTION_DEBUG,
	WWPERF_SECTION_SUBMIT_TRIANGLES,
	WWPERF_SECTION_END_SCENE,
	WWPERF_SECTION_RENDER_TARGET_READBACK,
	WWPERF_SECTION_COUNT
};

class WWPerfMonClass
{
public:
	static void Configure_From_Environment(void);
	static void Notify_Game_Initialized(void);
	static void Shutdown(void);

	static bool Is_Enabled(void);
	static Uint64 Begin_Scope(void);
	static void End_Scope(WWPerfSection section, Uint64 start_ticks);

	static void Begin_Frame(void);
	static void End_Frame(void);

	static bool Has_Forced_Swap_Interval(void);
	static int Get_Forced_Swap_Interval(void);

	static void Record_Draw_Call(unsigned count = 1);
	static void Record_Submitted_Vertex_Count(unsigned count);
	static void Record_Submitted_Index_Count(unsigned count);
	static void Record_Scene_View_Allocation(void);
	static void Record_Scene_View_Flush(void);
	static void Record_Render_Target_Readback(double milliseconds, unsigned frame_flushes);
	static void Record_Fast_Submit(void);
	static void Record_Slow_Submit(void);
	static void Record_Slow_Submit_Reasons(bool lighting, bool fog, bool texgen);
	static void Record_Bgfx_Frame_Timing(
		double cpu_frame_ms,
		double gpu_frame_ms,
		double wait_render_ms,
		double wait_submit_ms,
		unsigned draw_count);
};

#endif
