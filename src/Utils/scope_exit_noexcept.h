// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit

// based on public domain implementation:
// https://codeproject.cachefly.net/Articles/124130/Simple-Look-Scope-Guard-for-Visual-C

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

template <typename D>
class scope_exit_t
{
public:
	explicit scope_exit_t(D&& f) : func(f) {}
	scope_exit_t(scope_exit_t&& s) : func(s.func) {}

	~scope_exit_t() { func(); }

private:
	// Prohibit construction from lvalues.
	scope_exit_t(D&);

	// Prohibit copying.
	scope_exit_t(const scope_exit_t&);
	scope_exit_t& operator=(const scope_exit_t&);

	// Prohibit new/delete.
	void* operator new(size_t) = delete;
	void* operator new[](size_t) = delete;
	void operator delete(void*) = delete;
	void operator delete[](void*) = delete;

	const D func;
};

struct scope_exit_helper
{
	template <typename D>
	scope_exit_t<D> operator<< (D&& f) {
		return scope_exit_t<D>(std::forward<D>(f));
	}
};

#define SCOPE_EXIT_CAT2(x, y) x##y
#define SCOPE_EXIT_CAT1(x, y) SCOPE_EXIT_CAT2(x, y)
#define SCOPE_EXIT auto SCOPE_EXIT_CAT1(scope_exit_, __COUNTER__) \
	= scope_exit_helper() << [&]
