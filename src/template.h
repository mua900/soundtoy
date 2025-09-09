#pragma once

#include "common.h"

template <typename T>
struct DArray
{
	T* data = NULL;
	int size = 0;
	int cap = 0;

	DArray() {}
	DArray(int cap) {
		data = new T[cap];
		cap = cap;
	}

	T get(int index)
	{
		if (index >= size) panic("Out of bounds array access");
		return data[index];
	}

	void add(T elem)
	{
		if (size + 1 >= cap)
		{
			resize();
		}

		data[size] = elem;
		size += 1;
	}

	void resize()
	{
		int ncap = cap * 2;
		T* ndata = new T[ncap];
		for (int bucket_index = 0; bucket_index < size; bucket_index++)
		{
			ndata[bucket_index] = data[bucket_index];
		}
		delete[](data);
		data = ndata;
		cap *= 2;
	}

	void free()
	{
		delete[](data);
		size = 0;
		cap = 0;
	}
};

template <typename T>
struct Array
{
	T* data = NULL;
	int size = 0;

	Array() {}
	Array(T* data, int size) : data(data), size(size) {}
	Array(DArray<T> darray) : data(darray.data), size(darray.size) {}

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
		T elements[BUCKET_SIZE];
		uint16_t mask = 0;
	};

	DArray<Bucket*> buckets;

	// initial allocation is a big continious memory block
	Bucket_List(int p_bucket_count)
		:
		buckets(p_bucket_count)
	{
		Bucket* buckets_mem = new Bucket[p_bucket_count];
		for (int bucket_index = 0; bucket_index < p_bucket_count; bucket_index++) {
			Bucket* bucket = &buckets_mem[bucket_index];
			bucket->occupancy_mask = 0;
			buckets.data[bucket_index] = bucket;
		}
	}

	int add(T& elem)
	{
		for (int bucket_index = 0; bucket_index < buckets.size; bucket_index++)
		{
			Bucket* bucket = buckets[bucket_index];
			auto mask = bucket->mask;
			if (mask == BUCKET_FULL_MASK)
			{
				continue;
			}

			auto mask_r = ~mask;
			int index = pop_lsb(&mask_r);

			bucket->elements[index] = elem;
			bucket->mask |= BIT(index);
			return bucket_index * BUCKET_SIZE + index;
		}

		Bucket* nbucket = new Bucket;
		buckets.add(nbucket);

		nbucket->elements[0] = elem;
		nbucket->mask |= BIT(0);
		return buckets.size * BUCKET_SIZE;
	}

	T* get(int id)
	{
		int bucket_index = id / BUCKET_SIZE;
		int index = id % BUCKET_SIZE;
		return &buckets[bucket_index]->elements[index];
	}

	void remove(int id)
	{
		int bucket_index = id / BUCKET_SIZE;
		int index = id % BUCKET_SIZE;
		if (bucket_index >= buckets.size)
		{
			LOG_ERROR("Bucket_List: removal attempt from out of bounds bucket");
			return;
		}

		buckets.data[bucket_index]->mask &= ~BIT(index);
	}
};
