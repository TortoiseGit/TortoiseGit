/***************************************************************************
 *   Copyright (C) 2009 by Stefan Fuhrmann                                 *
 *   stefanfuhrmann@alice-dsl.de                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

namespace async
{

/**
* Just a simple critical section (i.e. recursive mutex)
* implementation with minimal overhead.
*/

class CCriticalSection
{
private:

    /// OS-specific critical section object

    CRITICAL_SECTION section;

public:

    /// construction

    CCriticalSection(void);

    /// destruction

    ~CCriticalSection(void);

    /// Acquire the mutex, i.e. enter the critical section

    void Acquire();

    /// Release the mutex, i.e. leave the critical section

    void Release();
};

// Mutex functionality

__forceinline void CCriticalSection::Acquire()
{
    EnterCriticalSection (&section);
}

__forceinline void CCriticalSection::Release()
{
    LeaveCriticalSection (&section);
}

/**
* RAII lock class for \ref CCriticalSection mutexes.
*/

class CCriticalSectionLock
{
private:

    CCriticalSection& section;

    // dummy assignment operator to silence the C4512 compiler warning
    // not implemented assignment operator (even is dummy and private) to 
    // get rid of cppcheck error as well as accidental use
    CCriticalSectionLock & operator=( const CCriticalSectionLock & ) /*{ section = ; return * this; }*/; 
public:

    __forceinline CCriticalSectionLock (CCriticalSection& section)
        : section (section)
    {
        section.Acquire();
    }

    __forceinline ~CCriticalSectionLock()
    {
        section.Release();
    }
};

}
