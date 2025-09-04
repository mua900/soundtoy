#pragma once

template <typename T>
struct Array
{
	T* m_data = NULL;
	int m_size = 0;

	Array() {}
	Array(T* data, int size) : m_data(data), m_size(size) {}
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

	void add(T elem)
	{
		if (m_size + 1 >= m_cap)
		{
			resize();
		}

		m_data[m_size] = elem;
		m_size += 1;
	}

	void resize()
	{
		int ncap = m_cap * 2;
		T* ndata = new T[ncap];
		for (int i = 0; i < m_size; i++)
		{
			ndata[i] = m_data[i];
		}
		delete[](m_data);
		m_data = ndata;
		m_cap *= 2;
	}
};
