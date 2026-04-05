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
*     $Archive: /Commando/Code/wwui/IMEManager.h $
*
* DESCRIPTION
*     Input Method Editor Manager for input of far east characters.
*
* PROGRAMMER
*     $Author: Denzil_l $
*
* VERSION INFO
*     $Revision: 3 $
*     $Modtime: 1/08/02 8:38p $
*
******************************************************************************/

#ifndef __IMEMANAGER_H__
#define __IMEMANAGER_H__

#include <cstdint>

#include "refcount.h"
#include "IMECandidate.h"
#include "Notify.h"
#include "widestring.h"
#include "win.h"

#if defined(_WIN32)
#include <imm.h>

namespace IME {

#define IME_MAX_STRING_LEN 255
#define IME_MAX_TYPING_LEN 80

class IMEManager;

typedef enum
	{
	IME_ACTIVATED = 1,
	IME_DEACTIVATED,
	IME_LANGUAGECHANGED,
	IME_GUIDELINE,
	IME_ENABLED,
	IME_DISABLED,
	} IMEAction;

typedef TypedActionPtr<IMEAction, IMEManager> IMEEvent;

typedef enum
	{
	COMPOSITION_INVALID = 0,
	COMPOSITION_TYPING,
	COMPOSITION_START,
	COMPOSITION_CHANGE,
	COMPOSITION_FULL,
	COMPOSITION_END,
	COMPOSITION_CANCEL,
	COMPOSITION_RESULT
	} CompositionAction;

typedef TypedActionPtr<CompositionAction, IMEManager> CompositionEvent;

class UnicodeType;
typedef TypedEvent<UnicodeType, wchar_t> UnicodeChar;

class IMEManager :
		public RefCountClass,
		public Notifier<IMEEvent>,
		public Notifier<UnicodeChar>,
		public Notifier<CompositionEvent>,
		public Notifier<CandidateEvent>
	{
	public:
		static IMEManager* Create(HWND hwnd);

		void Activate(void);
		void Deactivate(void);
		bool IsActive(void) const;

		void Disable(void);
		void Enable(void);
		bool IsDisabled(void) const;

		const wchar_t* GetDescription(void) const
			{return mIMEDescription;}

		uint16_t GetLanguageID(void) const
			{return mLangID;}

		uint32_t GetCodePage(void) const
			{return mCodePage;}

		const wchar_t* GetResultString(void) const
			{return mResultString;}

		const wchar_t* GetCompositionString(void) const
			{return mCompositionString;}

		int32_t GetCompositionCursorPos(void) const
			{return mCompositionCursorPos;}

		const wchar_t* GetReadingString(void) const
			{return mReadingString;}

		#ifdef SHOW_IME_TYPING
		const wchar_t* GetTypingString(void) const
			{return mTypingString;}
		#endif

		void GetTargetClause(uint32_t& start, uint32_t& end);

		bool GetCompositionFont(LPLOGFONT lpFont);

		const IMECandidateCollection GetCandidateColl(void) const
			{return mCandidateColl;}

		uint32_t GetGuideline(wchar_t* outString, int length);

		bool ProcessMessage(HWND hwnd, uint32_t msg, uintptr_t wParam, intptr_t lParam, intptr_t& result);

	protected:
		IMEManager();
		virtual ~IMEManager();

		bool FinalizeCreate(HWND hwnd);

		intptr_t IMENotify(uintptr_t wParam, intptr_t lParam);
		
		HKL InputLanguageChangeRequest(HKL hkl);
		void InputLanguageChanged(HKL hkl);

		void ResetComposition(void);
		void StartComposition(void);
		void DoComposition(uint32_t dbcsChar, int32_t changeFlag);
		void EndComposition(void);

		bool ReadCompositionString(HIMC imc, uint32_t flag, wchar_t* buffer, int length);
		int32_t ReadReadingAttr(HIMC imc, uint8_t* attr, int length);
		int32_t ReadReadingClause(HIMC imc, uint32_t* clause, int length);
		int32_t ReadCompositionAttr(HIMC imc, uint8_t* attr, int length);
		int32_t ReadCompositionClause(HIMC imc, uint32_t* clause, int length);
		int32_t ReadCursorPos(HIMC imc);

		void OpenCandidate(uint32_t candList);
		void ChangeCandidate(uint32_t candList);
		void CloseCandidate(uint32_t candList);

		bool IMECharHandler(uint16_t dbcs);
		bool CharHandler(uint16_t ch);

		int32_t ConvertAttrForUnicode(uint8_t* mbcs, uint8_t* attr);
		int32_t ConvertClauseForUnicode(uint8_t* mbcs, int32_t length, uint32_t* clause);

		DECLARE_NOTIFIER(IMEEvent)
		DECLARE_NOTIFIER(UnicodeChar)
		DECLARE_NOTIFIER(CompositionEvent)
		DECLARE_NOTIFIER(CandidateEvent)

		// Prevent copy and assignment
		IMEManager(const IMEManager&);
		const IMEManager& operator=(const IMEManager&);

	private:
		HWND mHWND;
		HIMC mDefaultHIMC;
		HIMC mHIMC;

		HIMC mDisabledHIMC;
		uint32_t mDisableCount;

		uint16_t mLangID;
		uint32_t mCodePage;
		WideStringClass mIMEDescription;
		uint32_t mIMEProperties;

		bool mHilite;
		bool mStartCandListFrom1;
		bool mOSCanUnicode;
		bool mUseUnicode;
		bool mInComposition;

		#ifdef SHOW_IME_TYPING
		wchar_t mTypingString[IME_MAX_TYPING_LEN];
		int32_t mTypingCursorPos;
		#endif
		
		wchar_t mCompositionString[IME_MAX_STRING_LEN];
		uint8_t mCompositionAttr[IME_MAX_STRING_LEN];
		uint32_t mCompositionClause[IME_MAX_STRING_LEN / 2];

		int32_t mCompositionCursorPos;

		wchar_t mReadingString[IME_MAX_STRING_LEN * 2];
		wchar_t mResultString[IME_MAX_STRING_LEN];

		IMECandidateCollection mCandidateColl;
	};

} // namespace IME

#else

namespace IME {

#define IME_MAX_STRING_LEN 255
#define IME_MAX_TYPING_LEN 80

class IMEManager;

typedef enum
	{
	IME_ACTIVATED = 1,
	IME_DEACTIVATED,
	IME_LANGUAGECHANGED,
	IME_GUIDELINE,
	IME_ENABLED,
	IME_DISABLED,
	} IMEAction;

typedef TypedActionPtr<IMEAction, IMEManager> IMEEvent;

typedef enum
	{
	COMPOSITION_INVALID = 0,
	COMPOSITION_TYPING,
	COMPOSITION_START,
	COMPOSITION_CHANGE,
	COMPOSITION_FULL,
	COMPOSITION_END,
	COMPOSITION_CANCEL,
	COMPOSITION_RESULT
	} CompositionAction;

typedef TypedActionPtr<CompositionAction, IMEManager> CompositionEvent;

class UnicodeType;
typedef TypedEvent<UnicodeType, wchar_t> UnicodeChar;

class IMEManager :
		public RefCountClass,
		public Notifier<IMEEvent>,
		public Notifier<UnicodeChar>,
		public Notifier<CompositionEvent>,
		public Notifier<CandidateEvent>
	{
	public:
		static IMEManager* Create(HWND) { return NULL; }

		void Activate(void) {}
		void Deactivate(void) {}
		bool IsActive(void) const { return false; }

		void Disable(void) {}
		void Enable(void) {}
		bool IsDisabled(void) const { return true; }

		const wchar_t* GetDescription(void) const { return L""; }
		uint16_t GetLanguageID(void) const { return 0; }
		uint32_t GetCodePage(void) const { return CP_ACP; }
		const wchar_t* GetResultString(void) const { return L""; }
		const wchar_t* GetCompositionString(void) const { return L""; }
		int32_t GetCompositionCursorPos(void) const { return 0; }
		const wchar_t* GetReadingString(void) const { return L""; }

		#ifdef SHOW_IME_TYPING
		const wchar_t* GetTypingString(void) const { return L""; }
		#endif

		void GetTargetClause(uint32_t& start, uint32_t& end) { start = 0; end = 0; }
		bool GetCompositionFont(LPLOGFONT) { return false; }
		const IMECandidateCollection GetCandidateColl(void) const { return IMECandidateCollection(); }
		uint32_t GetGuideline(wchar_t* outString, int length)
		{
			if (outString != NULL && length > 0) {
				outString[0] = 0;
			}
			return GL_LEVEL_NOGUIDELINE;
		}
		bool ProcessMessage(HWND, uint32_t, uintptr_t, intptr_t, intptr_t&) { return false; }
	};

} // namespace IME

#endif

#endif // __IMEMANAGER_H__
