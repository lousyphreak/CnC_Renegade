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

#ifndef UDP_HEADER
#define UDP_HEADER

#include <cstdint>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

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
#endif

#ifdef AIX
#include <sys/select.h>
#endif

#define DEFAULT_PROTOCOL 0

#include <wlib/wstypes.h>
#include <wlib/wtime.h>

class UDP
{
 // DATA
 private:
  int32_t       fd; 
  uint32_t       myIP;
  uint16_t       myPort;
  struct       sockaddr_in  addr;
  
  // These defines specify a system independent way to
  //   get error codes for socket services.
  enum sockStat
  {
    OK           =  0,     // Everything's cool
    UNKNOWN      = -1,     // There was an error of unknown type
    ISCONN       = -2,     // The socket is already connected
    INPROGRESS   = -3,     // The socket is non-blocking and the operation
                           //   isn't done yet
    ALREADY      = -4,     // The socket is already attempting a connection
                           //   but isn't done yet
    AGAIN        = -5,     // Try again.
    ADDRINUSE    = -6,     // Address already in use
    ADDRNOTAVAIL = -7,     // That address is not available on the remote host
    BADF         = -8,     // Not a valid FD
    CONNREFUSED  = -9,     // Connection was refused
    INTR         =-10,     // Operation was interrupted
    NOTSOCK      =-11,     // FD wasn't a socket
    PIPE         =-12,     // That operation just made a SIGPIPE
    WOULDBLOCK   =-13,     // That operation would block
    INVAL        =-14,     // Invalid
    TIMEDOUT     =-15      // Timeout
  };

// CODE
 private:
  int32_t           SetBlocking(int8_t block);

 public:
                   UDP();
                  ~UDP();
  int32_t           Bind(uint32_t IP,uint16_t port);
  int32_t           Bind(char *Host,uint16_t port);
  int32_t           Write(uint8_t *msg,uint32_t len,uint32_t IP,uint16_t port);
  int32_t           Read(uint8_t *msg,uint32_t len,sockaddr_in *from);
  sockStat         GetStatus(void);
  void             ClearStatus(void);
  int              Wait(int32_t sec,int32_t usec,fd_set &returnSet);
  int              Wait(int32_t sec,int32_t usec,fd_set &givenSet,fd_set &returnSet);

  int8_t             getLocalAddr(uint32_t &ip, uint16_t &port);
  int32_t           getFD(void) { return(fd); }
 
  int8_t             SetInputBuffer(uint32_t bytes);
  int8_t             SetOutputBuffer(uint32_t bytes);
  int              GetInputBuffer(void);
  int              GetOutputBuffer(void);
};

#endif
