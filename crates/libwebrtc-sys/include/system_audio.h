// /*
//  * Copyright (c) 2014 Hugh Bailey <obs.jim@gmail.com>
//  *
//  * Permission to use, copy, modify, and distribute this software for any
//  * purpose with or without fee is hereby granted, provided that the above
//  * copyright notice and this permission notice appear in all copies.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//  * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//  * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//  * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//  * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//  * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//  * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//  */

// #pragma once


// struct HRError {
// 	const char* str;
// 	HRESULT hr;

// 	inline HRError(const char* str, HRESULT hr) : str(str), hr(hr) {}
// };


// template<typename T> class CoTaskMemPtr {
// 	T* ptr;

// 	inline void Clear()
// 	{
// 		if (ptr)
// 			CoTaskMemFree(ptr);
// 	}

// public:
// 	inline CoTaskMemPtr() : ptr(NULL) {}
// 	inline CoTaskMemPtr(T* ptr_) : ptr(ptr_) {}
// 	inline ~CoTaskMemPtr() { Clear(); }

// 	inline operator T* () const { return ptr; }
// 	inline T* operator->() const { return ptr; }

// 	inline const T* Get() const { return ptr; }

// 	inline CoTaskMemPtr& operator=(T* val)
// 	{
// 		Clear();
// 		ptr = val;
// 		return *this;
// 	}

// 	inline T** operator&()
// 	{
// 		Clear();
// 		ptr = NULL;
// 		return &ptr;
// 	}
// };

// #if defined(_MSC_VER) && defined(_M_X64)
// #include <intrin.h>
// #endif

// static inline uint64_t util_mul_div64(uint64_t num, uint64_t mul, uint64_t div)
// {
// #if defined(_MSC_VER) && defined(_M_X64)
// 	unsigned __int64 high;
// 	const unsigned __int64 low = _umul128(num, mul, &high);
// 	unsigned __int64 rem;
// 	return _udiv128(high, low, div, &rem);
// #else
// 	const uint64_t rem = num % div;
// 	return (num / div) * mul + (rem * mul) / div;
// #endif
// }

// static inline long os_atomic_dec_long(volatile long* val)
// {
// 	return _InterlockedDecrement(val);
// }

// static inline long os_atomic_inc_long(volatile long* val)
// {
// 	return _InterlockedIncrement(val);
// }


// void os_set_thread_name(const char* name)
// {
// #if defined(__APPLE__)
// 	pthread_setname_np(name);
// #elif defined(__FreeBSD__)
// 	pthread_set_name_np(pthread_self(), name);
// #elif defined(__GLIBC__) && !defined(__MINGW32__)
// 	if (strlen(name) <= 15) {
// 		pthread_setname_np(pthread_self(), name);
// 	}
// 	else {
// 		char* thread_name = bstrdup_n(name, 15);
// 		pthread_setname_np(pthread_self(), thread_name);
// 		bfree(thread_name);
// 	}
// #endif
// }


// class WinHandle {
// 	HANDLE handle = INVALID_HANDLE_VALUE;

// 	inline void Clear()
// 	{
// 		if (handle && handle != INVALID_HANDLE_VALUE)
// 			CloseHandle(handle);
// 	}

// public:
// 	inline WinHandle() {}
// 	inline WinHandle(HANDLE handle_) : handle(handle_) {}
// 	inline ~WinHandle() { Clear(); }

// 	inline operator HANDLE() const { return handle; }

// 	inline WinHandle& operator=(HANDLE handle_)
// 	{
// 		if (handle_ != handle) {
// 			Clear();
// 			handle = handle_;
// 		}

// 		return *this;
// 	}

// 	inline HANDLE* operator&() { return &handle; }

// 	inline bool Valid() const
// 	{
// 		return handle && handle != INVALID_HANDLE_VALUE;
// 	}
// };

// class WinModule {
// 	HMODULE handle = NULL;

// 	inline void Clear()
// 	{
// 		if (handle)
// 			FreeLibrary(handle);
// 	}

// public:
// 	inline WinModule() {}
// 	inline WinModule(HMODULE handle_) : handle(handle_) {}
// 	inline ~WinModule() { Clear(); }

// 	inline operator HMODULE() const { return handle; }

// 	inline WinModule& operator=(HMODULE handle_)
// 	{
// 		if (handle_ != handle) {
// 			Clear();
// 			handle = handle_;
// 		}

// 		return *this;
// 	}

// 	inline HMODULE* operator&() { return &handle; }

// 	inline bool Valid() const { return handle != NULL; }
// };

// #ifdef _WIN32
// #include <Unknwn.h>
// #endif

// /* Oh no I have my own com pointer class, the world is ending, how dare you
//  * write your own! */

// template<class T> class ComPtr {

// protected:
// 	T *ptr;

// 	inline void Kill()
// 	{
// 		if (ptr)
// 			ptr->Release();
// 	}

// 	inline void Replace(T *p)
// 	{
// 		if (ptr != p) {
// 			if (p)
// 				p->AddRef();
// 			if (ptr)
// 				ptr->Release();
// 			ptr = p;
// 		}
// 	}

// public:
// 	inline ComPtr() : ptr(nullptr) {}
// 	inline ComPtr(T *p) : ptr(p)
// 	{
// 		if (ptr)
// 			ptr->AddRef();
// 	}
// 	inline ComPtr(const ComPtr<T> &c) : ptr(c.ptr)
// 	{
// 		if (ptr)
// 			ptr->AddRef();
// 	}
// 	inline ComPtr(ComPtr<T> &&c) noexcept : ptr(c.ptr) { c.ptr = nullptr; }
// 	template<class U>
// 	inline ComPtr(ComPtr<U> &&c) noexcept : ptr(c.Detach())
// 	{
// 	}
// 	inline ~ComPtr() { Kill(); }

// 	inline void Clear()
// 	{
// 		if (ptr) {
// 			ptr->Release();
// 			ptr = nullptr;
// 		}
// 	}

// 	inline ComPtr<T> &operator=(T *p)
// 	{
// 		Replace(p);
// 		return *this;
// 	}

// 	inline ComPtr<T> &operator=(const ComPtr<T> &c)
// 	{
// 		Replace(c.ptr);
// 		return *this;
// 	}

// 	inline ComPtr<T> &operator=(ComPtr<T> &&c) noexcept
// 	{
// 		if (&ptr != &c.ptr) {
// 			Kill();
// 			ptr = c.ptr;
// 			c.ptr = nullptr;
// 		}

// 		return *this;
// 	}

// 	template<class U> inline ComPtr<T> &operator=(ComPtr<U> &&c) noexcept
// 	{
// 		Kill();
// 		ptr = c.Detach();

// 		return *this;
// 	}

// 	inline T *Detach()
// 	{
// 		T *out = ptr;
// 		ptr = nullptr;
// 		return out;
// 	}

// 	inline void CopyTo(T **out)
// 	{
// 		if (out) {
// 			if (ptr)
// 				ptr->AddRef();
// 			*out = ptr;
// 		}
// 	}

// 	inline ULONG Release()
// 	{
// 		ULONG ref;

// 		if (!ptr)
// 			return 0;
// 		ref = ptr->Release();
// 		ptr = nullptr;
// 		return ref;
// 	}

// 	inline T **Assign()
// 	{
// 		Clear();
// 		return &ptr;
// 	}
// 	inline void Set(T *p)
// 	{
// 		Kill();
// 		ptr = p;
// 	}

// 	inline T *Get() const { return ptr; }

// 	inline T **operator&() { return Assign(); }

// 	inline operator T *() const { return ptr; }
// 	inline T *operator->() const { return ptr; }

// 	inline bool operator==(T *p) const { return ptr == p; }
// 	inline bool operator!=(T *p) const { return ptr != p; }

// 	inline bool operator!() const { return !ptr; }
// };

// #ifdef _WIN32

// template<class T> class ComQIPtr : public ComPtr<T> {

// public:
// 	inline ComQIPtr(IUnknown *unk)
// 	{
// 		this->ptr = nullptr;
// 		unk->QueryInterface(__uuidof(T), (void **)&this->ptr);
// 	}

// 	inline ComPtr<T> &operator=(IUnknown *unk)
// 	{
// 		ComPtr<T>::Clear();
// 		unk->QueryInterface(__uuidof(T), (void **)&this->ptr);
// 		return *this;
// 	}
// };

// #endif
