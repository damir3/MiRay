#pragma once

#if defined(__APPLE__)
#include <libkern/OSAtomic.h>
#endif

namespace mr
{

class BaseRC
{
private:
	BaseRC(const BaseRC &) {}
	const BaseRC & operator = (const BaseRC &) { return *this; }

protected:
#if defined(_WIN32)
	volatile LONG m_nRefCount;
#else
	volatile int m_nRefCount;
#endif

	BaseRC() : m_nRefCount(1) {}
	virtual ~BaseRC() {}

public:
	uint32 AddRef()
	{
#if defined(_WIN32)
		return InterlockedIncrement(&m_nRefCount);
#elif defined(__APPLE__)
		return OSAtomicIncrement32Barrier(&m_nRefCount);
#elif defined(__GNUC__)
		return __sync_add_and_fetch(&m_nRefCount, 1);
#else
		return ++m_nRefCount;
#endif
	}

	uint32 Release()
	{
#if defined(_WIN32)
		volatile LONG nRefCount = InterlockedDecrement(&m_nRefCount);
#elif defined(__APPLE__)
		volatile int nRefCount = OSAtomicDecrement32Barrier(&m_nRefCount);
#elif defined(__GNUC__)
		volatile int nRefCount = __sync_sub_and_fetch(&m_nRefCount);
#else
		volatile int nRefCount = --m_nRefCount;
#endif

		if (nRefCount == 0)
			delete this;

		return nRefCount;
	}

	uint32 GetRefCount() const { return m_nRefCount; }
};

}