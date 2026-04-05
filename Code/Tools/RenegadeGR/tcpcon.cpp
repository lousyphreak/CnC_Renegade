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

#include "tcpcon.h"
#include <time.h>

#define DEFAULT_TCP_DELAY 15

TCPCon::TCPCon(SOCKET sock) :
  BufferedWrites_(false),
  TCPMgrPtr_(NULL)
{
  Socket_=sock;
  isConnected();  // This will set the internal connect status
  setInputDelay(DEFAULT_TCP_DELAY);  
  setOutputDelay(DEFAULT_TCP_DELAY);  
}

TCPCon::~TCPCon()
{
  close();  // will try and flush the write queue
  ReadQueue_.clear();
  WriteQueue_.clear();

  if ((BufferedWrites_) && (TCPMgrPtr_))
    TCPMgrPtr_->setBufferedWrites(this, FALSE);
}

SOCKET TCPCon::getFD(void)
{  return(Socket_); }


void TCPCon::close(void)
{
  if (BufferedWrites_)
  {
    while(WriteQueue_.length())
      pumpWrites();
  }
  ::closesocket(Socket_);
  State_=TCPMgr::CLOSED;
}


//
// Write data
//
// Returns 'n' bytes written, 0 if closed, or -1 for error.
//
int32_t TCPCon::write(IN uint8_t *msg,uint32_t len, int32_t wait_secs)
{
  if (State_==TCPMgr::CLOSED)
    return(0);

  if (BufferedWrites_)
  {
    WriteQueue_.addMany(msg, WriteQueue_.length(), len);  // add to tail
    pumpWrites();  // send what I can
    return(len);
  }
  else
  {
    return (normalWrite(msg, len, wait_secs));
  }
}


//
// set buffered status 
//
void TCPCon::setBufferedWrites(TCPMgr *mgrptr, int8_t enabled)
{
  if (enabled)
    BufferedWrites_=TRUE;  
  else
  {
    while(WriteQueue_.length())
      pumpWrites();
    BufferedWrites_=FALSE;
  }
  TCPMgrPtr_=mgrptr;
}


//
// Try and send queued data (PRIVATE)
//
void TCPCon::pumpWrites(void)
{
  if (State_==TCPMgr::CLOSED)
    WriteQueue_.clear();

  if (WriteQueue_.length()==0)
    return;

  int   sendlen;
  int   retval;
  uint8_t *bufptr=NULL;

  while(1)
  {
    sendlen=WriteQueue_.length();
    if (sendlen == 0)
      break;

    WriteQueue_.getPointer(&bufptr,0);  // pointer to first byte

    retval=normalWrite((uint8_t *)bufptr, sendlen, 0);
    if (retval <= 0)
      break;

    WriteQueue_.removeMany(NULL, 0, retval);  // successful send
  }
  return;
}


//
// Non-buffered write (PRIVATE METHOD) 
//
// Returns 'n' bytes written, 0 if closed, or -1 for error.
//
int32_t TCPCon::normalWrite(IN uint8_t *msg,uint32_t len, int32_t wait_secs)
{
  if (State_==TCPMgr::CLOSED)
    return(0);

  if (wait_secs < 0)
    wait_secs=OutputDelay_;

  int32_t retval=0;
  int32_t sendCount=0;
  time_t start=time(NULL);

  TCPMgr::STATUS status;
 
  while(1)
  {
    retval=send(Socket_,(const char *)(msg+sendCount),(len-sendCount),0);
    if ((retval > 0) && (retval+sendCount)==int(len))
      break;
    if (retval==SOCKET_ERROR)
    {
      status=TCPMgr::getStatus(); 
      if ((status != TCPMgr::INTR) && (status != TCPMgr::WOULDBLOCK) && (status != TCPMgr::INPROGRESS))
      {
        if (sendCount)
          return(sendCount);
        else
          return(-1);
      }
    }
    else if (retval > 0)
      sendCount+=retval;

    int32_t remaining_wait=wait_secs - (time(NULL)-start);
    if ((remaining_wait > 0) && (TCPMgr::wait(remaining_wait,0,&Socket_,1,FALSE) > 0))
      continue;  // I can write now.... 

    if (remaining_wait <= 0)
      break;
  }
  return(retval);
}

//
// Read data
//
// Returns 'n' bytes read, 0 for close, or -1 for error.
// This may return less than we asked for
//
int32_t TCPCon::read(OUT uint8_t *msg,uint32_t maxlen, int32_t wait_secs)
{
  int32_t retval=0;
  int32_t recvCount=0;
  time_t start=time(NULL);
  char   readBuffer[257];

  if (wait_secs < 0)
    wait_secs=InputDelay_;

  if (State_==TCPMgr::CLOSED)
    return(0);
 
  TCPMgr::STATUS status;
 
  while(1)
  {
    // Do we even nead to read from the net?
    if (ReadQueue_.length() >= int(maxlen))
    {
      ReadQueue_.removeMany(msg, 0, maxlen);
      return(maxlen);
    }

    do
    {
      retval=recv(Socket_,readBuffer, 256,0);
      if (retval > 0)   // add to the tail of the list
      {
        readBuffer[retval]=0;
        DBGMSG("RECV: "<<readBuffer);

        // Add to tail
        ReadQueue_.addMany((uint8_t *)readBuffer, ReadQueue_.length(), retval);
      }
    } while ((retval > 0)&&(ReadQueue_.length() < int(maxlen)));

    if (ReadQueue_.length())  // OK, we'll take what we've got
    {
      uint8_t *cptr;
      ReadQueue_.getPointer(&cptr,0);

/*******
      fprintf(stderr,"ReadQueue(%d): '",ReadQueue_.length()); 
      for (int i=0; i<ReadQueue_.length(); i++)
        fprintf(stderr,"%c",cptr[i]);
      fprintf(stderr,"'\n"); 
********/

      int retcount=MIN(ReadQueue_.length(), int(maxlen));
      ReadQueue_.removeMany(msg, 0, retcount);
      return(retcount);
    }
    else if (retval==0)
    {
      close();
      return(0);
    }
    else if (retval==SOCKET_ERROR)
    {
      status=TCPMgr::getStatus(); 
      if ((status != TCPMgr::INTR) && (status != TCPMgr::WOULDBLOCK) && 
          (status != TCPMgr::INPROGRESS))
        return(-1);
    }
 
    int32_t remaining_wait=wait_secs - (time(NULL)-start);
    if ((remaining_wait > 0) && (TCPMgr::wait(remaining_wait,0,&Socket_,1,TRUE) > 0))
      continue;  // I can read now.... 

    if (remaining_wait <= 0)
      break;
  }
  return(retval);
}

// Push data back onto the read queue
int8_t TCPCon::unread(uint8_t *data, int length)
{
  ReadQueue_.addMany(data, 0, length);
  return(TRUE);
}


// Returns 0 on failure
// Returns IP in host byte order!
int8_t TCPCon::getRemoteAddr(uint32_t *ip, uint16_t *port)
{
  struct sockaddr_in sin;
  int    sinSize=sizeof(sin);
 
  if(getpeername(Socket_,(sockaddr *)&sin,&sinSize)==0)
  {
    if (ip)
      *ip=ntohl(sin.sin_addr.s_addr);
    if (port)
      *port=ntohs(sin.sin_port);
    return(TRUE);
  }
  return(FALSE);
}

//
// only use for strings up to 4096 chars!
//
int32_t TCPCon::printf(const char *format, ...)
{
  va_list arg;
  char string[4097];
  int32_t retval;
  va_start(arg,format);
  vsprintf(string,format,arg);
  va_end(arg);
  string[4096]=0;
 
  retval=write((IN uint8_t *)string,strlen(string), OutputDelay_);
  return(retval);
}
 

int8_t TCPCon::isConnected(void)
{
  uint32_t remoteIp;
  uint16_t remotePort;

  if (getRemoteAddr(&remoteIp,&remotePort)==TRUE)
  {
    State_=TCPMgr::CONNECTED;
    return(TRUE);
  }
  else
  {
    State_=TCPMgr::CLOSED;
    return(FALSE);
  }
}

/*********
// For the OutputDevice interface
int TCPCon::print(IN char *str, int len)
{
  return(write((IN uint8_t *)str,len,0));
}
********/


