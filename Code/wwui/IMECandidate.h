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

/******************************************************************************
*
* FILE
*     $Archive: /Commando/Code/wwui/IMECandidate.h $
*
* DESCRIPTION
*
* PROGRAMMER
*     $Author: Denzil_l $
*
* VERSION INFO
*     $Revision: 4 $
*     $Modtime: 1/11/02 6:44p $
*
******************************************************************************/

#ifndef __IMECANDIDATE_H__
#define __IMECANDIDATE_H__

#include <cstdint>

#include "Notify.h"
#include "win.h"

#if defined(_WIN32)
#include <imm.h>

#pragma warning(disable : 4514)

#if defined(_MSC_VER)
#pragma warning(push, 3)
#endif

#include <vector>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace IME {

class IMECandidate
	{
	public:
		IMECandidate();
		~IMECandidate();

		void Open(int index, HWND hwnd, uint32_t codepage, bool unicode, bool startFrom1);
		void Read(void);
		void Close(void);

		bool IsValid(void) const;

		int GetIndex(void) const;

		uint32_t GetStyle(void) const;

		// Get the index of the first candidate in the page
		uint32_t GetPageStart(void) const;

		// Set the page to start with the specified candidate index
		void SetPageStart(uint32_t);

		// Get the number of candidates per page
		uint32_t GetPageSize(void) const;

		// Get the total number of candidates in the list.
		uint32_t GetCount(void) const;

		// Get the index of the current candidate selection
		uint32_t GetSelection(void) const;

		// Get the specified candidate string
		const wchar_t* GetCandidate(uint32_t index);

		// Select a candidate from the list.
		void SelectCandidate(uint32_t index);

		// Set the candidate page view
		void SetView(uint32_t topIndex, uint32_t bottomIndex);

		// Check if the candidates should be displayed starting from 1 or 0
		bool IsStartFrom1(void) const;

	private:
		int mIndex;
		HWND mHWND;
		uint32_t mCodePage;
		bool mUseUnicode;
		bool mStartFrom1;

		uint32_t mCandidateSize;
		CANDIDATELIST* mCandidates;

		// Multibyte -> Unicode string conversion buffer
		wchar_t mTempString[80];
	};


typedef enum
	{
	CANDIDATE_OPEN = 1,
	CANDIDATE_CHANGE,
	CANDIDATE_CLOSE
	} CandidateAction;

typedef TypedActionPtr<CandidateAction, IMECandidate> CandidateEvent;

typedef std::vector<IMECandidate> IMECandidateCollection;

} // namespace IME

#else

#include <vector>

namespace IME {

class IMECandidate
	{
	public:
		IMECandidate() = default;
		~IMECandidate() = default;

		void Open(int, HWND, uint32_t, bool, bool) {}
		void Read(void) {}
		void Close(void) {}

		bool IsValid(void) const { return false; }
		int GetIndex(void) const { return -1; }
		uint32_t GetStyle(void) const { return 0; }
		uint32_t GetPageStart(void) const { return 0; }
		void SetPageStart(uint32_t) {}
		uint32_t GetPageSize(void) const { return 0; }
		uint32_t GetCount(void) const { return 0; }
		uint32_t GetSelection(void) const { return 0; }
		const wchar_t* GetCandidate(uint32_t) { return L""; }
		void SelectCandidate(uint32_t) {}
		void SetView(uint32_t, uint32_t) {}
		bool IsStartFrom1(void) const { return true; }
	};

typedef enum
	{
	CANDIDATE_OPEN = 1,
	CANDIDATE_CHANGE,
	CANDIDATE_CLOSE
	} CandidateAction;

typedef TypedActionPtr<CandidateAction, IMECandidate> CandidateEvent;

typedef std::vector<IMECandidate> IMECandidateCollection;

} // namespace IME

#endif

#endif // __IMECANDIDATE_H__

