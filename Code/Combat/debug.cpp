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

/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/debug.cpp                             $*
 *                                                                                             *
 *                      $Author:: Bhayes                                                      $*
 *                                                                                             *
 *                     $Modtime:: 2/16/02 8:44p                                               $*
 *                                                                                             *
 *                    $Revision:: 90                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "debug.h"

#include "input.h"
#include "mono.h"
#include "registry.h"
#include "ww3d.h"
#include "ww3dtrig.h"
#include "wwphystrig.h"

#include <SDL3/SDL_log.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

int		DebugManager::EnabledDevices;
int		DebugManager::EnabledTypes;
int		DebugManager::EnabledOptions;
bool	DebugManager::EnableFileLogging;
bool	DebugManager::EnableDiagLogging;
bool	DebugManager::LoadDebugScripts;
int		DebugManager::VersionNumber = 0;
bool	DebugManager::IsSlave = false;
bool	DebugManager::AllowCinematicKeys = false;

CriticalSectionClass DebugManager::CriticalSection;

DebugDisplayHandlerClass *DebugManager::DisplayHandler = NULL;
#define DEFAULT_LOGFILE_NAME "_logfile.txt"
LPSTR DebugManager::LOGFILE = DEFAULT_LOGFILE_NAME;
char DebugManager::LogfileNameBuffer[256];

MonoClass ScrollingScreen;
char DefaultRegistryModifier[1024] = { "" };

namespace {

void Debug_Manager_VDisplay(const char *prefix, const char *text, va_list args)
{
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), text, args);
	buffer[sizeof(buffer) - 1] = '\0';

	char output[1152];
	if (prefix != NULL) {
		snprintf(output, sizeof(output), "%s%s\n", prefix, buffer);
	} else {
		snprintf(output, sizeof(output), "%s", buffer);
	}
	output[sizeof(output) - 1] = '\0';
	DebugManager::Display(output);
}

void wwdebug_message_handler(DebugType type, const char *message)
{
	if (!DebugManager::Is_Type_Enabled(static_cast<DebugManager::DebugType>(1 << type))) {
		return;
	}

	if (type == WWDEBUG_TYPE_ERROR) {
		DebugManager::Display("ERROR:");
	} else if (type == WWDEBUG_TYPE_WARNING) {
		DebugManager::Display("WARNING:");
	}

	DebugManager::Display(message);
}

void wwdebug_assert_handler(const char *message)
{
	DebugManager::Display(message);
}

bool wwdebug_trigger_handler(int trigger_num)
{
#ifdef WWDEBUG
	switch (trigger_num) {
		case ' ': return Input::Get_State(INPUT_FUNCTION_DEBUG_SINGLE_STEP_STEP);
		case 'S': return Input::Get_State(INPUT_FUNCTION_DEBUG_SINGLE_STEP);

		case WWDEBUG_TRIGGER_GENERIC0:				return Input::Get_State(INPUT_FUNCTION_DEBUG_GENERIC0);
		case WWDEBUG_TRIGGER_GENERIC1:				return Input::Get_State(INPUT_FUNCTION_DEBUG_GENERIC1);
		case WWPHYS_TRIGGER_COLLISION_DEBUGGING:	return DebugManager::Option_Is_Enabled(DebugManager::DEBUG_COLLISION_MESSAGES);
		case WWPHYS_TRIGGER_COLLISION_DISPLAY:		return DebugManager::Option_Is_Enabled(DebugManager::DEBUG_COLLISION_DISPLAY);
		case WWPHYS_TRIGGER_INVERT_VIS:				return DebugManager::Option_Is_Enabled(DebugManager::DEBUG_INVERT_VIS);
		case WWPHYS_TRIGGER_DISABLE_VIS:			return DebugManager::Option_Is_Enabled(DebugManager::DEBUG_DISABLE_VIS);
		case WW3D_TRIGGER_PROCESS_STATS:			return DebugManager::Option_Is_Enabled(DebugManager::DEBUG_PROCESS_STATS);
		case WW3D_TRIGGER_RENDER_STATS:				return false;
		case WW3D_TRIGGER_SURFACE_CACHE_STATS:		return DebugManager::Option_Is_Enabled(DebugManager::DEBUG_SURFACE_CACHE);
		default:
			Debug_Say(("Unhandled Trigger %d %c\n", trigger_num, trigger_num));
			break;
	}
#endif
	return false;
}

void wwdebug_profile_start_handler(const char *)
{
}

void wwdebug_profile_stop_handler(const char *)
{
}

} // namespace

void	DebugManager::Init( void )
{
	ScrollingScreen.Enable();

	WWDebug_Install_Message_Handler(wwdebug_message_handler);
	WWDebug_Install_Assert_Handler(wwdebug_assert_handler);
	WWDebug_Install_Trigger_Handler(wwdebug_trigger_handler);
	WWDebug_Install_Profile_Start_Handler(wwdebug_profile_start_handler);
	WWDebug_Install_Profile_Stop_Handler(wwdebug_profile_stop_handler);

	EnabledDevices = -1;
	EnabledTypes = -1;
	EnabledOptions = 0;
	EnableFileLogging = false;
	EnableDiagLogging = false;
	LoadDebugScripts = false;
	AllowCinematicKeys = false;

	Disable_Device(DEBUG_DEVICE_SCREEN);
	Disable_Type(DEBUG_TYPE_NETWORK_PROLIFIC);

	Init_Logfile();
	Debug_Say(("\n"));
}

void	DebugManager::Shutdown( void )
{
	WWDebug_Install_Message_Handler(NULL);
	WWDebug_Install_Assert_Handler(NULL);
	WWDebug_Install_Trigger_Handler(NULL);
	WWDebug_Install_Profile_Start_Handler(NULL);
	WWDebug_Install_Profile_Stop_Handler(NULL);

	ScrollingScreen.Disable();
}

void	DebugManager::Update( void )
{
	if (Input::Get_State(INPUT_FUNCTION_MAKE_SCREEN_SHOT)) {
		WW3D::Make_Screen_Shot();
	}

	if (Input::Get_State(INPUT_FUNCTION_TOGGLE_MOVIE_CAPTURE)) {
#ifdef WWDEBUG
		WW3D::Toggle_Movie_Capture();
#endif
	}
}

void	DebugManager::Load_Registry_Settings( const char * sub_key )
{
	RegistryClass registry(sub_key);
	if (registry.Is_Valid()) {
		EnabledDevices = registry.Get_Int("EnabledDevices", EnabledDevices);
		EnabledTypes = registry.Get_Int("EnabledTypes", EnabledTypes);
		EnabledOptions = registry.Get_Int("EnabledOptions", EnabledOptions);
		EnableFileLogging = registry.Get_Bool("EnableFileLogging", EnableFileLogging);
		EnableDiagLogging = registry.Get_Bool("EnableDiagLogging", EnableDiagLogging);
		LoadDebugScripts = registry.Get_Bool("LoadDebugScripts", LoadDebugScripts);
		AllowCinematicKeys = registry.Get_Bool("AllowCinematicKeys", AllowCinematicKeys);
	}
}

void	DebugManager::Save_Registry_Settings( const char * sub_key )
{
	RegistryClass registry(sub_key);
	if (registry.Is_Valid()) {
		registry.Set_Int("EnabledDevices", EnabledDevices);
		registry.Set_Int("EnabledTypes", EnabledTypes);
		registry.Set_Int("EnabledOptions", EnabledOptions);
		registry.Set_Bool("EnableFileLogging", EnableFileLogging);
		registry.Set_Bool("EnableDiagLogging", EnableDiagLogging);
		registry.Set_Bool("LoadDebugScripts", LoadDebugScripts);
		registry.Set_Bool("AllowCinematicKeys", AllowCinematicKeys);
	}
}

void	DebugManager::Display( char const *buffer )
{
	if (buffer == NULL) {
		return;
	}

	CriticalSectionClass::LockClass lock(CriticalSection);

	if (EnabledDevices & DEBUG_DEVICE_SCREEN) {
		Display_Text(buffer);
	}

	if (EnabledDevices & DEBUG_DEVICE_MONO) {
		ScrollingScreen.Printf(buffer);
	}

#ifdef WWDEBUG
	if (EnabledDevices & DEBUG_DEVICE_DBWIN32) {
		WWDebug_DBWin32_Message_Handler(buffer);
	}
#endif

	if (EnabledDevices & DEBUG_DEVICE_LOG) {
		Write_To_File(buffer);
	}

	if (EnabledDevices & DEBUG_DEVICE_WINDOWS) {
		SDL_Log("%s", buffer);
	}
}

void	DebugManager::Display_Script( char const *text, ... )
{
	if (!(EnabledTypes & DEBUG_TYPE_SCRIPT)) {
		return;
	}

	va_list args;
	va_start(args, text);
	Debug_Manager_VDisplay("SCRIPT:", text, args);
	va_end(args);
}

void DebugManager::Display_Network_Admin(char const *text, ...)
{
	if (!(EnabledTypes & DEBUG_TYPE_NETWORK_ADMIN)) {
		return;
	}

	va_list args;
	va_start(args, text);
	Debug_Manager_VDisplay("NET ADMIN:", text, args);
	va_end(args);
}

void DebugManager::Display_Network_Basic(char const *text, ...)
{
	if (!(EnabledTypes & DEBUG_TYPE_NETWORK_BASIC)) {
		return;
	}

	va_list args;
	va_start(args, text);
	Debug_Manager_VDisplay("NET BASIC:", text, args);
	va_end(args);
}

void DebugManager::Display_Network_Prolific(char const *text, ...)
{
	if (!(EnabledTypes & DEBUG_TYPE_NETWORK_PROLIFIC)) {
		return;
	}

	va_list args;
	va_start(args, text);
	Debug_Manager_VDisplay("NET PROLIFIC:", text, args);
	va_end(args);
}

void	DebugManager::Measure_Frame_Textures( void )
{
}

void	DebugManager::Display_Text( const char * string, const Vector4 & color )
{
	if (DisplayHandler != NULL) {
		DisplayHandler->Display_Text(string, color);
	}
}

void	DebugManager::Display_Text( const char * string, const Vector3 & color )
{
	if (DisplayHandler != NULL) {
		DisplayHandler->Display_Text(string, Vector4(color[0], color[1], color[2], 1.0f));
	}
}

void DebugManager::Display_Text( const WideStringClass & string, const Vector4 & color )
{
	if (DisplayHandler != NULL) {
		DisplayHandler->Display_Text(string, color);
	}
}

void DebugManager::Display_Text( const WideStringClass & string, const Vector3 & color )
{
	if (DisplayHandler != NULL) {
		DisplayHandler->Display_Text(string, Vector4(color[0], color[1], color[2], 1.0f));
	}
}

void DebugManager::Init_Logfile(void)
{
	if (IsSlave) {
		snprintf(LogfileNameBuffer, sizeof(LogfileNameBuffer), "%s%s", DefaultRegistryModifier, DEFAULT_LOGFILE_NAME);
		LOGFILE = LogfileNameBuffer;
	}

	FILE *file = fopen(LOGFILE, "wt");
	if (file != NULL) {
		fclose(file);
	}
}

void DebugManager::Write_To_File(LPCSTR str)
{
	FILE *file = fopen(LOGFILE, "at");
	if (file != NULL) {
		fwrite(str, 1, std::strlen(str), file);
		fclose(file);
	}
}

void DebugManager::Enable_Memory_Logging( bool enable )
{
	EnableFileLogging = enable;
}
