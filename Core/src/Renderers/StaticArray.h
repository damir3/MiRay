#pragma once

template< typename T, size_t N = 20 >
class StaticArray
{
public:

	inline StaticArray() :
		mSize(0), mStart(mElements)
	{
	}

	StaticArray(const StaticArray & src) : mSize(src.mSize), mStart(mElements + (src.mStart - src.mElements))
	{
		if (mSize > 0)
			::memcpy(mElements, src.mElements, sizeof(T) * mSize);
	}

	/// Size of the array
	inline size_t size() const
	{
		return mSize;
	}

	inline size_t capacity() const
	{
		return N;
	}

	/// Is array empty?
	inline bool empty() const
	{
		return mSize == 0;
	}

	/// Add element
	inline void push_back(const T & aElem)
	{
		assert(mSize != N);
		mStart[mSize++] = aElem;
	}

	/// Add element to start
	inline void push_front(const T & aElem)
	{
		assert(mSize != N);
		if (mStart == mElements) { // Is shift necessary ?
			for (T * i = mElements + mSize; i > mElements; --i) {
				*i = *(i - 1);
			}
		}
		*mStart = aElem;
		++mSize;
	}
	/// Remove element from back
	inline void pop_back()
	{
		assert(mSize != 0);
		--mSize;
	}

	/// Remove element from start
	inline void pop_front()
	{
		assert(mSize != 0);
		++mStart;
		--mSize;
	}

	/// Clear array
	inline void clear()
	{
		mSize = 0;
		mStart = mElements;
	}

	/// Access functions
	inline T & operator[](int i)
	{
		assert(mSize > i);
		return mStart[i];
	}

	inline const T & operator[](int i) const
	{
		assert(mSize > i);
		return mStart[i];
	}

	inline T & front()
	{
		assert(mSize > 0);
		return mStart[0];
	}

	inline T & back()
	{
		assert(mSize > 0);
		return mStart[mSize - 1];
	}

	/// Other functions
	inline void sort()
	{
		std::sort(mStart, mStart + mSize);
	}

	/// Iterator
	class iterator
	{
		friend StaticArray;
		inline iterator(T * ptr) :mPtr(ptr)
		{
		}
#ifdef DEBUGARRAY
		inline iterator(T * ptr, T * start, T * end) : mPtr(ptr), mStart(start), mEnd(end)
		{
		}
#endif
	public:
		inline iterator() :
			mPtr(0)
		{
		}

		inline iterator operator++()
		{
			++mPtr;
			return *this;
		}

		inline iterator operator++(int)
		{
			iterator tmp = *this;
			++mPtr;
			return tmp;
		}

		inline iterator operator--()
		{
			--mPtr;
			return *this;
		}

		inline iterator operator--(int)
		{
			iterator tmp = *this;
			--mPtr;
			return tmp;
		}

		inline const T* operator->() const
		{
			return mPtr;
		}

		inline T* operator->()
		{
			return mPtr;
		}

		inline const T & operator*() const
		{
			return *mPtr;
		}

		inline T & operator*()
		{
			return *mPtr;
		}

		inline bool operator==(const iterator &aIt)
		{
			return mPtr == aIt.mPtr;
		}

		inline bool operator!=(const iterator &aIt)
		{
			return mPtr != aIt.mPtr;
		}
	private:
		T * mPtr;
#ifdef DEBUGARRAY
		T * mStart, *mEnd;
#endif
	};

	/// Constant iterator
	class const_iterator
	{
		friend StaticArray;
		inline const_iterator(const T * ptr) :mPtr(ptr)
		{
		}
#ifdef DEBUGARRAY
		inline const_iterator(const T * ptr, const T * start, const T * end) : mPtr(ptr), mStart(start), mEnd(end)
		{
		}
#endif
	public:
		inline const_iterator() :
		mPtr(0)
		{
		}

		inline const_iterator operator++()
		{
			++mPtr;
			return *this;
		}

		inline const_iterator operator++(int)
		{
			iterator tmp = *this;
			++mPtr;
			return tmp;
		}

		inline const_iterator operator--()
		{
			--mPtr;
			return *this;
		}

		inline const_iterator operator--(int)
		{
			iterator tmp = *this;
			--mPtr;
			return tmp;
		}

		inline const T *  operator->() const
		{
			return mPtr;
		}

		inline const T & operator*() const
		{
			return *mPtr;
		}

		inline bool operator==(const const_iterator &aIt)
		{
			return mPtr == aIt.mPtr;
		}

		inline bool operator!=(const const_iterator &aIt)
		{
			return mPtr != aIt.mPtr;
		}
	private:
		const T * mPtr;
#ifdef DEBUGARRAY
		const T * mStart, * mEnd;
#endif
	};

	/// Iterator functions

	inline iterator begin()
	{
#ifndef DEBUGARRAY
		return iterator(mStart);
#else
		return iterator(mStart,mStart,mStart+mSize);
#endif
	}

	inline iterator end()
	{
#ifndef DEBUGARRAY
		return iterator(mStart + mSize);
#else
		return iterator(mStart + mSize, mStart, mStart + mSize);
#endif
	}

	inline const_iterator begin() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart);
#else
		return const_iterator(mStart, mStart, mStart + mSize);
#endif
	}

	inline const_iterator end() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart + mSize);
#else
		return const_iterator(mStart + mSize, mStart, mStart + mSize);
#endif
	}

	inline const_iterator cbegin() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart);
#else
		return const_iterator(mStart, mStart, mStart + mSize);
#endif
	}

	inline const_iterator cend() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart + mSize);
#else
		return const_iterator(mStart + mSize, mStart, mStart + mSize);
#endif
	}
private:
	T mElements[N];
	T * mStart;
	size_t mSize;
};
