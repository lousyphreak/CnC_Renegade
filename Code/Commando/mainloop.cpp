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
 *                     $Archive:: /Commando/Code/Commando/mainloop.cpp                        $*
 *                                                                                             *
 *                      $Author:: Tom_s                                                        $*
 *                                                                                             *
 *                     $Modtime:: 2/21/02 3:13p                                               $*
 *                                                                                             *
 *                    $Revision:: 77                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "mainloop.h"
#include "init.h"
#include "shutdown.h"
#include "timemgr.h"
#include "input.h"
#include "gamemode.h"
#include "debug.h"
#include "msgloop.h"
#include "wwlib_debug.h"
#include "wwprofile.h"
#include "cnetwork.h"
#include "miscutil.h"
//#include "gamesettings.h"
#include "WWAudio.h"
#include "devoptions.h"
#include "multihud.h"
#include "gamedata.h"
#include "diagnostics.h"
#include "wwprofile.h"
#include "crandom.h"
#include "dialogmgr.h"
#include "ccamera.h"
#include "pathmgr.h"
#include "networkobjectmgr.h"
#include "WebBrowser.h"
#include "AutoStart.h"
#include "gameinitmgr.h"
#include "servercontrol.h"
#include "ConsoleMode.h"
#include "gamespyadmin.h"
#include "demosupport.h"
#include "GameSpy_QnR.h"
#include "ww3d.h"
#include "wwperfmon.h"


/*
**
*/
bool	RunMainLoop = true;
int		ExitCode = EXIT_SUCCESS;

void Stop_Main_Loop(int exitCode)
{
	RunMainLoop = false;
	ExitCode = exitCode;
}


void _Game_Main_Loop_Loop(void)
{
	WWPROFILE( "Main Loop" );
	WWPerfMonClass::Begin_Frame();

	{
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		Windows_Message_Handler();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_EVENT_PUMP, start_ticks);
	}

	uint32_t time1 = TIMEGETTIME();

	{
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		TimeManager::Update();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_TIME_UPDATE, start_ticks);
	}

	{
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		Input::Update();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_INPUT, start_ticks);
	}


{	WWPROFILE( "Pathfind Evaluate" );
	const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
   if (COMBAT_CAMERA != NULL) {
		Vector3 camera_pos = COMBAT_CAMERA->Get_Position();
		PathMgrClass::Resolve_Paths( camera_pos );
	}
	WWPerfMonClass::End_Scope(WWPERF_SECTION_PATHFIND, start_ticks);
}

{	WWPROFILE( "Think" );
	const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
   GameModeManager::Think();
	GameInitMgrClass::Think();
	WWPerfMonClass::End_Scope(WWPERF_SECTION_THINK, start_ticks);
}

{	WWPROFILE( "Dialog Mgr Update" );
	const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
   DialogMgrClass::On_Frame_Update ();
	WWPerfMonClass::End_Scope(WWPERF_SECTION_DIALOG, start_ticks);
}

{	WWPROFILE( "Network Object Mgr Think" );
	const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
   NetworkObjectMgrClass::Think ();
	ServerControl.Service();
	WWPerfMonClass::End_Scope(WWPERF_SECTION_NETWORK, start_ticks);
}

{	WWPROFILE("GameSpy_QnR");
	const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
	GameSpyQnR.Think();
	WWPerfMonClass::End_Scope(WWPERF_SECTION_GAMESPY, start_ticks);
}

	if (cGameSpyAdmin::Is_Gamespy_Game()) {
		WWPROFILE( "cGameSpyAdmin Think" );
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		cGameSpyAdmin::Think();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_GAMESPY_ADMIN, start_ticks);
	}

	//
	// If the following assert hits it may indicate that your
	// working directory pathname got cleared in the project settings.
	//
	WWASSERT(GameModeManager::Find("Combat") != NULL);

	if (!GameModeManager::Find("Combat")->Is_Active()) {
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		cNetwork::Update();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_NETWORK_UPDATE, start_ticks);
	}

	// Denzil - Embedded browser
	if (WebBrowser::IsWebPageDisplayed() == false) {
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		GameModeManager::Render();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_RENDER, start_ticks);
	}

	if (AutoRestart.Is_Active()) {
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		AutoRestart.Think();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_AUTO_RESTART, start_ticks);
	}

{	WWPROFILE("ConsoleBox");
	const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
	ConsoleBox.Think();
	WWPerfMonClass::End_Scope(WWPERF_SECTION_CONSOLE, start_ticks);
}

	DEMO_SECURITY_CHECK;

{	WWPROFILE( "Audio" );
	const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
	if (!ConsoleBox.Is_Exclusive()) {
		WWAudioClass::Get_Instance ()->On_Frame_Update (0);
	}
	WWPerfMonClass::End_Scope(WWPERF_SECTION_AUDIO, start_ticks);
}
	// Give the sound manager a chance to think
  // PROFILE(	"Audio", WWAudioClass::Get_Instance ()->On_Frame_Update (0) );

#ifdef WWDEBUG
   // Sometimes it is useful to be able to artificially lower the frame rate
   Sleep(cDevOptions::DesiredFrameSleepMs.Get());
#endif

#if 0
{	WWPROFILE( "Random" );
	// spin the Random Generator, a little
	int count = FreeRandom.Get_Int( 5 );
	while ( count-- > 0 ) {
		FreeRandom.Get_Int();
	}
}
#endif

	{
		const Uint64 start_ticks = WWPerfMonClass::Begin_Scope();
		DebugManager::Update();
		WWPerfMonClass::End_Scope(WWPERF_SECTION_DEBUG, start_ticks);
	}


	/*
	** Sleep for a while if we are hogging the CPU.
	*/
	if (cNetwork::I_Am_Only_Server()) {
		uint32_t time2 = TIMEGETTIME();
		if (time2 >= time1) {

			/*
			** 16 (approx) for 60 fps. (1000/60)
			*/
			uint32_t diff = time2 - time1;
			if (diff < 16) {
				uint32_t sleep_time = 16 - (time2 - time1);
				Sleep(sleep_time);
			}
		}
	}

	WWPerfMonClass::End_Frame();
}

/*
** MAIN GAME LOOP
*/
int Game_Main_Loop(void)
{
	const uint32_t servicetime = 1000; // Time in milliseconds.

	uint32_t time;
	WWLib_Debug_Printf("MainLoop: Starting Game_Init()\n");

	// Only run main loop if the init is succesful!
	if (Game_Init()) {
		if (WWPerfMonClass::Has_Forced_Swap_Interval()) {
			WW3D::Set_Ext_Swap_Interval(WWPerfMonClass::Get_Forced_Swap_Interval());
			WWLib_Debug_Printf("MainLoop: Forced swap interval=%d\n", WWPerfMonClass::Get_Forced_Swap_Interval());
		}
		WWPerfMonClass::Notify_Game_Initialized();
		WWLib_Debug_Printf("MainLoop: Entering main loop\n");

		while ( RunMainLoop ) {
			_Game_Main_Loop_Loop();
		}

		WWLib_Debug_Printf("MainLoop: Exiting main loop\n");

		// IML: Allow a short period to process any outstanding sound effects before shutdown.
		time = TIMEGETTIME();
		while (TIMEGETTIME() - time < servicetime) {
			WWAudioClass::Get_Instance ()->On_Frame_Update (0);
		}

		Game_Shutdown();
	}

	return ExitCode;
}
