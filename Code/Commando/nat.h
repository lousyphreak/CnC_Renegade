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
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/nat.h                               $*
 *                                                                                             *
 *                      $Author:: Steve_t                                                     $*
 *                                                                                             *
 *                     $Modtime:: 3/26/02 12:16p                                              $*
 *                                                                                             *
 *                    $Revision:: 19                                                          $*
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include <cstdint>

#ifndef NAT_H
#define NAT_H

#include "renegade_build_config.h"

#include	"always.h"
#include	"win.h"

#if !RENEGADE_WITH_LEGACY_WOL

#include	"nataddr.h"
#include <wwlib/vector.h>

namespace WOL {
struct User;
class Server;
}

class WideStringClass;
class StringClass;
class cPacket;
class SocketHandlerClass;
class RegistryClass;

class FirewallHelperClass {
	public:
		typedef enum tFirewallBehaviorType {
			FIREWALL_TYPE_UNKNOWN = 0,
			FIREWALL_TYPE_SIMPLE = 1
		} FirewallBehaviorType;

		enum {
			FW_RESULT_UNKNOWN,
			FW_RESULT_FAILED,
			FW_RESULT_SUCCEEDED,
			FW_RESULT_CANCELLED
		};

		FirewallHelperClass(void) :
			SourcePortPool(0),
			ClientPort(0),
			SendDelay(false)
		{}

		void Startup(void) {}
		void Shutdown(void) {}
		void Detect_Firewall(HANDLE = 0) {}
		uint16_t Get_Raw_Firewall_Behavior(void) { return FIREWALL_TYPE_UNKNOWN; }
		int16_t Get_Source_Port_Allocation_Delta(void) { return 0; }
		uint16_t Get_Next_Mangled_Source_Port(uint16_t source_port) { return source_port; }
		int Get_Firewall_Hardness(FirewallBehaviorType) { return 0; }
		int Get_Firewall_Retries(FirewallBehaviorType) { return 0; }
		void Set_Source_Port_Pool_Start(int port) { SourcePortPool = port; }
		int Get_Source_Port_Pool(void) { return SourcePortPool; }
		int Build_Mangler_Packet(uint8_t *, uint16_t, uint32_t = 0, bool = false) { return 0; }
		uint16_t Get_Next_Temporary_Source_Port(int) { return 0; }
		bool Get_Reference_Port(void) { return false; }
		void Reset_Server(void) {}
		uint16_t Get_Client_Bind_Port(void) { return ClientPort; }
		void Set_Firewall_Info(uint32_t, int, uint16_t, bool send_delay, int) { SendDelay = send_delay; }
		void Get_Firewall_Info(uint32_t &last_behavior, int &last_delta, uint16_t &port_pool, bool &send_delay, int &confidence) const {
			last_behavior = FIREWALL_TYPE_UNKNOWN;
			last_delta = 0;
			port_pool = 0;
			send_delay = SendDelay;
			confidence = 0;
		}
		void Set_Send_Delay(bool send_delay) { SendDelay = send_delay; }
		bool Get_Send_Delay(void) { return SendDelay; }
		bool Send_To_Mangler(IPAddressClass *, SocketHandlerClass *, uint32_t, bool = false) { return false; }
		uint16_t Get_Mangler_Response(uint32_t, SocketHandlerClass *, int = 0, bool = false) { return 0; }
		void Connected_To_WWOnline_Server(void) {}
		void Talk_To_New_Player(WOL::User *) {}
		void Send_My_Port(uint16_t) {}
		void Set_Client_Connect_Event(HANDLE, HANDLE, int *, int *) {}
		bool Remove_Player_From_Negotiation_Queue(char *) { return false; }
		bool Remove_Player_From_Negotiation_Queue_If_Mutex_Available(char *) { return false; }
		void Cleanup_Client_Queue(void) {}
		bool Get_Local_Chat_Connection_Address(void) { return false; }
		uint32_t Get_Local_Address(void) { return 0; }
		IPAddressClass &Get_External_Address(void) { return ExternalAddress; }
		void Set_External_Address(IPAddressClass &addr) { ExternalAddress = addr; }
		void Reset(void) {}
		bool Is_Busy(void) { return false; }
		bool Is_NAT(void) { return false; }
		bool Is_NAT(FirewallBehaviorType) { return false; }
		bool Is_Netgear(FirewallBehaviorType) { return false; }
		bool Is_Netgear(void) { return false; }

	private:
		int SourcePortPool;
		uint16_t ClientPort;
		bool SendDelay;
		IPAddressClass ExternalAddress;
};

class WOLNATInterfaceClass {
	public:
		WOLNATInterfaceClass(void) :
			IsServer(false),
			ForcePort(0),
			PortBase(0),
			RegExternalIP(0),
			RegExternalPort(0),
			ChatExternalIP(0)
		{}

		void Init(void) {}
		void Shutdown(void) {}
		void Service(void) {}
		void Set_Service_Socket_Handler(SocketHandlerClass *) {}
		void Service_Receive_Queue(SocketHandlerClass *) {}
		void Get_Current_Server_ConnData(char *buffer, int size) { if (buffer != 0 && size > 0) { buffer[0] = 0; } }
		void Send_Private_Game_Options(WOL::User *, char *) {}
		bool Get_Private_Game_Options(WOL::User *, char *, int) { return false; }
		char *Get_Silly_String(WideStringClass *, char *buffer, int buffer_size) {
			if (buffer != 0 && buffer_size > 0) {
				buffer[0] = 0;
			}
			return buffer;
		}
		bool Get_Packet(char *, int, IPAddressClass &) { return false; }
		void Intercept_Game_Packet(cPacket &) {}
		bool Send_Game_Format_Packet_To(IPAddressClass *, char *, int, SocketHandlerClass * = NULL) { return false; }
		bool Get_My_Name(char *namebuf) { if (namebuf != 0) { namebuf[0] = 0; } return false; }
		bool Am_I_Server(void) { return IsServer; }
		void Set_Server(bool is_server) { IsServer = is_server; }
		void Set_Server_Negotiated_Address(IPAddressClass *) {}
		DynamicVectorClass<WOL::Server*> Get_Mangler_Server_List(void) { return DynamicVectorClass<WOL::Server*>(); }
		uint16_t Get_Mangler_Port_By_Index(int) { return 0; }
		bool Get_Mangler_Name_By_Index(int, char *mangler_name) { if (mangler_name != 0) { mangler_name[0] = 0; } return false; }
		int Get_Num_Mangler_Servers(void) { return 0; }
		uint32_t Get_Local_Address(void) { return 0; }
		void Tell_Server_That_Client_Is_In_Channel(void) {}
		bool Is_NAT_Thread_Busy(void) { return false; }
		uint16_t Get_Next_Client_Port(void) { return ForcePort; }
		uint16_t Get_Port_As_Server(void) { return (ForcePort != 0) ? ForcePort : PortBase; }
		uint16_t Get_Port_As_Server_Client(void) { return (ForcePort != 0) ? static_cast<uint16_t>(ForcePort + 1) : static_cast<uint16_t>(PortBase + 1); }
		uint16_t Get_Force_Port(void) { return ForcePort; }
		void Get_Config(RegistryClass *, int &port_number, bool &send_delay) { port_number = ForcePort; send_delay = false; }
		void Set_Config(RegistryClass *, int port_number, bool) { ForcePort = static_cast<uint16_t>(port_number); PortBase = static_cast<uint16_t>(port_number); }
		void Save_Firewall_Info_To_Registry(void) {}
		uint32_t Get_Reg_External_IP(void) { return RegExternalIP; }
		uint32_t Get_Reg_External_Port(void) { return RegExternalPort; }
		void Get_Compact_Log(StringClass &) {}
		uint32_t Get_Chat_External_IP(void) { return ChatExternalIP; }

	private:
		bool IsServer;
		uint16_t ForcePort;
		uint16_t PortBase;
		uint32_t RegExternalIP;
		uint32_t RegExternalPort;
		uint32_t ChatExternalIP;
};

extern FirewallHelperClass FirewallHelper;
extern WOLNATInterfaceClass WOLNATInterface;

#else

#ifdef WWASSERT
#ifndef fw_assert
#define fw_assert WWASSERT
#endif //fw_assert
#else //WWASSERT
#define fw_assert assert
#endif //WWASSERT

#include	"nataddr.h"
#include "natter.h"
#include "wolapi.h"

/*
** Number of ports to use when testing port mangling sequences.
*/
#define NUM_TEST_PORTS 4

#ifndef IPAddressClass
class IPAddressClass;
#endif //IPAddressClass

#ifndef SocketHandlerClass
class SocketHandlerClass;
#endif //SocketHandlerClass


/*
**
**   Class to help in dealing with firewalls.
**
**		Basically detects firewall type and behavior.
**
**		 In Renegade it also does the firewall port negotiation between server and client.
*/
class FirewallHelperClass {

	public:

		/*
		** Enumeration of firewall behaviors we can detect.
		**
		** It is assumed that all port mangling firewalls change the mangled source port in response to
		** an application source port change.
		*/
		typedef enum tFirewallBehaviorType {

			/*
			** Just used as an initialiser.
			*/
			FIREWALL_TYPE_UNKNOWN = 0,

			/*
			** This is a simple, non-port translating firewall, or there is no firewall at all.
			*/
			FIREWALL_TYPE_SIMPLE = 1,

			/*
			** This is a firewall/NAT with port mangling but it's pretty dumb - it uses the same mangled
			** source port regardless of the destination address.
			*/
			FIREWALL_TYPE_DUMB_MANGLING = 2,

			/*
			** This is a smarter firewall/NAT with port mangling that uses different mangled source ports
			** for different destination IPs.
			*/
			FIREWALL_TYPE_SMART_MANGLING = 4,

			/*
			** This is a firewall that exhibits the bug as seen in the Netgear firewalls. A previously good
			** source port mapping will change in response to unsolicited traffic from a known IP.
			*/
			FIREWALL_TYPE_NETGEAR_BUG = 8,

			/*
			** This firewall has a simple absolute offset port allocation scheme.
			*/
			FIREWALL_TYPE_SIMPLE_PORT_ALLOCATION = 16,

			/*
			** This firewall has a relative offset port allocation scheme. For these firewalls, we have to
			** subtract the actual source port from the mangled source port to discover the allocation scheme.
			** The mangled port number is based in part on the original source port number.
			*/
			FIREWALL_TYPE_RELATIVE_PORT_ALLOCATION = 32,

			/*
			** This firewall mangles source ports differently depending on the destination port.
			*/
			FIREWALL_TYPE_DESTINATION_PORT_DELTA = 64

		} FirewallBehaviorType;


		/*
		** Constructor, destructor.
		*/
		FirewallHelperClass(void);
		~FirewallHelperClass(void);

		/*
		** Startup, shutdown.
		*/
		void Startup(void);
		void Shutdown(void);

		/*
		** Detection.
		*/
		void Detect_Firewall(HANDLE event = INVALID_HANDLE_VALUE);
		uint16_t Get_Raw_Firewall_Behavior(void);	// {return((uint16_t)Behavior);};
		int16_t Get_Source_Port_Allocation_Delta(void) {return(static_cast<int16_t>(SourcePortAllocationDelta));}

		/*
		** Query class for behavior.
		*/
		uint16_t Get_Next_Mangled_Source_Port(uint16_t source_port);
		int Get_Firewall_Hardness(FirewallBehaviorType behavior);
		int Get_Firewall_Retries(FirewallBehaviorType behavior);
		void Set_Source_Port_Pool_Start(int port) {SourcePortPool = port;};
		int Get_Source_Port_Pool(void) {return(SourcePortPool);};

		/*
		** Talking to the manglers.
		*/
		int Build_Mangler_Packet(uint8_t *buffer, uint16_t port, uint32_t packet_id = 0, bool blitzme = false);

		/*
		** Port management.
		*/
		uint16_t Get_Next_Temporary_Source_Port(int skip);
		bool Get_Reference_Port(void);
		void Reset_Server(void);
		uint16_t Get_Client_Bind_Port(void) {return(ClientPort);};	//PlayersFirewallAddress.Get_Port());};

		/*
		** Firewall info import and export.
		*/
		void Set_Firewall_Info(uint32_t last_behavior, int last_delta, uint16_t port_pool, bool send_delay, int confidence);
		void Get_Firewall_Info(uint32_t &last_behavior, int &last_delta, uint16_t &port_pool, bool &send_delay, int &confidence) const;
		void Set_Send_Delay(bool send_delay) {SendDelay = send_delay;};
		bool Get_Send_Delay(void) {return(SendDelay);};

		/*
		** Communications functions.
		*/
		bool Send_To_Mangler(IPAddressClass *address, SocketHandlerClass *socket_handler, uint32_t packet_id, bool blitzme = false);
		uint16_t Get_Mangler_Response(uint32_t packet_id, SocketHandlerClass *socket_handler, int time = 0, bool all_service = false);

		/*
		** Server connection negotiation functions.
		*/
		void Connected_To_WWOnline_Server(void);
		void Talk_To_New_Player(WOL::User *user);
		void Send_My_Port(uint16_t port);
		void Set_Client_Connect_Event(HANDLE thread_event, HANDLE cancel_event, int *flag_ptr, int *queue_ptr);
		bool Remove_Player_From_Negotiation_Queue(char *player_name);
		bool Remove_Player_From_Negotiation_Queue_If_Mutex_Available(char *player_name);
		void Cleanup_Client_Queue(void);


		/*
		** Get the local chat connection address.
		*/
		bool Get_Local_Chat_Connection_Address(void);
		uint32_t Get_Local_Address(void);
		IPAddressClass &Get_External_Address(void) {return(ExternalAddress);}
		void Set_External_Address(IPAddressClass &addr) {ExternalAddress = addr;}

		/*
		** Reset.
		*/
		void Reset(void);

		/*
		** Query.
		*/
		bool Is_Busy(void) {return((ThreadState != THREAD_IDLE) ? true : false);}

		/*
		** Inline behavior query functions.
		*/
		inline bool Is_NAT(void) {
			if (Behavior == FIREWALL_TYPE_UNKNOWN || (Behavior & FIREWALL_TYPE_SIMPLE) != 0) {
				return(false);
			}
			return(true);
		};

		inline bool Is_NAT(FirewallBehaviorType behavior) {
			if (behavior == FIREWALL_TYPE_UNKNOWN || (behavior & FIREWALL_TYPE_SIMPLE) != 0) {
				return(false);
			}
			return(true);
		};

		inline bool Is_Netgear(FirewallBehaviorType behavior) {
			if ((behavior & FIREWALL_TYPE_NETGEAR_BUG) != 0) {
				return(true);
			}
			return(false);
		};

		inline bool Is_Netgear(void) {
			if ((Behavior & FIREWALL_TYPE_NETGEAR_BUG) != 0) {
				return(true);
			}
			return(false);
		};

		/*
		** Exposing the thread ID for the exception handler.
		*/
		uint32_t Get_Thread_ID(void) {return(ThreadID);};

		/*
		** Connection results reported back to the dialog wait object.
		*/
		enum {
			FW_RESULT_UNKNOWN,
			FW_RESULT_FAILED,
			FW_RESULT_SUCCEEDED,
			FW_RESULT_CANCELLED
		};

	private:

		/************************************************************************************************************************
		**
		** Private functions.
		**
		*/

		/*
		** Detection.
		*/
		FirewallBehaviorType Detect_Firewall_Behavior(void);
		int Get_NAT_Port_Allocation_Scheme(int num_ports, uint16_t *original_ports, uint16_t *mangled_ports, bool &relative_delta, bool &looks_good);

		/*
		** Server connection negotiation functions.
		*/
		int Negotiate_Port(void);
		void Send_Connection_Result(int result, uint16_t port);
		void Set_Client_Success(int success);
		void Send_Queue_States(void);
		bool Client_Cancelled(void);
		bool Remote_Client_Cancelled(void);
		void Send_Cancel_Notification(void);

		/*
		** Threading.
		*/
		static uint32_t __stdcall NAT_Thread_Start(void *param);
		uint32_t NAT_Thread_Main_Loop(void);
		void Add_Thread_Action(int thread_action, HANDLE thread_event);
		void Set_Thread_Event(void);

		/*
		** Game options processing.
		*/
		void Process_Game_Options(void);



		/************************************************************************************************************************
		**
		** Private data.
		**
		*/

		/*
		** How does our firewall behave?
		*/
		FirewallBehaviorType Behavior;

		/*
		** How did the firewall behave the last time we ran the game.
		*/
		FirewallBehaviorType LastBehavior;

		/*
		** What is the delta in our firewalls NAT port allocation scheme.
		*/
		int SourcePortAllocationDelta;

		/*
		** What was the delta the last time we ran?
		*/
		int LastSourcePortAllocationDelta;

		/*
		** Source ports used only to discover port allocation patterns.
		*/
		int SourcePortPool;

		/*
		** Should we delay before sending (for Netgear bug)?
		*/
		bool SendDelay;

		/*
		** How well do the remembered settings work.
		*/
		int Confidence;

		/*
		** Mangler server info.
		*/
		int NumManglerServers;
		char ManglerServerAddress[64][128];
		int ManglerServerPort[64];
		int CurrentManglerServer;

		/*
		** Local chat connection address.
		*/
		IPAddressClass LocalChatConnectionAddress;

		/*
		** Our address as far as chat knows.
		*/
		IPAddressClass ExternalAddress;



		/************************************************************************************************************************
		**
		** Server/Client connection related private data.
		**
		**
		*/

		/*
		** Who we are currently trying to talk to.
		*/
		char 						PlayersName[64];

		/*
		** Local address of the player we are trying to talk to.
		*/
		IPAddressClass 		PlayersLocalAddress;

		/*
		** External (mangler view) address of the player we are trying to talk to.
		*/
		IPAddressClass 		PlayersExternalAddress;

		/*
		** Type of firewall that the other player has.
		*/
		FirewallBehaviorType PlayersFirewallType;

		/*
		** Teacks what we expect the other player to have his port number mangled to.
		*/
		uint16_t			PlayersMangledPort;

		/*
		** Other players in WOL::User struct form.
		*/
		WOL::User				PlayerAsUser;

		/*
		** Our current best guess at the combined mangled port/ip of the other player.
		*/
		IPAddressClass			PlayersFirewallAddress;

		/*
		** Connection result reported by other player. Used to find out if our packet got through. See enum below.
		*/
		int 						PlayersConnectionResult;

		/*
		** Port that the other player saw our packet actually come from.
		*/
		uint16_t			PlayersConnectionResultPort;

		/*
		** Enum for connection results.
		*/
		enum {
			CONNRESULT_UNTRIED = -1,
			CONNRESULT_FAILED = 0,
			CONNRESULT_TRY1,
			CONNRESULT_TRY2,
			CONNRESULT_TRY3,
			CONNRESULT_TRY4,
			CONNRESULT_TRY5,
			CONNRESULT_TRY6,
			CONNRESULT_TRY7,
			CONNRESULT_TRY8,
			CONNRESULT_TRY9,
			CONNRESULT_TRY10,
			CONNRESULT_CONNECTED
		};


		/************************************************************************************************************************
		**
		** Server side only data.
		**
		*/

		/*
		** Client queue (for server only). This is a list of clients waiting to negotiate a port.
		*/
		typedef struct tClientStruct {
			char 						Name[64];
			IPAddressClass 		LocalAddress;
			IPAddressClass 		ExternalAddress;
			FirewallBehaviorType	FirewallType;
			WOL::User				User;
		} ClientStruct;

		DynamicVectorClass<ClientStruct*> ClientQueue;

		/*
		** List if names waiting to be removed from the client queue.
		*/
		DynamicVectorClass<ClientStruct*> ClientQueueRemoveList;

		/*
		** Connections we have made so far in this game. This includes people who were in the game and left.
		*/
		DynamicVectorClass<IPAddressClass> ConnectionHistory;
		DynamicVectorClass<uint16_t> MangledPortHistory;

		/*
		** When we last heard from the client.
		*/
		uint32_t LastOptionsFromClient;

		/*
		** Name of player who has cancelled out of our game channel before connecting.
		*/
		char CancelPlayer[64];


		/************************************************************************************************************************
		**
		** Client side only data.
		**
		*/

		/*
		** Number of players in the servers queue ahead of us.
		*/
		int QueuedPlayers;

		/*
		** Port that the client will use as a basis for negotiation.
		*/
		uint16_t ClientPort;

		/*
		** Client connect event notification.
		*/
		int		*SuccessFlagPtr;
		HANDLE	ClientConnectEvent;
		HANDLE	ClientCancelEvent;
		int		*QueueNotifyPtr;





		/************************************************************************************************************************
		**
		** Threading.
		**
		**		This class runs in it's own thread so that firewall negotiation doesn't stall the server when a new client joins.
		**		It also means that the clients join dialog is serviced while connecting, allowing him to cancel at any time.
		**
		*/
		HANDLE ThreadHandle;
		uint32_t ThreadID;
		HANDLE NATThreadMutex;
		HANDLE NATDataMutex;
		bool ThreadActive;

		/*
		** Thread actions.
		*/
		enum {
			THREAD_IDLE,
			THREAD_DETECT_FIREWALL,
			THREAD_DETECT_FIREWALL_DONE,
			THREAD_CONNECT_FIREWALL,
			THREAD_CONNECT_FIREWALL_DONE,
			THREAD_GET_LOCAL_ADDRESS,
			THREAD_GET_LOCAL_ADDRESS_DONE
		};

		/*
		** Thread state i.e. what it's doing.
		*/
		int ThreadState;

		/*
		** Event to be signalled when the current thread action completes.
		*/
		HANDLE ThreadEvent;

		/*
		** Thread action FIFO
		*/
		class ThreadActionClass {

			public:
				inline bool operator == (ThreadActionClass const &data) {
					return(memcmp((void*)this, &data, sizeof(*this)) == 0);
				};
				inline bool operator != (ThreadActionClass const &data) {
					return(memcmp((void*)this, &data, sizeof(*this)) != 0);
				};

				int		ThreadAction;
				HANDLE	ThreadEvent;
		};

		DynamicVectorClass<ThreadActionClass> ThreadQueue;


		/*
		** Thread safety.
		*/
		class ThreadLockClass
		{
			public:
				/*
				** Constructor. Grabs the mutex.
				*/
				inline ThreadLockClass(FirewallHelperClass *fwptr, uint32_t timeout = 10 * 1000) {
					FWPtr = fwptr;

					/*
					** Just test the mutex if timeout is 0.
					*/
					WaitResult = WaitForSingleObject(fwptr->NATDataMutex, timeout);
					if (timeout != 0 && WaitResult == WAIT_TIMEOUT) {
						WWDEBUG_SAY(("FirewallHelper - Timeout waiting for firewall helper data mutex\n"));
						fw_assert(WaitResult != WAIT_TIMEOUT);
					}
				};

				FirewallHelperClass *FWPtr;

				int WaitResult;

				/*
				** Destructor, releases the mutex.
				*/
				inline ~ThreadLockClass(void) {
					ReleaseMutex(FWPtr->NATDataMutex);
				};
		};
		friend ThreadLockClass;









	/*****************************************************************************************************************
	**
	**
	** C&C Packet format as expected by the mangler. Copied from various places in the RA2 code.
	**
	*/
	public:

		/*
		** One byte alignment.
		*/
		#pragma pack(push, 1)

		/*
		** Misc C&C packet defines.
		*/
		#define GLOBAL_MAGICNUM 0x1236
		#define PACKET_DATA_NOACK 1
		#define COMMAND_AND_CONQUER_RA2 0xaa03
		#define NET_MANGLER_REQUEST 43
		#define NET_MANGLER_RESPONSE 44
		#define MPLAYER_NAME_MAX 20
		#define SERIAL_MAX 23

		/*
		** CommHeaderType - Low level packet wrapper.
		*/
		struct CommHeaderType {
			uint16_t MagicNumber;		   // in, out = GLOBAL_MAGICNUM = 0x1236. Just preserve the incoming value.
			char Code;									// in, out = PACKET_DATA_NOACK = 1
			union  {
				uint8_t ForwardTo;			//  = 0
				uint8_t ForwardFrom;
			};
			uint32_t PacketID;					// Dont care.
			uint8_t ForwardAddress[4];
			uint16_t ForwardPort;
		};

		/*
		** GlobalHeaderType - Low level packet wrapper for global channel packets.
		*/
		struct GlobalHeaderType {
			CommHeaderType Header;					// See above
			uint16_t ProductID;				// in, out = COMMAND_AND_CONQUER_RA2 = 0xaa03. Just return the value from the received packet
		};


		/*
		** GlobalPacketType - Payload of global channel packet.
		*/
		struct GlobalPacketType {
			int Command;								// in = NET_MANGLER_REQUEST = 43 , out = NET_MANGLER_RESPONSE = 44
			char Name[MPLAYER_NAME_MAX];			//	in, out = Player name (8 bit) - return same name
			char Serial[SERIAL_MAX];				// Not used.

			union
			{
				struct {
					uint16_t MangledPortNumber;
					uint8_t MangledAddress[4];
					uint16_t OriginalPortNumber;
					uint8_t BlitzMe;
				} ManglerData;

			};
		};

		/*
		** CnCPacketType - Header and payload in one structure.
		*/
		typedef struct tCnCPacketType {
			GlobalHeaderType GHeader;
			GlobalPacketType Packet;
		} CnCPacketType;

		#pragma pack(pop)

	/*
	** End C&C packet format.
	****************************************************************************************************************
	*/
};


/*
** Single instance of FirewallHelperClass
*/
extern FirewallHelperClass FirewallHelper;


#endif


#endif	//NAT_H
