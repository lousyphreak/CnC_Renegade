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
 *                 Project Name : WWAudio                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWAudio/SoundBuffer.cpp                      $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 11/01/01 11:00a                                             $*
 *                                                                                             *
 *                    $Revision:: 12                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "SoundBuffer.h"
#include "rawfile.h"
#include "wwdebug.h"
#include "Utils.h"
#include "ffactory.h"
#include "sdlmixer_audio_utils.h"
#include "win.h"
#include "wwprofile.h"

#include <vector>


/////////////////////////////////////////////////////////////////////////////////
//	FileMappingClass
/////////////////////////////////////////////////////////////////////////////////
class FileMappingClass
{
public:
	StringClass			Filename;
	HANDLE				FileMapping;
	int					RefCount;	

	bool operator== (const FileMappingClass &src)	{ return false; }
	bool operator!= (const FileMappingClass &src)	{ return false; }
};

static DynamicVectorClass<FileMappingClass> MappingList;

namespace
{
bool Read_File_Contents(FileClass &file, std::vector<unsigned char> &buffer, int reported_size)
{
	buffer.clear ();

	if (reported_size > 0) {
		buffer.resize (reported_size);
		return (file.Read (buffer.data (), reported_size) == reported_size);
	}

	unsigned char chunk[4096];
	for (;;) {
		int bytes_read = file.Read (chunk, sizeof (chunk));
		if (bytes_read < 0) {
			buffer.clear ();
			return false;
		}
		if (bytes_read == 0) {
			break;
		}

		buffer.insert (buffer.end (), chunk, chunk + bytes_read);
		if (bytes_read < (int)sizeof (chunk)) {
			break;
		}
	}

	return (!buffer.empty ());
}
}



/////////////////////////////////////////////////////////////////////////////////
//
//	SoundBufferClass
//
SoundBufferClass::SoundBufferClass (void)
	: m_Buffer (NULL),
	  m_Length (0),
	  m_Filename (NULL),
	  m_Duration (0),
	  m_Rate (0),
	  m_Bits (0),
	  m_Channels (0),
	  m_Type (WAVE_FORMAT_IMA_ADPCM)
{
	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	~SoundBufferClass
//
SoundBufferClass::~SoundBufferClass (void)
{
	SAFE_FREE (m_Filename);
	Free_Buffer ();
	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Free_Buffer
//
void
SoundBufferClass::Free_Buffer (void)
{
	// Free the buffer's memory
	if (m_Buffer != NULL) {
		delete [] m_Buffer;
		m_Buffer = NULL;
	}

	// Make sure we reset the length
	m_Length = 0L;
	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Determine_Stats
//
void
SoundBufferClass::Determine_Stats (unsigned char *buffer)
{
	WWPROFILE ("Determine_Stats");

	MMSLockClass lock;

	m_Duration = 0;
	m_Rate = 0;
	m_Channels = 0;
	m_Bits = 0;
	m_Type = WAVE_FORMAT_IMA_ADPCM;

	// Attempt to get statistical information about this sound
	AILSOUNDINFO info = { 0 };
	if ((buffer != NULL) && WWAudio_Get_Audio_Info_From_Memory(buffer, m_Length, &info, &m_Duration)) {

		// Cache this information
		m_Rate = info.rate;
		m_Channels = info.channels;
		m_Bits = info.bits;
		m_Type = info.format;
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Set_Filename
//
void
SoundBufferClass::Set_Filename (const char *name)
{
	SAFE_FREE (m_Filename);
	if (name != NULL) {
		m_Filename = ::strdup (name);
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Load_From_File
//
bool
SoundBufferClass::Load_From_File (const char *filename)
{
	WWPROFILE ("SoundBufferClass::Load_From_File");
	WWDEBUG_SAY(( "Loading sound file %s.\r\n", filename));

	// Assume failure
	bool retval = false;

	// Param OK?
	WWASSERT (filename != NULL);
	if (filename != NULL) {

		// Create a file object and pass it onto the appropriate function
		FileClass *file=_TheFileFactory->Get_File(filename);
		if ( file ) {
			retval = Load_From_File(*file);
			_TheFileFactory->Return_File(file);
		}
		file=NULL;
	}

	// Return the true/false result code
	return retval;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Load_From_File
//
bool
SoundBufferClass::Load_From_File (FileClass &file)
{
	WWPROFILE ("SoundBufferClass::Load_From_File");

	MMSLockClass lock;

	// Assume failure
	bool retval = false;

	// Start from scratch
	Free_Buffer ();
	Set_Filename (file.File_Name ());

	// Open the file if necessary
	bool we_opened = false;
	if (file.Is_Open () == false) {
		we_opened = (file.Open () == TRUE);
	}

	std::vector<unsigned char> file_data;
	const int reported_size = file.Size ();
	retval = Read_File_Contents (file, file_data, reported_size);
	if (retval && !file_data.empty ()) {

		m_Length = file_data.size ();
		m_Buffer = new unsigned char[m_Length];
		::memcpy (m_Buffer, file_data.data (), m_Length);
		Determine_Stats (m_Buffer);
	} else {
		Free_Buffer ();
	}

	// Close the file if necessary
	if (we_opened) {
		file.Close ();
	}

	// Return the true/false result code
	return retval;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Load_From_Memory
//
bool
SoundBufferClass::Load_From_Memory
(
	unsigned char *mem_buffer,
	unsigned long size
)
{
	MMSLockClass lock;

	// Assume failure
	bool retval = false;

	// Start from scratch
	Free_Buffer ();
	Set_Filename ("unknown.wav");

	// Params OK?
	WWASSERT (mem_buffer != NULL);
	WWASSERT (size > 0L);
	if ((mem_buffer != NULL) && (size > 0L)) {

		// Allocate a new buffer of the correct length and copy the contents
		// into the buffer
		m_Length = size;
		m_Buffer = new unsigned char[m_Length];
		::memcpy (m_Buffer, mem_buffer, size);
		retval = true;

		// If we failed, free the buffer
		if (retval == false) {
			Free_Buffer ();
		}
		Determine_Stats (m_Buffer);
	}

	// Return the true/false result code
	return retval;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	StreamSoundBufferClass
//
StreamSoundBufferClass::StreamSoundBufferClass (void)	:
	  SoundBufferClass ()
{
	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	~StreamSoundBufferClass
//
StreamSoundBufferClass::~StreamSoundBufferClass (void)
{
	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Free_Buffer
//
void
StreamSoundBufferClass::Free_Buffer (void)
{
	return ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Load_From_File
//
/////////////////////////////////////////////////////////////////////////////////
bool
StreamSoundBufferClass::Load_From_File
(
	HANDLE			/*hfile*/,
	unsigned long	/*size*/,
	unsigned long	/*offset*/
)
{
	WWPROFILE ("StreamSoundBufferClass::Load_From_File");
	return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Load_From_File
//
/////////////////////////////////////////////////////////////////////////////////
bool
StreamSoundBufferClass::Load_From_File (const char *filename)
{
	WWPROFILE ("StreamSoundBufferClass::Load_From_File");
	return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Load_From_File
//
/////////////////////////////////////////////////////////////////////////////////
bool
StreamSoundBufferClass::Load_From_File (FileClass &file)
{
	WWPROFILE ("StreamSoundBufferClass::Load_From_File");

	MMSLockClass lock;

	// Start from scratch
	Free_Buffer ();
	Set_Filename (file.File_Name ());

	// Open the file if necessary
	bool we_opened = false;
	if (file.Is_Open () == false) {
		we_opened = (file.Open () == TRUE);
	}

	const int reported_size = file.Size ();
	m_Length = (reported_size > 0) ? reported_size : 0;

	if (reported_size > 0) {
		std::vector<unsigned char> buffer;
		if (Read_File_Contents(file, buffer, reported_size) && !buffer.empty()) {
			Determine_Stats(buffer.data());
		}
	}

	// Close the file if necessary
	if (we_opened) {
		file.Close ();
	}

	return true;
}
