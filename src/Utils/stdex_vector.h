// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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

#include <vector>

namespace stdex {

	/**
	 * Subclass of the STL's vector class that allows the class to be used more
	 * freely in place of a C-style array.
	 * @author Damian Powell
	 */
	template <class T, class Al = std::allocator<T> >
	class vector : public std::vector<T, Al> {

	  public:

		/**
		 * Default constructor creates an empty vector.
		 */
		inline vector()
		{ }

		/**
		 * Creates a vector of n items where each item is initialized to its
		 * default value as defined by 'T()'
		 * @param n The number of items to allocate in the new vector.
		 */
		inline vector(size_type n)
			: std::vector<T, Al> (n)
		{ }

		/**
		 * Creates a vector of n items where each item is initialized to the
		 * specified value t.
		 * @param n The number of items to allocate in the new vector.
		 * @param t The value that each new item shall be initialized to.
		 */
		inline vector(size_type n, const T& t)
			: std::vector<T, Al> (n, t)
		{ }

		/**
		 * Index operator returns reference to the specified immutable item.
		 * No additional bounds checking is performed.
		 * @param i The index of the value to return a reference to.
		 * @return An immutable reference to the item at index i.
		 */
		inline const_reference operator [] (size_type i) const {
			ASSERT(i < size());
			return std::vector<T, Al>::operator [] (i);
		}

		/**
		 * Index operator returns a reference to the specified item. No
		 * additional bounds checking is performed.
		 * @param i The index of the value to return a reference to.
		 * @return A reference to the item at index i.
		 */
		inline reference operator [] (size_type i) {
			ASSERT(i < size());
			return std::vector<T, Al>::operator [] (i);
		}

		/**
		 * Conversion operator returns pointer to the immutable item at the
		 * beginning of this array of NULL if the array is empty.
		 * @param i The index of the value to return a reference to.
		 * @return A pointer to an immutable item or NULL.
		 */
		inline operator const_pointer () const {
			return empty() ? NULL : &operator[](0);
		}

		/**
		 * Conversion operator returns pointer to the item at the beginning of
		 * this array of NULL if the array is empty.
		 * @param i The index of the value to return a reference to.
		 * @return A pointer to an item or NULL.
		 */
		inline operator pointer () {
			return empty() ? NULL : &operator[](0);
		}

	};

}
