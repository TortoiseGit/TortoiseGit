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

namespace async
{

/**
 * Execute a call to a function asynchronuously in the background.
 * Return the result in \ref GetResult. The latter will wait for
 * the call to finished, if necessary.
 *
 * Accepts a pointer to some free function or member function plus
 * parameters and schedules it for execution it in the given job scheduler.
 * If no scheduler is given, the default scheduler is used (see
 * \ref CJobScheduler::GetDefault for details).
 *
 * Please note that the parameters must remain valid until the call
 * actually got executed. Therefore, only pointer and value parameter
 * are possible; reference parameters won't compile.
 */

template<class R>
class CFuture
{
private:

    class CFuture0 : public CJobBase
    {
    private:

        R (*func)();
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (*func)();
        }

    public:

        CFuture0 (R (*func)(), R& result, CJobScheduler* scheduler)
            : func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    template<class C>
    class CFutureMember0 : public CJobBase
    {
    private:

        C* instance;
        R (C::*func)();
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (instance->*func)();
        }

    public:

        CFutureMember0 (C* instance, R (C::*func)(), R& result, CJobScheduler* scheduler)
            : instance (instance)
            , func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    template<class T1>
    class CFuture1 : public CJobBase
    {
    private:

        T1 parameter1;
        R (*func)(T1);
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (*func)(parameter1);
        }

    public:

        CFuture1 (R (*func)(T1), R& result, T1 parameter1, CJobScheduler* scheduler)
            : parameter1 (parameter1)
            , func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    template<class C, class T1>
    class CFutureMember1 : public CJobBase
    {
    private:

        C* instance;
        T1 parameter1;
        R (C::*func)(T1);
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (instance->*func)(parameter1);
        }

    public:

        CFutureMember1 (C* instance, R (C::*func)(T1), R& result, T1 parameter1, CJobScheduler* scheduler)
            : instance (instance)
            , parameter1 (parameter1)
            , func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    template<class T1, class T2>
    class CFuture2 : public CJobBase
    {
    private:

        T1 parameter1;
        T2 parameter2;
        R (*func)(T1, T2);
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (*func)(parameter1, parameter2);
        }

    public:

        CFuture2 (R (*func)(T1, T2), R& result, T1 parameter1, T2 parameter2, CJobScheduler* scheduler)
            : parameter1 (parameter1)
            , parameter2 (parameter2)
            , func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    template<class C, class T1, class T2>
    class CFutureMember2 : public CJobBase
    {
    private:

        C* instance;
        T1 parameter1;
        T2 parameter2;
        R (C::*func)(T1, T2);
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (instance->*func)(parameter1, parameter2);
        }

    public:

        CFutureMember2 (C* instance, R (C::*func)(T1, T2), R& result, T1 parameter1, T2 parameter2, CJobScheduler* scheduler)
            : instance (instance)
            , parameter1 (parameter1)
            , parameter2 (parameter2)
            , func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    template<class T1, class T2, class T3>
    class CFuture3 : public CJobBase
    {
    private:

        T1 parameter1;
        T2 parameter2;
        T3 parameter3;
        R (*func)(T1, T2, T3);
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (*func)(parameter1, parameter2, parameter3);
        }

    public:

        CFuture3 (R (*func)(T1, T2, T3), R& result, T1 parameter1, T2 parameter2, T3 parameter3, CJobScheduler* scheduler)
            : parameter1 (parameter1)
            , parameter2 (parameter2)
            , parameter3 (parameter3)
            , func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    template<class C, class T1, class T2, class T3>
    class CFutureMember3 : public CJobBase
    {
    private:

        C* instance;
        T1 parameter1;
        T2 parameter2;
        T3 parameter3;
        R (C::*func)(T1, T2, T3);
        R& result;

    protected:

        virtual void InternalExecute() override
        {
            result = (instance->*func)(parameter1, parameter2, parameter3);
        }

    public:

        CFutureMember3 (C* instance, R (C::*func)(T1, T2, T3), R& result, T1 parameter1, T2 parameter2, T3 parameter3, CJobScheduler* scheduler)
            : instance (instance)
            , parameter1 (parameter1)
            , parameter2 (parameter2)
            , parameter3 (parameter3)
            , func (func)
            , result (result)
        {
            Schedule (false, scheduler);
        }
    };

    // result and actual "future"

    R result;
    CJobBase* job;

public:

    // construct futures for all sorts of callable items

    CFuture (R (*func)(), CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFuture0(func, result, scheduler);
    }

    template<class C>
    CFuture (C* instance, R (C::*func)(), CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFutureMember0<C>(instance, func, result, scheduler);
    }

    template<class T1>
    CFuture (R (*func)(T1), T1 parameter1, CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFuture1<T1>(func, result, parameter1, scheduler);
    }

    template<class C, class T1>
    CFuture (C* instance, R (C::*func)(T1), T1 parameter1, CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFutureMember1<C, T1>(instance, func, result, parameter1, scheduler);
    }

    template<class T1, class T2>
    CFuture (R (*func)(T1, T2), T1 parameter1, T2 parameter2, CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFuture2<T1, T2>
                    (func, result, parameter1, parameter2, scheduler);
    }

    template<class C, class T1, class T2>
    CFuture (C* instance, R (C::*func)(T1, T2), T1 parameter1, T2 parameter2, CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFutureMember2<C, T1, T2>
                    (instance, func, result, parameter1, parameter2, scheduler);
    }

    template<class T1, class T2, class T3>
    CFuture (R (*func)(T1, T2, T3), T1 parameter1, T2 parameter2, T3 parameter3, CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFuture3<T1, T2, T3>
                    (func, result, parameter1, parameter2, parameter3, scheduler);
    }

    template<class C, class T1, class T2, class T3>
    CFuture (C* instance, R (C::*func)(T1, T2, T3), T1 parameter1, T2 parameter2, T3 parameter3, CJobScheduler* scheduler = NULL)
        : job (NULL)
    {
        job = new CFutureMember3<C, T1, T2, T3>
                    (instance, func, result, parameter1, parameter2, parameter3, scheduler);
    }

    // cleanup

    ~CFuture()
    {
        job->Delete (true);
    }

    // ensure the function has returned and pass the result back to the caller

    const R& GetResult() const
    {
        job->WaitUntilDone();
        return result;
    }

    bool IsDone() const
    {
        return job->GetStatus() == IJob::done;
    }
};

}
