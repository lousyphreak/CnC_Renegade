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
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : commando                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/dlgconfigaudiotab.cpp                              $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/25/02 7:12p                                                $*
 *                                                                                             *
 *                    $Revision:: 11                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "dlgconfigaudiotab.h"
#include "resource.h"
#include "sliderctrl.h"
#include "checkboxctrl.h"
#include "WWAudio.h"
#include "comboboxctrl.h"
#include "listctrl.h"
#include "_globals.h"
#include "string_ids.h"
#include "translatedb.h"
#include "AudibleSound.h"
#include "dialogmgr.h"
#include "debug.h"


////////////////////////////////////////////////////////////////
//
//	DlgConfigAudioTabClass
//
////////////////////////////////////////////////////////////////
DlgConfigAudioTabClass::DlgConfigAudioTabClass (void)	:
	InitialDeviceIndex (0),
	InitialIsStereo (true),
	InitialHertz (44100),
	InitialBits (16),
	SoundVolumeTestSound (NULL),
	MusicVolumeTestSound (NULL),
	DialogVolumeTestSound (NULL),
	CinematicVolumeTestSound (NULL),
	SoundVolumeTestSoundStartTime (0),
	MusicVolumeTestSoundStartTime (0),
	DialogVolumeTestSoundStartTime (0),
	CinematicVolumeTestSoundStartTime (0),
	ChildDialogClass (IDD_CONFIG_AUDIO)
{
	SoundVolumeTestSound = WWAudioClass::Get_Instance ()->Create_Sound_Effect ("laser_rifle_fire_01.wav");
	if (SoundVolumeTestSound != NULL) {
		SoundVolumeTestSound->Set_Loop_Count (0);
		SoundVolumeTestSound->Set_Type (AudibleSoundClass::TYPE_SOUND_EFFECT);
	}

	MusicVolumeTestSound = WWAudioClass::Get_Instance ()->Create_Sound_Effect ("sakura battle theme.mp3");
	if (MusicVolumeTestSound != NULL) {
		MusicVolumeTestSound->Set_Type (AudibleSoundClass::TYPE_MUSIC);
	}

	DialogVolumeTestSound = WWAudioClass::Get_Instance ()->Create_Sound_Effect ("m00s1_s1s1gbmg_snd.wav");
	if (DialogVolumeTestSound != NULL) {
		DialogVolumeTestSound->Set_Loop_Count (0);
		DialogVolumeTestSound->Set_Type (AudibleSoundClass::TYPE_DIALOG);
	}

	CinematicVolumeTestSound = WWAudioClass::Get_Instance ()->Create_Sound_Effect ("00-n000e.wav");
	if (CinematicVolumeTestSound != NULL) {
		CinematicVolumeTestSound->Set_Loop_Count (0);
		CinematicVolumeTestSound->Set_Type (AudibleSoundClass::TYPE_CINEMATIC);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	~DlgConfigAudioTabClass
//
////////////////////////////////////////////////////////////////
DlgConfigAudioTabClass::~DlgConfigAudioTabClass (void)
{
	if (SoundVolumeTestSound != NULL) {
		SoundVolumeTestSound->Stop ();
		REF_PTR_RELEASE (SoundVolumeTestSound);
	}

	if (MusicVolumeTestSound != NULL) {
		MusicVolumeTestSound->Stop ();
		REF_PTR_RELEASE (MusicVolumeTestSound);
	}
	
	if (DialogVolumeTestSound != NULL) {
		DialogVolumeTestSound->Stop ();
		REF_PTR_RELEASE (DialogVolumeTestSound);
	}

	if (CinematicVolumeTestSound != NULL) {
		CinematicVolumeTestSound->Stop ();
		REF_PTR_RELEASE (CinematicVolumeTestSound);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Init_Dialog
//
////////////////////////////////////////////////////////////////
void
DlgConfigAudioTabClass::On_Init_Dialog (void)
{
	ChildDialogClass::On_Init_Dialog ();

	//
	//	Read the audio library's settings from the registry
	//	
	bool is_stereo			= WWAudioClass::Get_Instance ()->Get_Playback_Stereo ();	
	float sound_vol		= WWAudioClass::Get_Instance ()->Get_Sound_Effects_Volume ();
	float music_vol		= WWAudioClass::Get_Instance ()->Get_Music_Volume ();
	float dialog_vol		= WWAudioClass::Get_Instance ()->Get_Dialog_Volume ();
	float cinematic_vol	= WWAudioClass::Get_Instance ()->Get_Cinematic_Volume ();
	bool sound_on			= WWAudioClass::Get_Instance ()->Are_Sound_Effects_On ();
	bool music_on			= WWAudioClass::Get_Instance ()->Is_Music_On ();
	bool dialog_on			= WWAudioClass::Get_Instance ()->Is_Dialog_On ();
	bool cinematic_on		= WWAudioClass::Get_Instance ()->Is_Cinematic_Sound_On ();
		
	SliderCtrlClass *snd_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_SOUND_EFFECTS_SLIDER);
	SliderCtrlClass *mus_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_MUSIC_SLIDER);
	SliderCtrlClass *dia_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_DIALOG_SLIDER);
	SliderCtrlClass *cin_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_CINEMATIC_SLIDER);	
	
	//
	//	Setup the sound volume controls
	//
	Check_Dlg_Button (IDC_SOUND_EFFECTS_CHECK, sound_on);
	if (snd_vol_slider != NULL) {
		snd_vol_slider->Set_Range (0, 100);
		snd_vol_slider->Set_Pos (static_cast<int>(sound_vol * 100), false);
	}

	//
	//	Setup the music volume controls
	//
	Check_Dlg_Button (IDC_MUSIC_CHECK, music_on);
	if (mus_vol_slider != NULL) {
		mus_vol_slider->Set_Range (0, 100);
		mus_vol_slider->Set_Pos (static_cast<int>(music_vol * 100), false);
	}

	//
	//	Setup the dialog volume controls
	//
	Check_Dlg_Button (IDC_DIALOG_CHECK, dialog_on);
	if (dia_vol_slider != NULL) {
		dia_vol_slider->Set_Range (0, 100);
		dia_vol_slider->Set_Pos (static_cast<int>(dialog_vol * 100), false);
	}

	//
	//	Setup the dialog volume controls
	//
	Check_Dlg_Button (IDC_CINEMATIC_CHECK, cinematic_on);
	if (cin_vol_slider != NULL) {
		cin_vol_slider->Set_Range (0, 100);
		cin_vol_slider->Set_Pos (static_cast<int>(cinematic_vol * 100), false);
	}	

	//
	//	Check the stereo box if necessary
	//
	Check_Dlg_Button (IDC_STEREO_CHECK, is_stereo);
	InitialIsStereo = is_stereo;

	//
	//	Configure the comboboxes and list control
	//	
	Configure_Quality_Combobox ();
	Configure_Rate_Combobox ();
	Configure_Speaker_Combobox ();
	Configure_Driver_List ();
	
	//
	//	Update the enabled state of the volume sliders
	//
	Update_Slider_Enable_State ();	
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
// Update_Slider_Enable_State
//
/////////////////////////////////////////////////////////////////////////////
void
DlgConfigAudioTabClass::Update_Slider_Enable_State (void)
{
	bool sound_enabled		= Is_Dlg_Button_Checked (IDC_SOUND_EFFECTS_CHECK);
	bool music_enabled		= Is_Dlg_Button_Checked (IDC_MUSIC_CHECK);
	bool dialog_enabled		= Is_Dlg_Button_Checked (IDC_DIALOG_CHECK);
	bool cinematic_enabled	= Is_Dlg_Button_Checked (IDC_CINEMATIC_CHECK);

	//
	//	Enable the slider's based on the state of the check boxes
	//
	Enable_Dlg_Item (IDC_SOUND_EFFECTS_SLIDER,	sound_enabled);
	Enable_Dlg_Item (IDC_MUSIC_SLIDER,				music_enabled);
	Enable_Dlg_Item (IDC_DIALOG_SLIDER,				dialog_enabled);
	Enable_Dlg_Item (IDC_CINEMATIC_SLIDER,			cinematic_enabled);	

	//
	//	Update the audio itself
	//
	WWAudioClass::Get_Instance ()->Allow_Sound_Effects (sound_enabled);
	WWAudioClass::Get_Instance ()->Allow_Music (music_enabled);
	WWAudioClass::Get_Instance ()->Allow_Dialog (dialog_enabled);
	WWAudioClass::Get_Instance ()->Allow_Cinematic_Sound (cinematic_enabled);
	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Configure_Rate_Combobox
//
//////////////////////////////////////////////////////////////////////
void	
DlgConfigAudioTabClass::Configure_Rate_Combobox (void)
{
	ComboBoxCtrlClass *combo_box = (ComboBoxCtrlClass *)Get_Dlg_Item (IDC_RATE_COMBO);
	if (combo_box == NULL) {
		return ;
	}

	//
	//	Add an entry for each locale to the combobox
	//
	combo_box->Add_String (TRANSLATE (IDS_SND_RATE_11));
	combo_box->Add_String (TRANSLATE (IDS_SND_RATE_22));
	combo_box->Add_String (TRANSLATE (IDS_SND_RATE_44));

	InitialHertz = WWAudioClass::Get_Instance ()->Get_Playback_Rate ();
	switch (InitialHertz)
	{
		case 11025:
			combo_box->Set_Curr_Sel (0);
			break;

		case 22050:
			combo_box->Set_Curr_Sel (1);
			break;

		default:
		case 44100:
			combo_box->Set_Curr_Sel (2);
			break;
	}

	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Configure_Speaker_Combobox
//
//////////////////////////////////////////////////////////////////////
void	
DlgConfigAudioTabClass::Configure_Speaker_Combobox (void)
{
	ComboBoxCtrlClass *combo_box = (ComboBoxCtrlClass *)Get_Dlg_Item (IDC_SPEAKER_SETUP_COMBO);
	if (combo_box == NULL) {
		return ;
	}

	//
	//	Add an entry for each speaker type to the combobox
	//
	combo_box->Add_String (TRANSLATE (IDS_MENU_SPEAKER_2));
	combo_box->Add_String (TRANSLATE (IDS_MENU_SPEAKER_HEADPHONE));
	combo_box->Add_String (TRANSLATE (IDS_MENU_SPEAKER_SURROUND));
	combo_box->Add_String (TRANSLATE (IDS_MENU_SPEAKER_4));

	//
	//	Select the current setting
	//
	int type = WWAudioClass::Get_Instance ()->Get_Speaker_Type ();
	switch (type)
	{
		default:
		case AIL_3D_2_SPEAKER:
			combo_box->Set_Curr_Sel (0);
			break;

		case AIL_3D_HEADPHONE:
			combo_box->Set_Curr_Sel (1);
			break;

		case AIL_3D_SURROUND:
			combo_box->Set_Curr_Sel (2);
			break;

		case AIL_3D_4_SPEAKER:
			combo_box->Set_Curr_Sel (3);
			break;
	}

	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Configure_Quality_Combobox
//
//////////////////////////////////////////////////////////////////////
void	
DlgConfigAudioTabClass::Configure_Quality_Combobox (void)
{
	ComboBoxCtrlClass *combo_box = (ComboBoxCtrlClass *)Get_Dlg_Item (IDC_QUALITY_COMBO);
	if (combo_box == NULL) {
		return ;
	}

	//
	//	Add an entry for each locale to the combobox
	//
	combo_box->Add_String (TRANSLATE (IDS_SND_8_BIT));
	combo_box->Add_String (TRANSLATE (IDS_SND_16_BIT));

	InitialBits = WWAudioClass::Get_Instance ()->Get_Playback_Bits ();	

	//
	//	Select the appropriate quality combobox entry
	//
	switch (InitialBits)
	{
		case 8:
			combo_box->Set_Curr_Sel (0);
			break;

		default:
		case 16:
			combo_box->Set_Curr_Sel (1);
			break;
	}

	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	On_Command
//
//////////////////////////////////////////////////////////////////////
void	
DlgConfigAudioTabClass::On_Command (int ctrl_id, int message_id, uint32_t param)
{
	switch (ctrl_id)
	{
		case IDC_SOUND_EFFECTS_CHECK:
		case IDC_MUSIC_CHECK:
		case IDC_DIALOG_CHECK:
		case IDC_CINEMATIC_CHECK:
			Update_Slider_Enable_State ();
			break;

		case IDC_DEFAULTS_BUTTON:
			Set_Default_Volumes ();
			break;
	}

	ChildDialogClass::On_Command (ctrl_id, message_id, param);
	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Configure_Driver_List
//
//////////////////////////////////////////////////////////////////////
void	
DlgConfigAudioTabClass::Configure_Driver_List (void)
{
	ListCtrlClass *list_ctrl = (ListCtrlClass *)Get_Dlg_Item (IDC_DRIVER_LIST);
	if (list_ctrl == NULL) {
		InitialDeviceIndex = -1;
		return ;
	}

	//
	//	Get the name of the current 3D driver
	//
	StringClass device_name = WWAudioClass::Get_Instance ()->Get_3D_Driver_Name ();

	//
	//	Configure the column
	//
	list_ctrl->Add_Column (TRANSLATE (IDS_SND_DRIVER_COL), 1.0F, Vector3 (1, 1, 1));

	//
	//	Loop over all the drivers
	//
	bool selected_default = false;
	int driver_count = WWAudioClass::Get_Instance ()->Get_3D_Device_Count ();
	for (int index = 0; index < driver_count; index ++) {
		
		//
		//	Get information about this sound driver
		//
		WWAudioClass::DRIVER_INFO_STRUCT *driver_info = NULL;
		if (WWAudioClass::Get_Instance ()->Get_3D_Device (index, &driver_info)) {

			WideStringClass wide_driver_name;
			wide_driver_name.Convert_From (driver_info->name);

			//
			//	Add an entry to the list for this driver
			//
			int item_index = list_ctrl->Insert_Entry (index, wide_driver_name);
			if (item_index >= 0) {
				list_ctrl->Set_Entry_Data (item_index, 0, reinterpret_cast<uintptr_t>(driver_info->driver));

				//
				//	Select this entry if its the default
				//
				if (::lstrcmpi (device_name, driver_info->name) == 0) {
					list_ctrl->Set_Curr_Sel (item_index);
					selected_default = true;
				}
			}
		}
	}

	//
	//	Select the first entry by default (if necessary)
	//
	if (selected_default == false && list_ctrl->Get_Entry_Count () > 0) {
		list_ctrl->Set_Curr_Sel (0);
	}

	//
	//	Record the initial device index so we don't have
	// to recreate the device if its not necessary
	//
	InitialDeviceIndex = list_ctrl->Get_Curr_Sel ();		
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
// On_Apply
//
/////////////////////////////////////////////////////////////////////////////
bool
DlgConfigAudioTabClass::On_Apply (void)
{
	WWAudioClass *audio = WWAudioClass::Get_Instance ();
	StringClass device_name	= audio->Get_3D_Driver_Name ();
	int hertz					= audio->Get_Playback_Rate ();
	int bits						= audio->Get_Playback_Bits ();
	int speaker_type			= audio->Get_Speaker_Type ();
	bool is_stereo				= audio->Get_Playback_Stereo ();
	float sound_vol			= audio->Get_Sound_Effects_Volume ();
	float music_vol			= audio->Get_Music_Volume ();
	float dialog_vol			= audio->Get_Dialog_Volume ();
	float cinematic_vol		= audio->Get_Cinematic_Volume ();
	bool sound_on				= audio->Are_Sound_Effects_On ();
	bool music_on				= audio->Is_Music_On ();
	bool dialog_on				= audio->Is_Dialog_On ();
	bool cinematic_on			= audio->Is_Cinematic_Sound_On ();
	int device_index			= InitialDeviceIndex;
	bool can_change_3d_device = false;
	bool missing_control		= false;

	SliderCtrlClass *snd_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_SOUND_EFFECTS_SLIDER);
	SliderCtrlClass *mus_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_MUSIC_SLIDER);
	SliderCtrlClass *dia_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_DIALOG_SLIDER);
	SliderCtrlClass *cin_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_CINEMATIC_SLIDER);
	DialogControlClass *sound_check = Get_Dlg_Item (IDC_SOUND_EFFECTS_CHECK);
	DialogControlClass *music_check = Get_Dlg_Item (IDC_MUSIC_CHECK);
	DialogControlClass *dialog_check = Get_Dlg_Item (IDC_DIALOG_CHECK);
	DialogControlClass *cinematic_check = Get_Dlg_Item (IDC_CINEMATIC_CHECK);
	DialogControlClass *stereo_check = Get_Dlg_Item (IDC_STEREO_CHECK);

	//
	//	Get the volume settings
	//
	if (snd_vol_slider != NULL) {
		sound_vol = snd_vol_slider->Get_Pos () / 100.0F;
	} else {
		missing_control = true;
	}

	if (mus_vol_slider != NULL) {
		music_vol = mus_vol_slider->Get_Pos () / 100.0F;
	} else {
		missing_control = true;
	}

	if (dia_vol_slider != NULL) {
		dialog_vol = dia_vol_slider->Get_Pos () / 100.0F;
	} else {
		missing_control = true;
	}

	if (cin_vol_slider != NULL) {
		cinematic_vol = cin_vol_slider->Get_Pos () / 100.0F;
	} else {
		missing_control = true;
	}

	//
	//	Setup the music volume controls
	//
	if (sound_check != NULL && sound_check->As_CheckBoxCtrlClass () != NULL) {
		sound_on = sound_check->As_CheckBoxCtrlClass ()->Get_Check ();
	} else {
		missing_control = true;
	}

	if (music_check != NULL && music_check->As_CheckBoxCtrlClass () != NULL) {
		music_on = music_check->As_CheckBoxCtrlClass ()->Get_Check ();
	} else {
		missing_control = true;
	}

	if (dialog_check != NULL && dialog_check->As_CheckBoxCtrlClass () != NULL) {
		dialog_on = dialog_check->As_CheckBoxCtrlClass ()->Get_Check ();
	} else {
		missing_control = true;
	}

	if (cinematic_check != NULL && cinematic_check->As_CheckBoxCtrlClass () != NULL) {
		cinematic_on = cinematic_check->As_CheckBoxCtrlClass ()->Get_Check ();
	} else {
		missing_control = true;
	}
	
	//
	//	Get the name of the selected device
	//	
	ListCtrlClass *list_ctrl = (ListCtrlClass *)Get_Dlg_Item (IDC_DRIVER_LIST);
	if (list_ctrl != NULL) {
		device_index = list_ctrl->Get_Curr_Sel ();
		if (device_index >= 0) {
			WideStringClass wide_device_name = list_ctrl->Get_Entry_Text (device_index, 0);
			wide_device_name.Convert_To (device_name);
			can_change_3d_device = true;
		}
	} else {
		missing_control = true;
	}

	//
	//	Get the stereo flag from the dialog
	//
	if (stereo_check != NULL && stereo_check->As_CheckBoxCtrlClass () != NULL) {
		is_stereo = stereo_check->As_CheckBoxCtrlClass ()->Get_Check ();
	} else {
		missing_control = true;
	}

	//
	//	Get the playback bit rate from the dialog
	//
	ComboBoxCtrlClass *combo_box = (ComboBoxCtrlClass *)Get_Dlg_Item (IDC_QUALITY_COMBO);
	if (combo_box != NULL) {
		int quality_cursel = combo_box->Get_Curr_Sel ();
		if (quality_cursel == 0) {
			bits = 8;
		} else if (quality_cursel == 1) {
			bits = 16;
		}
	} else {
		missing_control = true;
	}

	//
	//	Get the playback rate from the controls
	//
	combo_box = (ComboBoxCtrlClass *)Get_Dlg_Item (IDC_RATE_COMBO);
	if (combo_box != NULL) {
		int rate_cursel = combo_box->Get_Curr_Sel ();
		if (rate_cursel == 0) {
			hertz = 11025;
		} else if (rate_cursel == 1) {
			hertz = 22050;
		} else if (rate_cursel == 2) {
			hertz = 44100;
		}
	} else {
		missing_control = true;
	}

	//
	//	Get the playback rate from the controls
	//
	combo_box = (ComboBoxCtrlClass *)Get_Dlg_Item (IDC_SPEAKER_SETUP_COMBO);
	if (combo_box != NULL) {
		int speaker_cursel = combo_box->Get_Curr_Sel ();
		if (speaker_cursel >= 0) {
			speaker_type = speaker_cursel;
		}
	} else {
		missing_control = true;
	}

	if (missing_control) {
		Debug_Say (("DlgConfigAudioTabClass::On_Apply: one or more audio controls were unavailable; preserving current audio settings for those controls.\n"));
	}

	//
	//	Store these settings in the registry
	//
	audio->Save_To_Registry (APPLICATION_SUB_KEY_NAME_SOUND,
												device_name, is_stereo, bits, hertz, sound_on,	
												music_on, dialog_on, cinematic_on, sound_vol, music_vol,
												dialog_vol, cinematic_vol, speaker_type);

	//
	//	Don't change drivers unless necessary
	//
	bool reopen_audio_device = (InitialBits != bits || InitialHertz != hertz || is_stereo != InitialIsStereo);
	bool change_3d_device = (can_change_3d_device && InitialDeviceIndex != device_index);
	if (reopen_audio_device || change_3d_device) {
		audio->Open_2D_Device (is_stereo, bits, hertz);
		if (device_name.Get_Length () > 0) {
			audio->Select_3D_Device (device_name);
		}
	}

	//
	//	Apply the simple changes
	//
	audio->Set_Sound_Effects_Volume (sound_vol);
	audio->Set_Music_Volume (music_vol);
	audio->Set_Dialog_Volume (dialog_vol);
	audio->Set_Cinematic_Volume (cinematic_vol);
	audio->Allow_Sound_Effects (sound_on);
	audio->Allow_Music (music_on);
	audio->Allow_Dialog (dialog_on);
	audio->Allow_Cinematic_Sound (cinematic_on);
	audio->Set_Speaker_Type (speaker_type);
	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
// On_SliderCtrl_Pos_Changed
//
/////////////////////////////////////////////////////////////////////////////
void
DlgConfigAudioTabClass::On_SliderCtrl_Pos_Changed
(
	SliderCtrlClass *	slider_ctrl,
	int					ctrl_id,
	int					new_pos
)
{
	if (ctrl_id == IDC_SOUND_EFFECTS_SLIDER) {
		
		//
		//	Update the sound effects volume...
		//
		float sound_vol = slider_ctrl->Get_Pos () / 100.0F;
		WWAudioClass::Get_Instance ()->Set_Sound_Effects_Volume (sound_vol);

		//
		//	Start the sound effect sound playing
		//
		if (	SoundVolumeTestSound != NULL &&
				(SoundVolumeTestSound->Is_Playing () == false || ((DialogMgrClass::Get_Time () - SoundVolumeTestSoundStartTime) > 150)))
		{			
			SoundVolumeTestSound->Stop ();
			SoundVolumeTestSound->Set_Loop_Count (0);
			SoundVolumeTestSound->Play ();

			//
			//	Record a time stamp for this sound...
			//
			SoundVolumeTestSoundStartTime = DialogMgrClass::Get_Time ();
		}

	} else if (ctrl_id == IDC_MUSIC_SLIDER) {

		//
		//	Update the music volume...
		//
		float music_vol = slider_ctrl->Get_Pos () / 100.0F;
		WWAudioClass::Get_Instance ()->Set_Music_Volume (music_vol);

		//
		//	Start the music sound playing
		//
		if (MusicVolumeTestSound != NULL && MusicVolumeTestSound->Is_Playing () == false) {
			MusicVolumeTestSound->Play ();
		}

		//
		//	Record a time stamp for this sound...
		//
		MusicVolumeTestSoundStartTime = DialogMgrClass::Get_Time ();
	} else if (ctrl_id == IDC_DIALOG_SLIDER) {
		
		//
		//	Update the dialog volume...
		//
		float dialog_vol = slider_ctrl->Get_Pos () / 100.0F;
		WWAudioClass::Get_Instance ()->Set_Dialog_Volume (dialog_vol);

		//
		//	Start the dialog effect sound playing
		//
		if (DialogVolumeTestSound != NULL) {
			DialogVolumeTestSound->Set_Loop_Count (0);
			DialogVolumeTestSound->Play ();

			//
			//	Record a time stamp for this sound...
			//
			DialogVolumeTestSoundStartTime = DialogMgrClass::Get_Time ();
		}

	} else if (ctrl_id == IDC_CINEMATIC_SLIDER) {
		
		//
		//	Update the dialog volume...
		//
		float cinematic_vol = slider_ctrl->Get_Pos () / 100.0F;
		WWAudioClass::Get_Instance ()->Set_Cinematic_Volume (cinematic_vol);

		//
		//	Start the dialog effect sound playing
		//
		if (CinematicVolumeTestSound != NULL) {
			CinematicVolumeTestSound->Set_Loop_Count (0);
			CinematicVolumeTestSound->Play ();

			//
			//	Record a time stamp for this sound...
			//
			CinematicVolumeTestSoundStartTime = DialogMgrClass::Get_Time ();
		}
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
// On_Frame_Update
//
/////////////////////////////////////////////////////////////////////////////
void
DlgConfigAudioTabClass::On_Frame_Update (void)
{
	int curr_time = DialogMgrClass::Get_Time ();

	//
	//	Stop the sound volume test sound if necessary
	//
	if (SoundVolumeTestSound != NULL && (curr_time - SoundVolumeTestSoundStartTime) > 500) {
		SoundVolumeTestSound->Set_Loop_Count (1);
	}

	//
	//	Stop the sound volume test sound if necessary
	//
	if (MusicVolumeTestSound != NULL && (curr_time - MusicVolumeTestSoundStartTime) > 2000) {
		MusicVolumeTestSound->Stop ();
	}

	//
	//	Stop the dialog test sound if necessary
	//
	if (DialogVolumeTestSound != NULL && (curr_time - DialogVolumeTestSoundStartTime) > 2000) {
		DialogVolumeTestSound->Set_Loop_Count (1);
	}

	//
	//	Stop the cinematic test sound if necessary
	//
	if (CinematicVolumeTestSound != NULL && (curr_time - CinematicVolumeTestSoundStartTime) > 2000) {
		CinematicVolumeTestSound->Set_Loop_Count (1);
	}
	
	ChildDialogClass::On_Frame_Update ();	
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
// Set_Default_Volumes
//
/////////////////////////////////////////////////////////////////////////////
void
DlgConfigAudioTabClass::Set_Default_Volumes (void)
{
	int defaultmusicvolume, defaultsoundvolume, defaultdialogvolume, defaultcinematicvolume;

	SliderCtrlClass *snd_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_SOUND_EFFECTS_SLIDER);
	SliderCtrlClass *mus_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_MUSIC_SLIDER);
	SliderCtrlClass *dia_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_DIALOG_SLIDER);
	SliderCtrlClass *cin_vol_slider = (SliderCtrlClass *)Get_Dlg_Item (IDC_CINEMATIC_SLIDER);	
  
	// IML: Get the default settings from the audio system.
	WWAudioClass::Get_Instance()->Load_Default_Volume (defaultmusicvolume, defaultsoundvolume, defaultdialogvolume, defaultcinematicvolume);

	bool missing_slider = (snd_vol_slider == NULL || mus_vol_slider == NULL || dia_vol_slider == NULL || cin_vol_slider == NULL);
	if (missing_slider) {
		Debug_Say (("DlgConfigAudioTabClass::Set_Default_Volumes: one or more volume sliders were unavailable.\n"));
	}

	if (snd_vol_slider != NULL) {
		snd_vol_slider->Set_Pos (defaultsoundvolume);
	}

	if (mus_vol_slider != NULL) {
		mus_vol_slider->Set_Pos (defaultmusicvolume);
	}

	if (dia_vol_slider != NULL) {
		dia_vol_slider->Set_Pos (defaultdialogvolume);
	}

	if (cin_vol_slider != NULL) {
		cin_vol_slider->Set_Pos (defaultcinematicvolume);
	}
	
	return ;
}
