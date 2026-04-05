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

/****************************************************************************\
TCP                   Neal Kettler        neal@westwood.com

\****************************************************************************/

#ifndef TCP_HEADER
#define TCP_HEADER

#include <cstdint>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#ifdef _WINDOWS

#include <winsock.h>
#include <io.h>
#define close _close
#define read  _read
#define write _write

#else  //UNIX
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <limits.h>

typedef int32_t SOCKET;

#endif


#ifdef AIX
#include <sys/select.h>
#endif

#define DEFAULT_PROTOCOL 0

#include "wlib/wstypes.h"
#include "wlib/wdebug.h"
#include "wlib/wtime.h"

class TCP
{

// DATA ---------------

private:
  int         mode;               // client or server
  int32_t      fd;                 // the primary FD

  uint32_t      myIP;               // after bind myIP & myPort will be
  uint16_t      myPort;             //   whatever we bound to

  struct sockaddr_in addr;
  int         maxFD;              // value of the biggest FD
  int         clientCount;        // how many clients open


  int32_t      inputDelay;         // default delay for semi-blocking reads
  int32_t      outputDelay;        // default delay for semi-blocking writes 

  enum ConnectionState
  {
     CLOSED,
     CONNECTING,
     CONNECTED
  }           connectionState;     // What state is client FD in 

public:

  enum
  {
    CLIENT = 1,
    SERVER = 2
  };

  // These defines specify a system independent way to
  //   get error codes for socket services.
  enum
  {
    OK,                 // Everything's cool
    UNKNOWN,            // There was an error of unknown type
    ISCONN,             // The socket is already connected
    INPROGRESS,         // The socket is non-blocking and the operation
                        //   isn't done yet
    ALREADY,            // The socket is already attempting a connection
                        //   but isn't done yet
    AGAIN,              // Try again.
    ADDRINUSE,          // Address already in use
    ADDRNOTAVAIL,       // That address is not available on the remote host
    BADF,               // Not a valid FD
    CONNREFUSED,        // Connection was refused
    INTR,               // Operation was interrupted
    NOTSOCK,            // FD wasn't a socket
    PIPE,               // That operation just made a SIGPIPE
    WOULDBLOCK,         // That operation would block
    INVAL,              // Invalid
    TIMEDOUT            // Timeout
  };

  // for client list (if this is a server)
  fd_set clientList;


// CODE ----------------

public:
          TCP(int newMode);
          TCP(int newMode,int16_t socket);
         ~TCP();
  int8_t    Bind(uint32_t IP,uint16_t port,int8_t reuseAddr=FALSE);
  int8_t    Bind(char *Host,uint16_t port,int8_t reuseAddr=FALSE);

  int32_t  GetMaxFD(void);

  int8_t    Connect(uint32_t IP,uint16_t port);
  int8_t    Connect(char *Host,uint16_t port);
  int8_t    ConnectAsync(uint32_t IP,uint16_t port);
  int8_t    ConnectAsync(char *Host,uint16_t port);

  int8_t    IsConnected(int32_t whichFD=0);

  int32_t  GetFD(void);
  int32_t  GetClientCount(void) { return(clientCount); }

  // Get IP or Port of a connected endpoint
  uint32_t  GetRemoteIP(int32_t whichFD=0);
  uint16_t  GetRemotePort(int32_t whichFD=0);

  int32_t  GetConnection(void);
  int32_t  GetConnection(struct sockaddr *clientAddr);
  void    WaitWrite(int32_t whichFD=0);
  int8_t    CanWrite(int32_t whichFD=0);
  int32_t  Write(const uint8_t *msg,uint32_t len,int32_t whichFD=0);
  int32_t  WriteNB(uint8_t *msg,uint32_t len,int32_t whichFD=0);
  int32_t  EncapsulatedWrite(uint8_t *msg,uint32_t len,int32_t whichFD=0);
  int32_t  WriteString(char *msg,int32_t whichFD=0);
  int32_t  Printf(int32_t whichFD,const char *format,...);
  int32_t  Read(uint8_t *msg,uint32_t len,int32_t whichFD=0);
  int32_t  TimedRead(uint8_t *msg,uint32_t len,int seconds,int32_t whichFD=0);
  int32_t  Peek(uint8_t *msg,uint32_t len,int32_t whichFD=0);
  int32_t  EncapsulatedRead(uint8_t *msg,uint32_t len,int32_t whichFD=0);

  char   *Gets(char *string,int n,int whichFD=0);

  // Wait on all sockets (or a specified one)
  //   return when ready for reading (or timeout occurs)
  int     Wait(int32_t sec,int32_t usec,fd_set &returnSet,int32_t whichFD=0);
  int     Wait(int32_t sec,int32_t usec,fd_set &inputSet,fd_set &returnSet);

  int     GetStatus(void);
  void    ClearStatus(void);

  //int32_t  GetSockStatus(int32_t whichFD=0);

  // give up ownership of the socket without closing it
  void    DisownSocket(void);

  int32_t  Close(int32_t whichFD=0);
  int32_t  CloseAll(void);   // close all sockets (same as close for client)

  int32_t  SetBlocking(int8_t block,int32_t whichFD=0);

  // Set default delays for semi-blocking reads & writes
  // default input = 5, output = 5
  // this is new and not used everywhere
  //
  int8_t    SetInputDelay(int32_t delay) { inputDelay=delay; return(TRUE); };
  int8_t    SetOutputDelay(int32_t delay) { outputDelay=delay; return(TRUE); };

};

#endif
