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
*     $Archive: /Commando/Code/WWOnline/WOLChatObserver.h $
*
* DESCRIPTION
*
* PROGRAMMER
*     $Author: Steve_t $
*
* VERSION INFO
*     $Revision: 4 $
*     $Modtime: 10/14/02 12:38p $
*
******************************************************************************/

#ifndef __WOLCHATOBSERVER_H__
#define __WOLCHATOBSERVER_H__

#include <cstdint>

#include <objbase.h>
#include "RefPtr.h"
#include "WOLUser.h"

namespace WOL 
{
#include <WOLAPI\wolapi.h>
}

namespace WWOnline {

class Session;
class SquadData;

class ChatObserver :
		public WOL::IChatEvent
	{
	public:
		ChatObserver();

		void Init(Session& outer);

		//---------------------------------------------------------------------------
		// IUnknown methods
		//---------------------------------------------------------------------------
		virtual int32_t STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
		virtual uint32_t STDMETHODCALLTYPE AddRef(void);
		virtual uint32_t STDMETHODCALLTYPE Release(void);

		//---------------------------------------------------------------------------
		// IChatEvent Methods
		//---------------------------------------------------------------------------
		STDMETHOD(OnServerList)(int32_t hr, WOL::Server* servers);
        
		STDMETHOD(OnUpdateList)(int32_t hr, WOL::Update* updates);
    
		STDMETHOD(OnServerError)(int32_t hr, LPCSTR ircmsg);
    
		STDMETHOD(OnConnection)(int32_t hr, LPCSTR motd);
    
		STDMETHOD(OnMessageOfTheDay)(int32_t hr, LPCSTR motd);
    
		STDMETHOD(OnChannelList)(int32_t hr, WOL::Channel* channels);
    
		STDMETHOD(OnChannelCreate)(int32_t hr, WOL::Channel* channel);
    
		STDMETHOD(OnChannelJoin)(int32_t hr, WOL::Channel* channel, WOL::User* user);
    
		STDMETHOD(OnChannelLeave)(int32_t hr, WOL::Channel* channel, WOL::User* user);
    
		STDMETHOD(OnChannelTopic)(int32_t hr, WOL::Channel* channel, LPCSTR topic);
    
		STDMETHOD(OnPrivateAction)(int32_t hr, WOL::User* user, LPCSTR action);
    
		STDMETHOD(OnPublicAction)(int32_t hr, WOL::Channel* channel, WOL::User* user, LPCSTR action);
    
		STDMETHOD(OnUserList)(int32_t hr, WOL::Channel* channel, WOL::User* users);
    
		STDMETHOD(OnPublicMessage)(int32_t hr, WOL::Channel* channel, WOL::User* user, LPCSTR message);
    
		STDMETHOD(OnPrivateMessage)(int32_t hr, WOL::User* user, LPCSTR message);
    
		STDMETHOD(OnSystemMessage)(int32_t hr, LPCSTR message);
    
		STDMETHOD(OnNetStatus)(int32_t hr);
    
		STDMETHOD(OnLogout)(int32_t status, WOL::User* user);
    
		STDMETHOD(OnPrivateGameOptions)(int32_t hr, WOL::User* user, LPCSTR options);
    
		STDMETHOD(OnPublicGameOptions)(int32_t hr, WOL::Channel* channel, WOL::User* user, LPCSTR options);
    
		STDMETHOD(OnGameStart)(int32_t hr, WOL::Channel* channel, WOL::User* users, int gameid);
    
		STDMETHOD(OnUserKick)(int32_t hr, WOL::Channel* channel, WOL::User* kicked, WOL::User* kicker);
    
		STDMETHOD(OnUserIP)(int32_t hr, WOL::User* user);
    
		STDMETHOD(OnFind)(int32_t hr, WOL::Channel* chan);
    
		STDMETHOD(OnPageSend)(int32_t hr);
    
		STDMETHOD(OnPaged)(int32_t hr, WOL::User* user, LPCSTR message);
    
		STDMETHOD(OnServerBannedYou)(int32_t hr, WOL::time_t bannedTill);
    
		STDMETHOD(OnUserFlags)(int32_t hr, LPCSTR name, uint32_t flags, uint32_t mask);
    
		STDMETHOD(OnChannelBan)(int32_t hr, LPCSTR name, int banned);
    
		STDMETHOD(OnSquadInfo)(int32_t hr, uint32_t id, WOL::Squad* squad);
    
		STDMETHOD(OnUserLocale)(int32_t hr, WOL::User* users);
    
		STDMETHOD(OnUserTeam)(int32_t hr, WOL::User* users);
    
		STDMETHOD(OnSetLocale)(int32_t hr, WOL::Locale newlocale);
    
		STDMETHOD(OnSetTeam)(int32_t hr, int newteam);

		STDMETHOD(OnBuddyList)(int32_t hr, WOL::User* buddyList);
        
		STDMETHOD(OnBuddyAdd)(int32_t hr, WOL::User* buddyAdded);
        
		STDMETHOD(OnBuddyDelete)(int32_t hr, WOL::User* buddyDeleted);

		STDMETHOD(OnPublicUnicodeMessage)(int32_t hr, WOL::Channel* channel, WOL::User* user, const uint16_t* message);
        
		STDMETHOD(OnPrivateUnicodeMessage)(int32_t hr, WOL::User* user, const uint16_t* message);
        
		STDMETHOD(OnPrivateUnicodeAction)(int32_t hr, WOL::User* user, const uint16_t* action);
        
		STDMETHOD(OnPublicUnicodeAction)(int32_t hr, WOL::Channel* channel, WOL::User* user, const uint16_t* action);
        
		STDMETHOD(OnPagedUnicode)(int32_t hr, WOL::User* user, const uint16_t* message);
        
		STDMETHOD(OnServerTime)(int32_t hr, WOL::time_t stime);
        
		STDMETHOD(OnInsiderStatus)(int32_t hr, WOL::User* users);
        
		STDMETHOD(OnSetLocalIP)(int32_t hr, LPCSTR message);

		STDMETHOD(OnChannelListBegin)(int32_t hr);
        
		STDMETHOD(OnChannelListEntry)(int32_t hr, WOL::Channel* channel);
        
		STDMETHOD(OnChannelListEnd)(int32_t hr);

	protected:
		virtual ~ChatObserver();

		// prevent copy and assignment
		ChatObserver(ChatObserver const &);
		ChatObserver const & operator =(ChatObserver const &);

		void AssignSquadToUsers(const UserList& users, const RefPtr<SquadData>& squad);
		void ProcessSquadRequest(const RefPtr<SquadData>& squad);
		void Kick_Spammer(WOL::User *wol_user);


	private:
		uint32_t mRefCount;
		Session* mOuter;
	};

}

#endif // __WOLCHATOBSERVER_H__