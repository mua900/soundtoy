#pragma once

#include "common.h"

struct Find_Result {
	int index = 0;
	bool found = false;
};

template <typename T>
struct DArray
{
	T* m_data = NULL;
	int m_size = 0;
	int m_cap = 0;

	DArray() {}
	DArray(int cap) {
		m_data = new T[cap];
		m_cap = cap;
	}

	T get(int index)
	{
		if (index >= m_size) panic("Out of bounds array access");
		return m_data[index];
	}

	T& get_ref(int index) {
		if (index >= m_size) panic("Out of bounds array access");
		return m_data[index];
	}

	int add(T elem)
	{
		int ret_index = m_size;
		if (m_size + 1 >= m_cap)
		{
			resize();
		}

		m_data[m_size] = elem;
		m_size += 1;
		return ret_index;
	}

	int add_unique(T elem)
	{
		Find_Result find_result = find(elem);
		if (find_result.found)
		{
			return find_result.index;
		}

		return add(elem);
	}

	Find_Result find(T& elem) {
		for (int i = 0; i < m_size; i++)
		{
			if (m_data[i] == elem)
			{
				return Find_Result {i, true};
			}
		}

		return Find_Result {0, false};
	}

	void resize()
	{
		int ncap = m_cap ? (m_cap * 2) : 8;
		T* ndata = new T[ncap];
		for (int i = 0; i < m_size; i++)
		{
			ndata[i] = m_data[i];
		}
		delete[](m_data);
		m_data = ndata;
		m_cap = ncap;
	}

	bool is_empty()
	{
		return m_size == 0;
	}

	T pop()
	{
		if (is_empty())
		{
			return nullptr;
		}

		m_size -= 1;
		return m_data[m_size - 1];
	}

	void free()
	{
		if (m_data)
		{
			delete[](m_data);
			m_data = nullptr;

			m_size = 0;
			m_cap = 0;
		}
	}
};

template <typename T>
struct Array
{
	T* data = NULL;
	int size = 0;

	Array() {}
	Array(T* data, int size) : data(data), size(size) {}
	Array(DArray<T> darray) : data(darray.m_data), size(darray.m_size) {}

	T get(int index)
	{
		if (index >= size) panic("Out of bounds array access");
		return data[index];
	}
};


#define BUCKET_SIZE				16
#define BUCKET_FULL_MASK		((uint16_t)((1U << BUCKET_SIZE) - 1))

template <typename T>
struct Bucket_List {
	struct Bucket {
		T elements[BUCKET_SIZE] = {};
		uint16_t occupancy_mask = 0;
	};

	DArray<Bucket*> buckets;

	Bucket_List(int p_bucket_count)
		:
		buckets(p_bucket_count)
	{
		Bucket* buckets_mem = new Bucket[p_bucket_count];

		// @todo fix if we decide that this should be able to shrink in size and discard unused buckets
		// then every bucket needs to be a seperate allocation.

		for (int bucket_index = 0; bucket_index < p_bucket_count; bucket_index++) {
			Bucket* bucket = &buckets_mem[bucket_index];
			bucket->occupancy_mask = 0;
			buckets.m_data[bucket_index] = bucket;
		}
	}

	unsigned int add(T& elem)
	{
		for (unsigned int bucket_index = 0; bucket_index < buckets.m_size; bucket_index++)
		{
			Bucket* bucket = buckets[bucket_index];
			auto mask = bucket->occupancy_mask;
			if (mask == BUCKET_FULL_MASK)
			{
				continue;
			}

			auto mask_r = ~mask;
			unsigned int index = pop_lsb(&mask_r);

			bucket->elements[index] = elem;
			bucket->occupancy_mask |= BIT(index);
			return bucket_index * BUCKET_SIZE + index;
		}

		Bucket* nbucket = new Bucket;
		buckets.add(nbucket);

		nbucket->elements[0] = elem;
		nbucket->occupancy_mask |= BIT(0);
		return buckets.m_size * BUCKET_SIZE;
	}

	T* get(unsigned int id)
	{
		unsigned int bucket_index = id / BUCKET_SIZE;
		unsigned int index = id % BUCKET_SIZE;
		return &buckets[bucket_index]->elements[index];
	}

	void remove(unsigned int id)
	{
		unsigned int bucket_index = id / BUCKET_SIZE;
		unsigned int index = id % BUCKET_SIZE;
		if (bucket_index >= buckets.m_size)
		{
			LOG_ERROR("Bucket_List: removal attempt from out of bounds bucket");
			return;
		}

		buckets.m_data[bucket_index]->occupancy_mask &= ~BIT(index);
	}
};
