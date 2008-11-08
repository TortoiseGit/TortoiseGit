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

class CFullGraphNode;
class CVisibleGraph;
class CVisibleGraphNode;

/**
* Base interface for all binary options.
*/

class IRevisionGraphOption
{
public:

    /// make all interface destructors virtual

    virtual ~IRevisionGraphOption() {}

    /// toolbar button ID

    virtual UINT CommandID() const = 0;

    /// controls the execution order. 
    /// Lower numbers take precedence before higher ones.

    virtual int Priority() const = 0; 

    /// false -> currently grayed out

    virtual bool IsAvailable() const = 0;

    /// button shown as "pressed"

    virtual bool IsSelected() const = 0;

    /// The actual option may be "reversed".
    /// IsActive() should either *always* be equal to IsSelected()
    /// or *always* be equal to !IsSelected().

    virtual bool IsActive() const = 0; 

    /// for simple "on-click" handling

    virtual void ToggleSelection() = 0;

    /// will be called before a new graph gets created.
    /// Use this method to reset internal caches etc.

    virtual void Prepare() = 0;

    /// required typedef for multi-interface support

    typedef IRevisionGraphOption I;
};

/**
* Base interface extension for options that must be applied onto
* the graph nodes in a particular traversal order.
*/

class IOrderedTraversalOption : public IRevisionGraphOption
{
public:

    /// breadth-first traversal

    virtual bool WantsCopiesFirst() const = 0;

    /// if false, start with the HEAD / branch end nodes

    virtual bool WantsRootFirst() const = 0;

};

/**
* Container base class for all revision graph options.
*
* Options are expected to be created on the heap and
* to auto-add themselves to an instance of this class.
* That instance will take over ownership of the options.
*
* Every option is expected to provide its own class,
* i.e. no two options of the same class are allowed
* in any given CRevisionGraphOptionList instance.
*
* Individual options as well as sub-lists are being
* accessed through RTTI-based filtering.
*/

class CRevisionGraphOptionList
{
private:

    /// (only) this base class owns all the options

    std::vector<IRevisionGraphOption*> options;

protected:

    /// options owner and filter management

    template<class T, class I> T GetFilteredList() const;

    /// utility method

    IRevisionGraphOption* GetOptionByID (UINT id) const;

    /// construction / destruction (frees all owned options)

    CRevisionGraphOptionList();
    virtual ~CRevisionGraphOptionList();

public:

    /// member access

    size_t count() const;

    const IRevisionGraphOption* operator[](size_t index) const;
    IRevisionGraphOption* operator[](size_t index);

    /// find a particular option

    template<class T> T* GetOption() const;

    /// called by CRevisionGraphOption constructor

    void Add (IRevisionGraphOption* option);

    /// menu interaction

    bool IsAvailable (UINT id) const;
    bool IsSelected (UINT id) const;

    void ToggleSelection (UINT id);

    /// registry encoding

    DWORD GetRegistryFlags() const;
    void SetRegistryFlags (DWORD flags, DWORD mask);

    /// called before applying the options

    void Prepare();
};

// options owner and filter management

template<class I>
bool AscendingPriority (I* lhs, I* rhs)
{
    return lhs->Priority() < rhs->Priority();
}

template<class T, class I> 
T CRevisionGraphOptionList::GetFilteredList() const
{
    // get filtered options

    std::vector<I*> filteredOptions;
    for (size_t i = 0, count = options.size(); i < count; ++i)
    {
        I* option = dynamic_cast<I*>(options[i]);
        if ((option != NULL) && options[i]->IsActive())
            filteredOptions.push_back (option);
    }

    // sort them by priority

    std::stable_sort ( filteredOptions.begin()
                     , filteredOptions.end()
                     , AscendingPriority<I>);

    // create list

    return T (filteredOptions);
}

// find a particular option

template<class T> 
T* CRevisionGraphOptionList::GetOption() const
{
    for (size_t i = 0, count = options.size(); i < count; ++i)
    {
        T* result = dynamic_cast<T*>(options[i]);
        if (result != NULL)
            return result;
    }

    // should not happen 

    assert (0);

    return NULL;
}
