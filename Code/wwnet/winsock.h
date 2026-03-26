#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "win.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include "win.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

using SOCKET = int;
using SOCKADDR = struct sockaddr;
using SOCKADDR_IN = struct sockaddr_in;
using IN_ADDR = struct in_addr;
using HOSTENT = struct hostent;
using SERVENT = struct servent;
using LPSOCKADDR = SOCKADDR *;
using LPSOCKADDR_IN = SOCKADDR_IN *;
using LPHOSTENT = HOSTENT *;
using LPSERVENT = SERVENT *;
using u_long = unsigned long;

struct WSADATA {
    WORD wVersion;
    WORD wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char * lpVendorInfo;
};

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#ifndef WSAEINTR
#define WSAEINTR EINTR
#endif
#ifndef WSAEBADF
#define WSAEBADF EBADF
#endif
#ifndef WSAEACCES
#define WSAEACCES EACCES
#endif
#ifndef WSAEFAULT
#define WSAEFAULT EFAULT
#endif
#ifndef WSAEINVAL
#define WSAEINVAL EINVAL
#endif
#ifndef WSAEMFILE
#define WSAEMFILE EMFILE
#endif
#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK EWOULDBLOCK
#endif
#ifndef WSAEINPROGRESS
#define WSAEINPROGRESS EINPROGRESS
#endif
#ifndef WSAEALREADY
#define WSAEALREADY EALREADY
#endif
#ifndef WSAENOTSOCK
#define WSAENOTSOCK ENOTSOCK
#endif
#ifndef WSAEDESTADDRREQ
#define WSAEDESTADDRREQ EDESTADDRREQ
#endif
#ifndef WSAEMSGSIZE
#define WSAEMSGSIZE EMSGSIZE
#endif
#ifndef WSAEPROTOTYPE
#define WSAEPROTOTYPE EPROTOTYPE
#endif
#ifndef WSAENOPROTOOPT
#define WSAENOPROTOOPT ENOPROTOOPT
#endif
#ifndef WSAEPROTONOSUPPORT
#define WSAEPROTONOSUPPORT EPROTONOSUPPORT
#endif
#ifndef WSAESOCKTNOSUPPORT
#define WSAESOCKTNOSUPPORT ESOCKTNOSUPPORT
#endif
#ifndef WSAEOPNOTSUPP
#define WSAEOPNOTSUPP EOPNOTSUPP
#endif
#ifndef WSAEPFNOSUPPORT
#define WSAEPFNOSUPPORT EPFNOSUPPORT
#endif
#ifndef WSAEAFNOSUPPORT
#define WSAEAFNOSUPPORT EAFNOSUPPORT
#endif
#ifndef WSAEADDRINUSE
#define WSAEADDRINUSE EADDRINUSE
#endif
#ifndef WSAEADDRNOTAVAIL
#define WSAEADDRNOTAVAIL EADDRNOTAVAIL
#endif
#ifndef WSAENETDOWN
#define WSAENETDOWN ENETDOWN
#endif
#ifndef WSAENETUNREACH
#define WSAENETUNREACH ENETUNREACH
#endif
#ifndef WSAENETRESET
#define WSAENETRESET ENETRESET
#endif
#ifndef WSAECONNABORTED
#define WSAECONNABORTED ECONNABORTED
#endif
#ifndef WSAECONNRESET
#define WSAECONNRESET ECONNRESET
#endif
#ifndef WSAENOBUFS
#define WSAENOBUFS ENOBUFS
#endif
#ifndef WSAEISCONN
#define WSAEISCONN EISCONN
#endif
#ifndef WSAENOTCONN
#define WSAENOTCONN ENOTCONN
#endif
#ifndef WSAESHUTDOWN
#define WSAESHUTDOWN ESHUTDOWN
#endif
#ifndef WSAETOOMANYREFS
#define WSAETOOMANYREFS ETOOMANYREFS
#endif
#ifndef WSAETIMEDOUT
#define WSAETIMEDOUT ETIMEDOUT
#endif
#ifndef WSAECONNREFUSED
#define WSAECONNREFUSED ECONNREFUSED
#endif
#ifndef WSAEDISCON
#define WSAEDISCON 10101
#endif
#ifndef WSAELOOP
#define WSAELOOP ELOOP
#endif
#ifndef WSAENAMETOOLONG
#define WSAENAMETOOLONG ENAMETOOLONG
#endif
#ifndef WSAEHOSTDOWN
#define WSAEHOSTDOWN EHOSTDOWN
#endif
#ifndef WSAEHOSTUNREACH
#define WSAEHOSTUNREACH EHOSTUNREACH
#endif
#ifndef WSAENOTEMPTY
#define WSAENOTEMPTY ENOTEMPTY
#endif
#ifndef WSAEPROCLIM
#define WSAEPROCLIM 10067
#endif
#ifndef WSAEUSERS
#define WSAEUSERS EUSERS
#endif
#ifndef WSAEDQUOT
#define WSAEDQUOT EDQUOT
#endif
#ifndef WSAESTALE
#define WSAESTALE ESTALE
#endif
#ifndef WSAEREMOTE
#define WSAEREMOTE EREMOTE
#endif
#ifndef WSASYSNOTREADY
#define WSASYSNOTREADY 10091
#endif
#ifndef WSAVERNOTSUPPORTED
#define WSAVERNOTSUPPORTED 10092
#endif
#ifndef WSANOTINITIALISED
#define WSANOTINITIALISED 10093
#endif
#ifndef WSAHOST_NOT_FOUND
#define WSAHOST_NOT_FOUND HOST_NOT_FOUND
#endif
#ifndef WSATRY_AGAIN
#define WSATRY_AGAIN TRY_AGAIN
#endif
#ifndef WSANO_RECOVERY
#define WSANO_RECOVERY NO_RECOVERY
#endif
#ifndef WSANO_DATA
#define WSANO_DATA NO_DATA
#endif

#ifndef MAKEWORD
#define MAKEWORD(low, high) (static_cast<WORD>((static_cast<BYTE>(low)) | (static_cast<WORD>(static_cast<BYTE>(high)) << 8)))
#endif

inline int WSAStartup(WORD, WSADATA * data)
{
    if (data != nullptr) {
        std::memset(data, 0, sizeof(WSADATA));
        data->wVersion = MAKEWORD(2, 2);
        data->wHighVersion = data->wVersion;
    }
    return 0;
}

inline int WSACleanup()
{
    return 0;
}

inline int WSAGetLastError()
{
    return errno;
}

inline void WSASetLastError(int error)
{
    errno = error;
}

inline int closesocket(SOCKET socket_handle)
{
    return ::close(socket_handle);
}

inline int ioctlsocket(SOCKET socket_handle, long command, u_long * argument)
{
    return ::ioctl(socket_handle, command, argument);
}

inline int getsockopt(SOCKET socket_handle, int level, int option_name, char * option_value, int * option_length)
{
    socklen_t native_length = (option_length != nullptr) ? static_cast<socklen_t>(*option_length) : 0;
    const int result = ::getsockopt(socket_handle, level, option_name, option_value, (option_length != nullptr) ? &native_length : nullptr);
    if (option_length != nullptr) {
        *option_length = static_cast<int>(native_length);
    }
    return result;
}

inline int recvfrom(SOCKET socket_handle, char * buffer, int length, int flags, LPSOCKADDR address, int * address_length)
{
    socklen_t native_length = (address_length != nullptr) ? static_cast<socklen_t>(*address_length) : 0;
    const int result = static_cast<int>(::recvfrom(socket_handle, buffer, static_cast<std::size_t>(length), flags, address, (address_length != nullptr) ? &native_length : nullptr));
    if (address_length != nullptr) {
        *address_length = static_cast<int>(native_length);
    }
    return result;
}

#endif
