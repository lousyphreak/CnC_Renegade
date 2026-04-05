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
*     $Archive: /Commando/Code/WWOnline/WOLNetUtilObserver.h $
*
* DESCRIPTION
*
* PROGRAMMER
*     $Author: Denzil_l $
*
* VERSION INFO
*     $Revision: 4 $
*     $Modtime: 1/22/02 5:53p $
*
******************************************************************************/

#ifndef __WOLNETUTILOBSERVER_H__
#define __WOLNETUTILOBSERVER_H__

#include <cstdint>

#include <windows.h>
#include "WOLUser.h"

namespace WOL 
{
#include <WOLAPI\wolapi.h>
}

template<typename T> class RefPtr;

namespace WWOnline {

class Session;
class SquadData;

class NetUtilObserver :
		public WOL::INetUtilEvent
	{
	public:
		NetUtilObserver();
		
		void Init(Session& outer);

		//---------------------------------------------------------------------------
		// IUnknown methods
		//---------------------------------------------------------------------------
		virtual int32_t STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
		virtual uint32_t STDMETHODCALLTYPE AddRef(void);
		virtual uint32_t STDMETHODCALLTYPE Release(void);

		//---------------------------------------------------------------------------
		// INetUtilEvent Methods
		//---------------------------------------------------------------------------
		STDMETHOD(OnPing)(int32_t hr, int time, uint32_t ip, int handle);
        
		STDMETHOD(OnLadderList)(int32_t hr, WOL::Ladder* list, int count, int32_t time, int keyRung);
       
		STDMETHOD(OnGameresSent)(int32_t hr);
      
		STDMETHOD(OnNewNick)(int32_t hr, LPCSTR message, LPCSTR nick, LPCSTR pass);
        
		STDMETHOD(OnAgeCheck)(int32_t hr, int years, int consent);
   
		STDMETHOD(OnWDTState)(int32_t hr, uint8_t* state, int length);

		STDMETHOD(OnHighscore)(int32_t hr, WOL::Highscore* list, int count, int32_t time, int keyRung);

	protected:
		virtual ~NetUtilObserver();

		NetUtilObserver(const NetUtilObserver&);
		const NetUtilObserver& operator=(const NetUtilObserver&);

		void ProcessLadderListResults(WOL::Ladder* list, int32_t timeStamp);
		void NotifyClanLadderUpdate(const UserList& users, const RefPtr<SquadData>& squad);

	private:
		uint32_t mRefCount;
		Session* mOuter;
	};

}

#endif // _WOLNETUTILOBSERVER_H__
