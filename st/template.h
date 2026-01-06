#pragma once

#include "common.h"

template <typename T>
struct DArray {
private:
	T* m_data = NULL;
	int m_size = 0;
	int m_cap = 0;

public:
	T* data() { return m_data; }
	int size() const { return m_size; }

	DArray() {}
	DArray(int cap) {
		m_data = new T[cap];
		m_cap = cap;
	}

	void discard_data() {
		m_size = 0;
	}

	void reset() {
		if (m_data)
		{
			delete[](m_data);
			m_data = nullptr;

			m_size = 0;
			m_cap = 0;
		}
	}

	bool in_bounds(int index) {
		return index < m_size && index >= 0;
	}

	T get(int index) const {
		if (index >= m_size) panic("Out of bounds array access");
		return m_data[index];
	}

	T get_or_default(int index) const {
		if (index >= m_size) return T();
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
			grow();
		}

		m_data[m_size] = elem;
		m_size += 1;
		return ret_index;
	}

	bool remove_shift(int index) {
		if (!in_bounds(index)) {
			return false;
		}

		for (int i = index; i < m_size; i++) {
			m_data[i] = m_data[i+1];
		}

		m_size -= 1;

		return true;
	}

	bool remove(int index) {
		if (!in_bounds(index)) {
			return false;
		}

		m_size -= 1;

		m_data[index] = m_data[m_size-1];
		return true;
	}

	int add_unique(T elem) {
		Find_Result find_result = find(elem);
		if (find_result.found)
		{
			return find_result.index;
		}

		return add(elem);
	}

	Find_Result find(T& elem) const {
		for (int i = 0; i < m_size; i++)
		{
			if (m_data[i] == elem)
			{
				return Find_Result {i, true};
			}
		}

		return Find_Result {0, false};
	}

	bool is_empty()	const {
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
		reset();
	}

	void resize(int size) {
		T* new_buffer = new T[size];

		int new_size = (size < m_size) ? size : m_size;

		for (int i = 0; i < new_size; i++) {
			new_buffer[i] = m_data[i];
		}

		if (m_data) {
			delete[] m_data;
		}

		m_data = new_buffer;
		m_cap = size;
		m_size = new_size;
	}

	T* begin() {
		return m_data;
	}

	T* end() {
		return m_data + m_size;
	}

	const T* begin() const {
		return m_data;
	}

	const T* end() const {
		return m_data + m_size;
	}

private:
	void grow() {
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
};

template <typename T>
struct Array {
	T* data = NULL;
	int size = 0;

	Array() {}
	Array(T* data, int size) : data(data), size(size) {}
	Array(DArray<T> darray) : data(darray.data()), size(darray.size()) {}

	T& operator[](int index) {
		return data[index];
	}
	
	T get(int index) const {
		if (index >= size) panic("Out of bounds array access");
		return data[index];
	}

	T get_or_default(int index) const {
		if (index >= size) return T();
		return data[index];
	}

	Find_Result find(T& elem) const {
		for (int i = 0; i < size; i++)
		{
			if (data[i] == elem)
			{
				return Find_Result {i, true};
			}
		}

		return Find_Result {0, false};
	}

	void resize(int new_size) {
		T* new_buffer = new T[new_size];

		if (size > new_size)
			size = new_size;
		
		for (int i = 0; i < size; i++) {
			new_buffer[i] = data[i];
		}

		if (data) {
			delete[] data;
		}

		data = new_buffer;
		size = new_size;
	}

	T* begin() {
		return data;
	}

	T* end() {
		return data + size;
	}

	const T* begin() const {
		return data;
	}

	const T* end() const {
		return data + size;
	}
};
