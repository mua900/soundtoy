#pragma once

#include "common.h"

struct Find_Result {
	int index = 0;
	bool found = false;
};

template <typename T>
struct DArray
{
private:
	T* m_data = NULL;
	int m_size = 0;
	int m_cap = 0;

public:
	T* data() { return m_data; }
	int size() { return m_size; }

	DArray() {}
	DArray(int cap) {
		m_data = new T[cap];
		m_cap = cap;
	}

	T get(int index) {
		if (index >= m_size) panic("Out of bounds array access");
		return m_data[index];
	}

	T& get_ref(int index) {
		if (index >= m_size) panic("Out of bounds array access");
		return m_data[index];
	}

	int add(T elem)	{
		int ret_index = m_size;
		if (m_size + 1 >= m_cap)
		{
			resize();
		}

		m_data[m_size] = elem;
		m_size += 1;
		return ret_index;
	}

	int add_unique(T elem) {
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

	void resize() {
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

	bool is_empty()	{
		return m_size == 0;
	}

	T pop()	{
		if (is_empty())
		{
			panic("Trying to pop from empty array");
		}

		m_size -= 1;
		return m_data[m_size - 1];
	}

	void free()	{
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
	Array(DArray<T> darray) : data(darray.data()), size(darray.size()) {}

	T get(int index)
	{
		if (index >= size) panic("Out of bounds array access");
		return data[index];
	}
};
