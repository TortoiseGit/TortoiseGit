// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

template<class T>
class simple_list
{
public:

    /// factory type

    class factory
    {
    private:

        boost::pool<> pool;

    public:

        factory()
            : pool (sizeof (simple_list<T>), 1024)
        {
        }

        void insert (T* item, simple_list<T>*& next)
        {
            simple_list<T> * result = static_cast<simple_list<T> *>(pool.malloc());
            new (result) simple_list<T> (item, next);
            next = result;
        }

        T* remove (simple_list<T>*& node)
        {
            simple_list<T>* temp = node->_next;
            T* item = node->item;

            node->~simple_list<T>();
            pool.free (node);

            node = temp;

            return item;
        }
    };

    friend class factory;

private:

    T* item;
    simple_list<T>* _next;

    /// no public constructon / destruction

    simple_list (T* item, simple_list<T>* next = NULL)
        : item (item), _next (next)
    {
    }

    ~simple_list(void)
    {
    }

public:

    /// data access

    T* value() const
    {
        return item;
    }
    T*& value()
    {
        return item;
    }

    const simple_list<T>* next() const
    {
        return _next;
    }
    simple_list<T>*& next()
    {
        return _next;
    }
};
