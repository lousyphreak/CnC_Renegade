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

#ifndef TCP_CON_HEADER
#define TCP_CON_HEADER

#include <cstdint>

#include "tcpmgr.h"
#include "wlib/arraylist.h"

class TCPMgr;

class TCPCon /// : OutputDevice
{
 public:
           TCPCon(SOCKET sock);
           ~TCPCon();

   SOCKET  getFD(void);
   void    close(void); 
   int32_t  write(IN uint8_t *msg, uint32_t len, int32_t wait_secs=-1);
   int32_t  read(OUT uint8_t *msg, uint32_t maxlen, int32_t wait_secs=-1);
   int8_t    unread(uint8_t *data, int length);
   int8_t    getRemoteAddr(uint32_t *ip, uint16_t *port);
   int32_t  printf(const char *format, ...);
   int8_t    isConnected(void);

   int8_t    setInputDelay(int32_t delay) { InputDelay_=delay; return(TRUE); };
   int8_t    setOutputDelay(int32_t delay) { OutputDelay_=delay; return(TRUE); };

   // For OutputDevice
   /// virtual int print(IN char *str, int len); 

 private:
   friend  class TCPMgr;
   void    pumpWrites(void);  // for buffered mode
   void    setBufferedWrites(TCPMgr *mgrptr, int8_t enabled);

   int32_t  normalWrite(IN uint8_t *msg, uint32_t len, int32_t wait_secs=-1);

   int32_t              InputDelay_;  // default max time for input
   int32_t              OutputDelay_; // default max time for output

   TCPMgr::CONN_STATE  State_;
   SOCKET              Socket_;
   ArrayList<uint8_t>    ReadQueue_;       // reads are buffered

   int8_t                BufferedWrites_;  // T/F buffer writes?
   ArrayList<uint8_t>    WriteQueue_;      // writes _can_ be buffered
   TCPMgr              *TCPMgrPtr_;      // pointer to my manager object
};

#endif
