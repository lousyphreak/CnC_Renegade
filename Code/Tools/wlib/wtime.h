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
wtime                      Neal Kettler

\****************************************************************************/
#ifndef WTIME_HEADER
#define WTIME_HEADER

#include <cstdint>


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

#ifndef _WINDOWS
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#else
#include <sys/timeb.h>
#include <winsock.h>
#endif

#include <time.h>
#include <string.h>

#include "wstypes.h"

class Wtime
{
  public:

  enum
  {
    POSITIVE=0,
    NEGATIVE=1
  };

              Wtime();  // init to system time
              Wtime( Wtime &other );
              Wtime( uint32_t    other );
             ~Wtime();

    void      Update();   // Update members sec & usec to system time
                          

    void      PrintTime(FILE *out) const;
    void      PrintTime(char *out) const;
    void      PrintDate(FILE *out) const;
    void      PrintDate(char *out) const;

    uint32_t    GetSec(void) const;    // Get member variable 'sec'
    uint32_t    GetUsec(void) const;   // Get member variable 'usec'
    void      SetSec(uint32_t newsec);
    void      SetUsec(uint32_t newusec);
    void      Set(uint32_t newsec,uint32_t newusec);
    int8_t      ParseDate(char *in);
    int8_t      FormatTime(char *out, char *format);

    struct timeval   *GetTimeval(void);
    void              GetTimevalMT(struct timeval &tv);

    uint32_t    GetSecond(void) const; // Second (0- 60) (60 is for a leap second)
    uint32_t    GetMinute(void) const; // Minute (0 - 59)
    uint32_t    GetHour(void) const;   // Hour (0-23)
    uint32_t    GetMDay(void) const;   // Day of Month (1-31)
    uint32_t    GetWDay(void) const;   // Day of Week  (1-7)
    uint32_t    GetYDay(void) const;   // Day of Year  (1-366)
    uint32_t    GetMonth(void) const;  // Month (1-12)
    uint32_t    GetYWeek(void) const;  // Week of Year (1-53)
    uint32_t    GetYear(void) const;   // Year (e.g. 1997)

    int8_t      GetSign(void) const;  // 0 = pos   1 = neg

    int       Compare(const Wtime &other) const;
    
    // comparisons
    int8_t   operator == ( const Wtime &other ) const;
    int8_t   operator != ( const Wtime &other ) const;
    int8_t   operator  < ( const Wtime &other ) const;
    int8_t   operator  > ( const Wtime &other ) const;
    int8_t   operator <= ( const Wtime &other ) const;
    int8_t   operator >= ( const Wtime &other ) const;

    // assignments
    Wtime   &operator = (const Wtime &other);
    Wtime   &operator = (const uint32_t    other);

    // math
    // signed
    void        SignedAdd(const Wtime &other);
    void        SignedSubtract(const Wtime &other);

    // unsigned
    Wtime   &operator += (const Wtime &other);
    Wtime   &operator -= (const Wtime &other);
    Wtime    operator +  (Wtime &other);
    Wtime    operator -  (Wtime &other);

    Wtime   &operator += (const uint32_t other);
    Wtime   &operator -= (const uint32_t other);
    Wtime    operator +  (uint32_t other);
    Wtime    operator -  (uint32_t other);

  protected:
    uint32_t    sec;     // seconds since Jan 1, 1970
    uint32_t    usec;    // microseconds (millionths of a second)
    int8_t      sign;    // for time differences 0 = pos 1 = neg
};

#endif
