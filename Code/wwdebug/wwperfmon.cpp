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

#include "wwperfmon.h"

#include "wwdebug.h"
#include "wwprofile.h"

#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

namespace {

const char *kSectionNames[WWPERF_SECTION_COUNT] = {
	"event",
	"time",
	"input",
	"path",
	"think",
	"dialog",
	"net",
	"gamespy",
	"gs_admin",
	"net_update",
	"render",
	"autorst",
	"console",
	"audio",
	"debug",
	"submit",
	"end_scene",
	"readback"
};

struct PerfFrameStats
{
	double section_ms[WWPERF_SECTION_COUNT] = { 0.0 };
	double frame_ms = 0.0;
	unsigned draw_calls = 0;
	unsigned submitted_vertices = 0;
	unsigned submitted_indices = 0;
	unsigned scene_views = 0;
	unsigned scene_view_flushes = 0;
	unsigned render_target_readbacks = 0;
	double render_target_readback_ms = 0.0;
	unsigned render_target_readback_flushes = 0;
	unsigned fast_submits = 0;
	unsigned slow_submits = 0;
	unsigned slow_submit_lighting = 0;
	unsigned slow_submit_fog = 0;
	unsigned slow_submit_texgen = 0;
	double bgfx_cpu_frame_ms = 0.0;
	double bgfx_gpu_frame_ms = 0.0;
	double bgfx_wait_render_ms = 0.0;
	double bgfx_wait_submit_ms = 0.0;
	unsigned bgfx_draw_count = 0;
};

struct PerfWindowStats
{
	double section_ms[WWPERF_SECTION_COUNT] = { 0.0 };
	double frame_ms_total = 0.0;
	double max_frame_ms = 0.0;
	unsigned frame_count = 0;
	unsigned frames_over_16ms = 0;
	unsigned frames_over_33ms = 0;
	unsigned draw_calls = 0;
	unsigned submitted_vertices = 0;
	unsigned submitted_indices = 0;
	unsigned scene_views = 0;
	unsigned scene_view_flushes = 0;
	unsigned render_target_readbacks = 0;
	double render_target_readback_ms = 0.0;
	unsigned render_target_readback_flushes = 0;
	unsigned fast_submits = 0;
	unsigned slow_submits = 0;
	unsigned slow_submit_lighting = 0;
	unsigned slow_submit_fog = 0;
	unsigned slow_submit_texgen = 0;
	double bgfx_cpu_frame_ms = 0.0;
	double bgfx_gpu_frame_ms = 0.0;
	double bgfx_wait_render_ms = 0.0;
	double bgfx_wait_submit_ms = 0.0;
	unsigned bgfx_draw_count = 0;
};

bool g_configured = false;
bool g_metrics_enabled = false;
bool g_frame_active = false;
bool g_force_swap_interval = false;
int g_forced_swap_interval = 0;
int g_profile_collect_frames = 0;
bool g_profile_collect_started = false;
bool g_profile_collect_stop_pending = false;
unsigned g_profile_runtime_frames = 0;
int g_auto_profile_collect_frames = 0;
int g_auto_profile_trigger_ms = 0;
unsigned g_auto_profile_slow_streak = 0;
unsigned g_auto_profile_required_streak = 0;
bool g_auto_profile_triggered = false;
Uint64 g_ticks_per_second = 0;
double g_ticks_to_ms = 0.0;
Uint64 g_log_interval_ticks = 0;
Uint64 g_last_log_ticks = 0;
Uint64 g_frame_start_ticks = 0;
PerfFrameStats g_frame_stats;
PerfWindowStats g_window_stats;
std::FILE *g_log_file = nullptr;
std::string g_log_path;
std::string g_profile_path;

void Reset_Frame_Stats()
{
	g_frame_stats = PerfFrameStats();
}

void Reset_Window_Stats()
{
	g_window_stats = PerfWindowStats();
}

int Parse_Int_Environment(const char *name, int default_value, bool *is_present = nullptr)
{
	const char *value = std::getenv(name);
	if (is_present != nullptr) {
		*is_present = value != nullptr;
	}
	if (value == nullptr || value[0] == '\0') {
		return default_value;
	}

	char *end = nullptr;
	const long parsed = std::strtol(value, &end, 10);
	if (end == value || (end != nullptr && *end != '\0')) {
		return default_value;
	}

	if (parsed > std::numeric_limits<int>::max()) {
		return std::numeric_limits<int>::max();
	}
	if (parsed < std::numeric_limits<int>::min()) {
		return std::numeric_limits<int>::min();
	}
	return static_cast<int>(parsed);
}

std::string Parse_String_Environment(const char *name, const char *default_value)
{
	const char *value = std::getenv(name);
	if (value == nullptr || value[0] == '\0') {
		return default_value != nullptr ? std::string(default_value) : std::string();
	}
	return value;
}

double Ticks_To_Milliseconds(Uint64 tick_count)
{
	return static_cast<double>(tick_count) * g_ticks_to_ms;
}

void Write_Log_Line(const char *format, ...)
{
	std::FILE *stream = g_log_file != nullptr ? g_log_file : stderr;
	va_list arguments;
	va_start(arguments, format);
	std::vfprintf(stream, format, arguments);
	va_end(arguments);
	std::fflush(stream);
}

void Finish_Profile_Collection()
{
	if (!g_profile_collect_started) {
		return;
	}

	WWProfileManager::End_Collecting(g_profile_path.c_str());
	Write_Log_Line("PERF profile collected frames=%u output=%s\n", g_profile_runtime_frames, g_profile_path.c_str());
	g_profile_collect_started = false;
	g_profile_collect_stop_pending = false;
}

void Begin_Profile_Collection(const char *reason)
{
	if (g_profile_collect_started || g_profile_collect_frames <= 0) {
		return;
	}

	WWProfileManager::Begin_Collecting();
	g_profile_collect_started = true;
	g_profile_collect_stop_pending = false;
	g_profile_runtime_frames = 0;
	Write_Log_Line(
		"PERF profile begin reason=%s frames=%d output=%s\n",
		reason,
		g_profile_collect_frames,
		g_profile_path.c_str());
}

void Write_Window_Report()
{
	if (!g_metrics_enabled || g_window_stats.frame_count == 0) {
		return;
	}

	const double frame_count = static_cast<double>(g_window_stats.frame_count);
	const double average_frame_ms = g_window_stats.frame_ms_total / frame_count;
	const double fps = g_window_stats.frame_ms_total > 0.0
		? (1000.0 * frame_count / g_window_stats.frame_ms_total)
		: 0.0;

	Write_Log_Line(
		"PERF frames=%u fps=%.2f avg_ms=%.3f max_ms=%.3f over16=%u over33=%u draws=%.1f verts=%.1f idx=%.1f views=%.1f view_flush=%u readbacks=%u readback_ms=%.3f fast=%.1f slow=%.1f slow_lit=%.1f slow_fog=%.1f slow_tex=%.1f bgfx_cpu=%.3f bgfx_gpu=%.3f wait_r=%.3f wait_s=%.3f bgfx_draws=%.1f",
		g_window_stats.frame_count,
		fps,
		average_frame_ms,
		g_window_stats.max_frame_ms,
		g_window_stats.frames_over_16ms,
		g_window_stats.frames_over_33ms,
		static_cast<double>(g_window_stats.draw_calls) / frame_count,
		static_cast<double>(g_window_stats.submitted_vertices) / frame_count,
		static_cast<double>(g_window_stats.submitted_indices) / frame_count,
		static_cast<double>(g_window_stats.scene_views) / frame_count,
		g_window_stats.scene_view_flushes,
		g_window_stats.render_target_readbacks,
		g_window_stats.render_target_readback_ms,
		static_cast<double>(g_window_stats.fast_submits) / frame_count,
		static_cast<double>(g_window_stats.slow_submits) / frame_count,
		static_cast<double>(g_window_stats.slow_submit_lighting) / frame_count,
		static_cast<double>(g_window_stats.slow_submit_fog) / frame_count,
		static_cast<double>(g_window_stats.slow_submit_texgen) / frame_count,
		g_window_stats.bgfx_cpu_frame_ms / frame_count,
		g_window_stats.bgfx_gpu_frame_ms / frame_count,
		g_window_stats.bgfx_wait_render_ms / frame_count,
		g_window_stats.bgfx_wait_submit_ms / frame_count,
		static_cast<double>(g_window_stats.bgfx_draw_count) / frame_count);

	for (int section_index = 0; section_index < WWPERF_SECTION_COUNT; ++section_index) {
		const double section_average = g_window_stats.section_ms[section_index] / frame_count;
		if (section_average < 0.001) {
			continue;
		}
		Write_Log_Line(" %s=%.3f", kSectionNames[section_index], section_average);
	}
	Write_Log_Line("\n");
}

void Accumulate_Frame_Into_Window()
{
	for (int section_index = 0; section_index < WWPERF_SECTION_COUNT; ++section_index) {
		g_window_stats.section_ms[section_index] += g_frame_stats.section_ms[section_index];
	}

	g_window_stats.frame_count++;
	g_window_stats.frame_ms_total += g_frame_stats.frame_ms;
	g_window_stats.max_frame_ms = std::max(g_window_stats.max_frame_ms, g_frame_stats.frame_ms);
	g_window_stats.draw_calls += g_frame_stats.draw_calls;
	g_window_stats.submitted_vertices += g_frame_stats.submitted_vertices;
	g_window_stats.submitted_indices += g_frame_stats.submitted_indices;
	g_window_stats.scene_views += g_frame_stats.scene_views;
	g_window_stats.scene_view_flushes += g_frame_stats.scene_view_flushes;
	g_window_stats.render_target_readbacks += g_frame_stats.render_target_readbacks;
	g_window_stats.render_target_readback_ms += g_frame_stats.render_target_readback_ms;
	g_window_stats.render_target_readback_flushes += g_frame_stats.render_target_readback_flushes;
	g_window_stats.fast_submits += g_frame_stats.fast_submits;
	g_window_stats.slow_submits += g_frame_stats.slow_submits;
	g_window_stats.slow_submit_lighting += g_frame_stats.slow_submit_lighting;
	g_window_stats.slow_submit_fog += g_frame_stats.slow_submit_fog;
	g_window_stats.slow_submit_texgen += g_frame_stats.slow_submit_texgen;
	g_window_stats.bgfx_cpu_frame_ms += g_frame_stats.bgfx_cpu_frame_ms;
	g_window_stats.bgfx_gpu_frame_ms += g_frame_stats.bgfx_gpu_frame_ms;
	g_window_stats.bgfx_wait_render_ms += g_frame_stats.bgfx_wait_render_ms;
	g_window_stats.bgfx_wait_submit_ms += g_frame_stats.bgfx_wait_submit_ms;
	g_window_stats.bgfx_draw_count += g_frame_stats.bgfx_draw_count;
	if (g_frame_stats.frame_ms > (1000.0 / 60.0)) {
		g_window_stats.frames_over_16ms++;
	}
	if (g_frame_stats.frame_ms > 33.333) {
		g_window_stats.frames_over_33ms++;
	}
}

} // namespace

void WWPerfMonClass::Configure_From_Environment(void)
{
	if (g_configured) {
		return;
	}
	g_configured = true;

	g_ticks_per_second = SDL_GetPerformanceFrequency();
	if (g_ticks_per_second != 0) {
		g_ticks_to_ms = 1000.0 / static_cast<double>(g_ticks_per_second);
	}

	const int perf_log_enabled = Parse_Int_Environment("RENEGADE_PERF_LOG", 0);
	g_profile_collect_frames = std::max(Parse_Int_Environment("RENEGADE_PERF_PROFILE_FRAMES", 0), 0);
	g_auto_profile_collect_frames = std::max(Parse_Int_Environment("RENEGADE_PERF_AUTO_PROFILE_FRAMES", 0), 0);
	g_auto_profile_trigger_ms = std::max(Parse_Int_Environment("RENEGADE_PERF_AUTO_PROFILE_TRIGGER_MS", 0), 0);
	g_auto_profile_required_streak = static_cast<unsigned>(std::max(Parse_Int_Environment("RENEGADE_PERF_AUTO_PROFILE_STREAK", 30), 1));
	g_metrics_enabled = perf_log_enabled != 0;

	bool swap_present = false;
	g_forced_swap_interval = Parse_Int_Environment("RENEGADE_FORCE_SWAP_INTERVAL", 0, &swap_present);
	g_force_swap_interval = swap_present;

	g_log_path = Parse_String_Environment("RENEGADE_PERF_LOG_PATH", "perf_log.txt");
	g_profile_path = Parse_String_Environment("RENEGADE_PERF_PROFILE_PATH", "profile_log.txt");

	const int log_interval_ms = std::max(Parse_Int_Environment("RENEGADE_PERF_LOG_INTERVAL_MS", 1000), 100);
	g_log_interval_ticks = static_cast<Uint64>((static_cast<double>(log_interval_ms) / 1000.0) * static_cast<double>(g_ticks_per_second));
	g_last_log_ticks = SDL_GetPerformanceCounter();

	if (g_metrics_enabled) {
		g_log_file = std::fopen(g_log_path.c_str(), "wb");
		if (g_log_file == nullptr) {
			WWDEBUG_WARNING(("Failed to open perf log '%s', falling back to stderr\n", g_log_path.c_str()));
		}
		Write_Log_Line(
			"PERF init log_path=%s interval_ms=%d profile_frames=%d auto_profile_frames=%d auto_profile_trigger_ms=%d auto_profile_streak=%u profile_path=%s forced_swap_interval=%s%d\n",
			g_log_path.c_str(),
			log_interval_ms,
			g_profile_collect_frames,
			g_auto_profile_collect_frames,
			g_auto_profile_trigger_ms,
			g_auto_profile_required_streak,
			g_profile_path.c_str(),
			g_force_swap_interval ? "" : "<unset>",
			g_force_swap_interval ? g_forced_swap_interval : 0);
	}
}

void WWPerfMonClass::Notify_Game_Initialized(void)
{
	if (g_profile_collect_frames <= 0 || g_profile_collect_started) {
		return;
	}

	Begin_Profile_Collection("startup");
}

void WWPerfMonClass::Shutdown(void)
{
	if (!g_configured) {
		return;
	}

	if (g_frame_active) {
		End_Frame();
	}

	Write_Window_Report();
	Reset_Window_Stats();
	Finish_Profile_Collection();

	if (g_log_file != nullptr) {
		std::fclose(g_log_file);
		g_log_file = nullptr;
	}
}

bool WWPerfMonClass::Is_Enabled(void)
{
	return g_metrics_enabled;
}

Uint64 WWPerfMonClass::Begin_Scope(void)
{
	if (!g_metrics_enabled || g_ticks_per_second == 0) {
		return 0;
	}
	return SDL_GetPerformanceCounter();
}

void WWPerfMonClass::End_Scope(WWPerfSection section, Uint64 start_ticks)
{
	if (!g_metrics_enabled || start_ticks == 0 || section < 0 || section >= WWPERF_SECTION_COUNT) {
		return;
	}

	const Uint64 end_ticks = SDL_GetPerformanceCounter();
	if (end_ticks < start_ticks) {
		return;
	}

	g_frame_stats.section_ms[section] += Ticks_To_Milliseconds(end_ticks - start_ticks);
}

void WWPerfMonClass::Begin_Frame(void)
{
	if (!g_metrics_enabled) {
		return;
	}

	Reset_Frame_Stats();
	g_frame_start_ticks = SDL_GetPerformanceCounter();
	g_frame_active = true;
}

void WWPerfMonClass::End_Frame(void)
{
	if (!g_metrics_enabled || !g_frame_active) {
		if (g_profile_collect_started) {
			g_profile_runtime_frames++;
			if (g_profile_collect_stop_pending) {
				Finish_Profile_Collection();
			} else if (g_profile_runtime_frames >= static_cast<unsigned>(g_profile_collect_frames)) {
				g_profile_collect_stop_pending = true;
			}
		}
		return;
	}

	const Uint64 end_ticks = SDL_GetPerformanceCounter();
	if (end_ticks >= g_frame_start_ticks) {
		g_frame_stats.frame_ms = Ticks_To_Milliseconds(end_ticks - g_frame_start_ticks);
	}

	if (!g_profile_collect_started &&
		!g_auto_profile_triggered &&
		g_auto_profile_collect_frames > 0 &&
		g_auto_profile_trigger_ms > 0) {
		if (g_frame_stats.frame_ms >= static_cast<double>(g_auto_profile_trigger_ms)) {
			++g_auto_profile_slow_streak;
		} else {
			g_auto_profile_slow_streak = 0;
		}

		if (g_auto_profile_slow_streak >= g_auto_profile_required_streak) {
			g_profile_collect_frames = g_auto_profile_collect_frames;
			g_auto_profile_triggered = true;
			Begin_Profile_Collection("slow_frame");
		}
	}

	Accumulate_Frame_Into_Window();
	g_frame_active = false;

	if (g_profile_collect_started) {
		g_profile_runtime_frames++;
		if (g_profile_collect_stop_pending) {
			Finish_Profile_Collection();
		} else if (g_profile_runtime_frames >= static_cast<unsigned>(g_profile_collect_frames)) {
			g_profile_collect_stop_pending = true;
		}
	}

	if (g_log_interval_ticks > 0 && end_ticks - g_last_log_ticks >= g_log_interval_ticks) {
		Write_Window_Report();
		Reset_Window_Stats();
		g_last_log_ticks = end_ticks;
	}
}

bool WWPerfMonClass::Has_Forced_Swap_Interval(void)
{
	return g_force_swap_interval;
}

int WWPerfMonClass::Get_Forced_Swap_Interval(void)
{
	return g_forced_swap_interval;
}

void WWPerfMonClass::Record_Draw_Call(unsigned count)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.draw_calls += count;
}

void WWPerfMonClass::Record_Submitted_Vertex_Count(unsigned count)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.submitted_vertices += count;
}

void WWPerfMonClass::Record_Submitted_Index_Count(unsigned count)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.submitted_indices += count;
}

void WWPerfMonClass::Record_Scene_View_Allocation(void)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.scene_views++;
}

void WWPerfMonClass::Record_Scene_View_Flush(void)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.scene_view_flushes++;
}

void WWPerfMonClass::Record_Render_Target_Readback(double milliseconds, unsigned frame_flushes)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.section_ms[WWPERF_SECTION_RENDER_TARGET_READBACK] += milliseconds;
	g_frame_stats.render_target_readbacks++;
	g_frame_stats.render_target_readback_ms += milliseconds;
	g_frame_stats.render_target_readback_flushes += frame_flushes;
}

void WWPerfMonClass::Record_Fast_Submit(void)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.fast_submits++;
}

void WWPerfMonClass::Record_Slow_Submit(void)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.slow_submits++;
}

void WWPerfMonClass::Record_Slow_Submit_Reasons(bool lighting, bool fog, bool texgen)
{
	if (!g_metrics_enabled) {
		return;
	}
	if (lighting) {
		g_frame_stats.slow_submit_lighting++;
	}
	if (fog) {
		g_frame_stats.slow_submit_fog++;
	}
	if (texgen) {
		g_frame_stats.slow_submit_texgen++;
	}
}

void WWPerfMonClass::Record_Bgfx_Frame_Timing(
	double cpu_frame_ms,
	double gpu_frame_ms,
	double wait_render_ms,
	double wait_submit_ms,
	unsigned draw_count)
{
	if (!g_metrics_enabled) {
		return;
	}
	g_frame_stats.bgfx_cpu_frame_ms += cpu_frame_ms;
	g_frame_stats.bgfx_gpu_frame_ms += gpu_frame_ms;
	g_frame_stats.bgfx_wait_render_ms += wait_render_ms;
	g_frame_stats.bgfx_wait_submit_ms += wait_submit_ms;
	g_frame_stats.bgfx_draw_count += draw_count;
}
