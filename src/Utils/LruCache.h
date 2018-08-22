// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseSVN

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

#include <unordered_map>
#include <list>

template<typename key_t, typename value_t>
class LruCache
{
public:
	LruCache(size_t maxSize)
		: maxSize(maxSize)
	{
	}

	void insert_or_assign(const key_t & key, const value_t & val)
	{
		ItemsMap::iterator mapIt = itemsMap.find(key);
		if (mapIt == itemsMap.end())
		{
			evict(maxSize - 1);

			ItemsList::iterator listIt = itemsList.insert(itemsList.cend(), ListItem(key, val));
			itemsMap.insert(std::make_pair(key, listIt));
		}
		else
		{
			mapIt->second->val = val;
		}
	}

	const value_t * try_get(const key_t & key)
	{
		ItemsMap::const_iterator it = itemsMap.find(key);
		if (it == itemsMap.end())
			return nullptr;

		// Move last recently accessed item to the end.
		if (it->second != itemsList.end())
		{
			itemsList.splice(itemsList.end(), itemsList, it->second);
		}

		return &it->second->val;
	}

	void reserve(size_t size)
	{
		itemsMap.reserve(min(maxSize, size));
	}

	void clear()
	{
		itemsMap.clear();
		itemsList.clear();
	}

protected:
	void evict(size_t itemsToKeep)
	{
		for(ItemsList::iterator it = itemsList.begin();
			itemsList.size() > itemsToKeep && it != itemsList.end();)
		{
			itemsMap.erase(it->key);
			it = itemsList.erase(it);
		}
	}

private:
	struct ListItem
	{
		ListItem(const key_t & key, const value_t & val)
			: key(key), val(val)
		{
		}

		key_t key;
		value_t val;
	};

	typedef std::list<ListItem> ItemsList;
	typedef std::unordered_map<key_t, typename ItemsList::iterator> ItemsMap;

	size_t maxSize;
	ItemsMap itemsMap;
	ItemsList itemsList;
};
