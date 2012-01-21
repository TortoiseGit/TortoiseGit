// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2011 - TortoiseSVN

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

/**
 * \ingroup Utils
 * Implements a queue like container which avoids storing duplicates.
 * If an entry is added that's already in the queue, it gets moved to
 * the end of the queue.
 *
 * \code
 * UniqueQueue<CString> myQueue;
 * myQueue.Push(CString(L"one"));
 * myQueue.Push(CString(L"two"));
 * myQueue.Push(CString(L"one"));  // "one" already exists, so moved to the end of the queue
 * myQueue.Push(CString(L"three"));
 * myQueue.Push(CString(L"three"));
 *
 * ATLASSERT(myQueue.Pop().Compare(L"two") == 0);   // because "one" got moved
 * ATLASSERT(myQueue.Pop().Compare(L"one") == 0);
 * ATLASSERT(myQueue.Pop().Compare(L"three") == 0);
 * \endcode
 */
template <class T>
class UniqueQueue
{
public:
	UniqueQueue();
	~UniqueQueue();

	size_t			Push(T value);
	T				Pop();
	size_t			erase(T value);
	size_t			size() { return m_Queue.size(); }
private:
	typedef struct UniqueQueueStruct
	{
		T			value;
		size_t		priority;
	};
	std::map<T, size_t>				m_QueueTMap;
	std::deque<UniqueQueueStruct>	m_Queue;
	size_t							m_highestValue;
};

template <class T>
UniqueQueue<T>::UniqueQueue()
	: m_highestValue(0)
{
}

template <class T>
UniqueQueue<T>::~UniqueQueue()
{

}

template <class T>
size_t UniqueQueue<T>::Push( T value )
{
	std::map<T, size_t>::iterator it = m_QueueTMap.find(value);
	if (it != m_QueueTMap.end())
	{
		// value is already in the queue: we don't allow duplicates
		// so just move the existing value to the end of the queue
		for (std::deque<UniqueQueueStruct>::iterator qIt = m_Queue.begin(); qIt != m_Queue.end(); ++qIt)
		{
			if (qIt->priority == it->second)
			{
				m_Queue.erase(qIt);
				break;
			}
		}
		it->second = m_highestValue;
		UniqueQueueStruct s;
		s.priority = m_highestValue++;
		s.value = it->first;
		m_Queue.push_back(s);
	}
	else
	{
		m_QueueTMap.insert(it, std::map<T, size_t>::value_type(value, m_highestValue));
		UniqueQueueStruct s;
		s.priority = m_highestValue++;
		s.value = value;
		m_Queue.push_back(s);
		if (m_highestValue == 0)
		{
			// overflow of priority value
			// recreate the whole queue from scratch
			std::map<T, size_t> tempQueue = m_QueueTMap;
			m_QueueTMap.clear();
			m_Queue.clear();

			for (std::map<T, size_t>::const_iterator tempIt = tempQueue.begin(); tempIt != tempQueue.end(); ++tempIt)
			{
				m_QueueTMap.insert(m_QueueTMap.end(), std::map<T, size_t>::value_type(tempIt->first, m_highestValue));
				UniqueQueueStruct s2;
				s2.priority = m_highestValue++;
				s2.value = tempIt->first;
				m_Queue.push_back(s2);
			}
		}
	}

	return m_Queue.size();
}

template <class T>
T UniqueQueue<T>::Pop()
{
	if (m_Queue.size() == 0)
		return T();

	T value = m_Queue.front().value;
	m_Queue.pop_front();
	m_QueueTMap.erase(value);

	if (m_Queue.size() == 0)
		m_highestValue = 0;

	return value;
}

template <class T>
size_t UniqueQueue<T>::erase( T value )
{
	std::map<T, size_t>::iterator it = m_QueueTMap.find(value);
	if (it != m_QueueTMap.end())
	{
		for (std::deque<UniqueQueueStruct>::iterator qIt = m_Queue.begin(); qIt != m_Queue.end(); ++qIt)
		{
			if (qIt->priority == it->second)
			{
				m_Queue.erase(qIt);
				break;
			}
		}
		m_QueueTMap.erase(it);
	}

	return m_QueueTMap.size();
}
