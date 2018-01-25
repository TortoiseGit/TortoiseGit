/***************************************************************************
 *   Copyright (C) 2009, 2012 by Stefan Fuhrmann                           *
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

#include "JobBase.h"
#include <functional>
namespace async
{

/**
 * Execute a call to a function returning @a void asynchronuously
 * in the background.
 *
 * Accepts a pointer to some free function or member function plus
 * parameters and schedules it for execution it in the given job scheduler.
 * If no scheduler is given, the default scheduler is used (see
 * \ref CJobScheduler::GetDefault for details).
 *
 * Instances of this class must be allocated dynamically and will be
 * deleted automatically immediately after function execution finished.
 *
 * Please note that the parameters must remain valid until the call
 * actually got executed. Therefore, only pointer and value parameter
 * are possible; reference parameters won't compile.
 */

class CAsyncCall : public CJobBase
{
private:

    class ICall
    {
    public:

        virtual ~ICall() {};
        virtual void Execute() = 0;
    };

    class CCallStd : public ICall
    {
    private:

        std::function<void()> func;

    public:

        CCallStd (std::function<void()> f)
            : func (f)
        {
        }

        virtual void Execute() override
        {
            func();
        }
    };

    class CCall0 : public ICall
    {
    private:

        void (*func)();

    public:

        CCall0 (void (*func)())
            : func (func)
        {
        }

        virtual void Execute() override
        {
            (*func)();
        }
    };

    template<class C>
    class CCallMember0 : public ICall
    {
    private:

        C* instance;
        void (C::*func)();

    public:

        CCallMember0 (C* instance, void (C::*func)())
            : instance (instance)
            , func (func)
        {
        }

        virtual void Execute() override
        {
            (instance->*func)();
        }
    };

    template<class T1>
    class CCall1 : public ICall
    {
    private:

        T1 parameter1;
        void (*func)(T1);

    public:

        CCall1 (void (*func)(T1), T1 parameter1)
            : parameter1 (parameter1)
            , func (func)
        {
        }

        virtual void Execute() override
        {
            (*func)(parameter1);
        }
    };

    template<class C, class T1>
    class CCallMember1 : public ICall
    {
    private:

        C* instance;
        T1 parameter1;
        void (C::*func)(T1);

    public:

        CCallMember1 (C* instance, void (C::*func)(T1), T1 parameter1)
            : instance (instance)
            , parameter1 (parameter1)
            , func (func)
        {
        }

        virtual void Execute() override
        {
            (instance->*func)(parameter1);
        }
    };

    template<class T1, class T2>
    class CCall2 : public ICall
    {
    private:

        T1 parameter1;
        T2 parameter2;
        void (*func)(T1, T2);

    public:

        CCall2 (void (*func)(T1, T2), T1 parameter1, T2 parameter2)
            : parameter1 (parameter1)
            , parameter2 (parameter2)
            , func (func)
        {
        }

        virtual void Execute() override
        {
            (*func)(parameter1, parameter2);
        }
    };

    template<class C, class T1, class T2>
    class CCallMember2 : public ICall
    {
    private:

        C* instance;
        T1 parameter1;
        T2 parameter2;
        void (C::*func)(T1, T2);

    public:

        CCallMember2 (C* instance, void (C::*func)(T1, T2), T1 parameter1, T2 parameter2)
            : instance (instance)
            , parameter1 (parameter1)
            , parameter2 (parameter2)
            , func (func)
        {
        }

        virtual void Execute() override
        {
            (instance->*func)(parameter1, parameter2);
        }
    };

    // result and actual "future"

    ICall* call;

    // actually execute the call

    virtual void InternalExecute()
    {
        call->Execute();
    }

public:

    // construct futures for all sorts of callable items

    CAsyncCall (void (*func)(), CJobScheduler* scheduler = NULL)
        : call (NULL)
    {
        call = new CCall0 (func);
        Schedule (true, scheduler);
    }

    CAsyncCall (std::function<void()> func, CJobScheduler* scheduler = NULL)
        : call (NULL)
    {
        call = new CCallStd (func);
        Schedule (true, scheduler);
    }

    template<class C>
    CAsyncCall (C* instance, void (C::*func)(), CJobScheduler* scheduler = NULL)
        : call (NULL)
    {
        call = new CCallMember0<C>(instance, func);
        Schedule (true, scheduler);
    }

    template<class T1>
    CAsyncCall (void (*func)(T1), T1 parameter1, CJobScheduler* scheduler = NULL)
        : call (NULL)
    {
        call = new CCall1<T1>(func, parameter1);
        Schedule (true, scheduler);
    }

    template<class C, class T1>
    CAsyncCall (C* instance, void (C::*func)(T1), T1 parameter1, CJobScheduler* scheduler = NULL)
        : call (NULL)
    {
        call = new CCallMember1<C, T1>(instance, func, parameter1);
        Schedule (true, scheduler);
    }

    template<class T1, class T2>
    CAsyncCall (void (*func)(T1,T2), T1 parameter1, T2 parameter2, CJobScheduler* scheduler = NULL)
        : call (NULL)
    {
        call = new CCall2<T1, T2>(func, parameter1, parameter2);
        Schedule (true, scheduler);
    }

    template<class C, class T1, class T2>
    CAsyncCall (C* instance, void (C::*func)(T1,T2), T1 parameter1, T2 parameter2, CJobScheduler* scheduler = NULL)
        : call (NULL)
    {
        call = new CCallMember2<C, T1, T2>(instance, func, parameter1, parameter2);
        Schedule (true, scheduler);
    }

    // cleanup

    ~CAsyncCall()
    {
        delete call;
    }
};

}