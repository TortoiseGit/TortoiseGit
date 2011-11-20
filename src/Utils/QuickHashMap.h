// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "QuickHash.h"


/**
 * A quick associative container that maps K (key) to V (value) instances.
 * K must implicitly convert to size_t.
 */
template<class K, class V>
class quick_hash_map
{
private:

    /// our actual data container
    struct element_type
    {
        K key;
        V value;

        element_type (K key, const V& value)
            : key (key), value (value)
        {
        }
    };

    typedef std::vector<element_type> data_type;
    data_type data;

	/**
	 * A simple string hash function that satisfies quick_hash interface
	 * requirements.
	 *
	 * NULL strings are supported and are mapped to index 0.
	 * Hence, the dictionary must contain the empty string at index 0.
	 */
	class CHashFunction
	{
	private:

		/// the data container we index with the hash
		/// (used to map key -> (key, value))
        typedef typename quick_hash_map<K, V>::data_type data_type; 
        data_type* data;

	public:

		// simple construction

		CHashFunction (data_type* map)
            : data (map)
        {
        }

		// required typedefs and constants

		typedef K value_type;
		typedef size_t index_type;

		enum {NO_INDEX = LogCache::NO_INDEX};

		/// the actual hash function
        size_t operator() (value_type value) const
        {
            return value;
        }

		/// dictionary lookup
		value_type value (index_type index) const
        {
            return (*data)[index].key;
        }

		/// lookup and comparison
		bool equal (value_type value, index_type index) const
        {
            return value == (*data)[index].key;
        }
	};

    friend class CHashFunction;

    // hash index over this container

    quick_hash<CHashFunction> hash;

public:

    // publicly available types

	typedef typename K key_type;
	typedef typename V value_type;

    class const_iterator
    {
    private:

        typedef typename quick_hash_map<K,V>::data_type::const_iterator iterator;
        typedef typename quick_hash_map<K,V>::element_type value_type;

        iterator iter;

    public:

        // construction

        const_iterator (iterator iter)
            : iter (iter)
        {
        }

		// pointer-like behavior

		const V& operator*() const
		{
			return iter->value;
		}

        const value_type* operator->() const
		{
			return &*iter;
		}

        // comparison

        bool operator== (const const_iterator& rhs) const
        {
            return iter == rhs.iter;
        }

        bool operator!= (const const_iterator& rhs) const
        {
            return iter != rhs.iter;
        }

		// move pointer

		const_iterator& operator++()		// prefix
		{
            iter++;
			return *this;
		}

		const_iterator operator++(int)	// postfix
		{
			const_iterator result (*this);
			operator++();
			return result;
		}

        const_iterator& operator+= (size_t diff)
        {
            iter += diff;
            return *this;
        }

        const_iterator& operator-= (size_t diff)
        {
            iter += diff;
            return *this;
        }

        const_iterator operator+ (size_t diff) const
        {
            return const_iterator (*this) += diff;
        }

        const_iterator operator- (size_t diff) const
        {
            return const_iterator (*this) -= diff;
        }
    };

    friend class const_iterator;

	// construction
    // (default-implemented destruction works as desired)

	quick_hash_map() 
		: hash (CHashFunction (&data))
	{
	}
	
	quick_hash_map (const quick_hash_map& rhs) 
		: hash (CHashFunction (&data))
	{
		operator=(rhs);
	}

	// assignment

	quick_hash_map& operator= (const quick_hash_map& rhs) 
	{
		hash = rhs.hash;
		data = rhs.data;

		return *this;
	}
	
	// data access
	
	const_iterator begin() const
    {
        return data.begin();
    }

	const_iterator end() const
    {
        return data.end();
    }

    const_iterator find (key_type key) const
	{
        size_t index = hash.find (key);
        return index == LogCache::NO_INDEX 
            ? end() 
            : begin() + index;
	}
	
    // insert a new key, value pair

	void insert (key_type key, const value_type& value)
	{
		assert (find (key) == end());
		
        data.push_back (element_type (key, value));
        hash.insert (key, data.size()-1);
	}

	void reserve (size_t min_bucket_count)
	{
        if (data.capacity() < min_bucket_count)
        {
            data.reserve (min_bucket_count);
            hash.reserve (min_bucket_count);
        }
	}

	// get rid of all entries

	void clear()
	{
        hash.clear();
        data.clear();
	}
};
