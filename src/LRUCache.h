/*
	This is an LRU cache, for now its only use is caching Unicode glyphs
	by the TrueType renderer.
*/

#ifndef CRTERM_LRU_CACHE_H
#define CRTERM_LRU_CACHE_H

#include <list>
#include <string>
#include <unordered_map>

#define DEFAULT_LRU_CACHE_SIZE 10

template <class KeyType, class ValueType> class LRUCache
{
public:
	LRUCache(size_t size, void (*DestroyCallBack)(ValueType))
	{
		if (size < DEFAULT_LRU_CACHE_SIZE)
		{
			this->cache_size = DEFAULT_LRU_CACHE_SIZE;
		}
		else
		{
			this->cache_size = size;
		}
		this->DestroyCallBack = DestroyCallBack;
	}

	void Add(KeyType key, ValueType value)
	{
		/* Find if key exists */
		auto pos = item_map.find(key);
		/* It does not exist, time to add. */
		if (pos == item_map.end())
		{
			item_list.push_front(key);
			/* The iterator stores a permanent pointer to the key in item list */
			item_map[key] = { value, item_list.begin() };
			
			if (item_map.size() > this->cache_size)
			{
				/* If we have exceeded the size, erase the last key in item_list from key-value map */
				KeyType last_elem = item_list.back();
				this->DestroyCallBack(item_map[last_elem].first);
				item_map.erase(item_list.back());
				/* Now erase the key from the item list  */
				item_list.pop_back();
			}
		}
		else
		{
			/* 
				It exists, no need to update, first we remove the iterator from the items_list
				Note: pos->second refers to the value from the key-value pair of the dictionary,
				as the value itself is a pair, we need pos->second.second to get the iterator (see defn
				below)
			*/
			item_list.erase(pos->second.second);
			/* Then we push it back! */
			item_list.push_front(key);
			/* Update the iterator and value */
			item_map[key] = { value, item_list.begin() };
		}
	}

	bool Get(KeyType key, ValueType& value)
	{
		/* Find the item */
		auto pos = item_map.find(key);

		/* Did we find it? */
		if (pos == item_map.end())
		{
			/* No, return false. */
			return false;
		}

		/* Erase it from the item list, to add it back again at the top. */
		item_list.erase(pos->second.second);
		item_list.push_front(key);
		item_map[key] = { pos->second.first, item_list.begin() };
		value = pos->second.first;
		return true;
	}

private:
	std::unordered_map<KeyType, std::pair<ValueType, typename std::list<KeyType>::iterator>> item_map;
	std::list<KeyType> item_list;
	size_t cache_size;
	void (*DestroyCallBack)(ValueType);
};

#endif