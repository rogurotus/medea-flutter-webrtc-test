/*
 * Copyright (c) 2014 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once
#include <windows.h>
#include <iostream>
#include <minwindef.h>
#include "system_audio.h"
#include <handleapi.h>
#include <avrt.h>
#include <combaseapi.h>
#include <assert.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <Audioclient.h>
#include <RTWorkQ.h>
#include <functional>

struct HRError {
	const char* str;
	HRESULT hr;

	inline HRError(const char* str, HRESULT hr) : str(str), hr(hr) {}
};


template<typename T> class CoTaskMemPtr {
	T* ptr;

	inline void Clear()
	{
		if (ptr)
			CoTaskMemFree(ptr);
	}

public:
	inline CoTaskMemPtr() : ptr(NULL) {}
	inline CoTaskMemPtr(T* ptr_) : ptr(ptr_) {}
	inline ~CoTaskMemPtr() { Clear(); }

	inline operator T* () const { return ptr; }
	inline T* operator->() const { return ptr; }

	inline const T* Get() const { return ptr; }

	inline CoTaskMemPtr& operator=(T* val)
	{
		Clear();
		ptr = val;
		return *this;
	}

	inline T** operator&()
	{
		Clear();
		ptr = NULL;
		return &ptr;
	}
};

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif



class WinHandle {
	HANDLE handle = INVALID_HANDLE_VALUE;

	inline void Clear()
	{
		if (handle && handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
	}

public:
	inline WinHandle() {}
	inline WinHandle(HANDLE handle_) : handle(handle_) {}
	inline ~WinHandle() { Clear(); }

	inline operator HANDLE() const { return handle; }

	inline WinHandle& operator=(HANDLE handle_)
	{
		if (handle_ != handle) {
			Clear();
			handle = handle_;
		}

		return *this;
	}

	inline HANDLE* operator&() { return &handle; }

	inline bool Valid() const
	{
		return handle && handle != INVALID_HANDLE_VALUE;
	}
};

class WinModule {
	HMODULE handle = NULL;

	inline void Clear()
	{
		if (handle)
			FreeLibrary(handle);
	}

public:
	inline WinModule() {}
	inline WinModule(HMODULE handle_) : handle(handle_) {}
	inline ~WinModule() { Clear(); }

	inline operator HMODULE() const { return handle; }

	inline WinModule& operator=(HMODULE handle_)
	{
		if (handle_ != handle) {
			Clear();
			handle = handle_;
		}

		return *this;
	}

	inline HMODULE* operator&() { return &handle; }

	inline bool Valid() const { return handle != NULL; }
};

#ifdef _WIN32
#include <Unknwn.h>
#endif

/* Oh no I have my own com pointer class, the world is ending, how dare you
 * write your own! */

template<class T> class ComPtr {

protected:
	T *ptr;

	inline void Kill()
	{
		if (ptr)
			ptr->Release();
	}

	inline void Replace(T *p)
	{
		if (ptr != p) {
			if (p)
				p->AddRef();
			if (ptr)
				ptr->Release();
			ptr = p;
		}
	}

public:
	inline ComPtr() : ptr(nullptr) {}
	inline ComPtr(T *p) : ptr(p)
	{
		if (ptr)
			ptr->AddRef();
	}
	inline ComPtr(const ComPtr<T> &c) : ptr(c.ptr)
	{
		if (ptr)
			ptr->AddRef();
	}
	inline ComPtr(ComPtr<T> &&c) noexcept : ptr(c.ptr) { c.ptr = nullptr; }
	template<class U>
	inline ComPtr(ComPtr<U> &&c) noexcept : ptr(c.Detach())
	{
	}
	inline ~ComPtr() { Kill(); }

	inline void Clear()
	{
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}

	inline ComPtr<T> &operator=(T *p)
	{
		Replace(p);
		return *this;
	}

	inline ComPtr<T> &operator=(const ComPtr<T> &c)
	{
		Replace(c.ptr);
		return *this;
	}

	inline ComPtr<T> &operator=(ComPtr<T> &&c) noexcept
	{
		if (&ptr != &c.ptr) {
			Kill();
			ptr = c.ptr;
			c.ptr = nullptr;
		}

		return *this;
	}

	template<class U> inline ComPtr<T> &operator=(ComPtr<U> &&c) noexcept
	{
		Kill();
		ptr = c.Detach();

		return *this;
	}

	inline T *Detach()
	{
		T *out = ptr;
		ptr = nullptr;
		return out;
	}

	inline void CopyTo(T **out)
	{
		if (out) {
			if (ptr)
				ptr->AddRef();
			*out = ptr;
		}
	}

	inline ULONG Release()
	{
		ULONG ref;

		if (!ptr)
			return 0;
		ref = ptr->Release();
		ptr = nullptr;
		return ref;
	}

	inline T **Assign()
	{
		Clear();
		return &ptr;
	}
	inline void Set(T *p)
	{
		Kill();
		ptr = p;
	}

	inline T *Get() const { return ptr; }

	inline T **operator&() { return Assign(); }

	inline operator T *() const { return ptr; }
	inline T *operator->() const { return ptr; }

	inline bool operator==(T *p) const { return ptr == p; }
	inline bool operator!=(T *p) const { return ptr != p; }

	inline bool operator!() const { return !ptr; }
};

#ifdef _WIN32

template<class T> class ComQIPtr : public ComPtr<T> {

public:
	inline ComQIPtr(IUnknown *unk)
	{
		this->ptr = nullptr;
		unk->QueryInterface(__uuidof(T), (void **)&this->ptr);
	}

	inline ComPtr<T> &operator=(IUnknown *unk)
	{
		ComPtr<T>::Clear();
		unk->QueryInterface(__uuidof(T), (void **)&this->ptr);
		return *this;
	}
};

#endif




#define RECONNECT_INTERVAL 3000
#define UNUSED_PARAMETER(param) (void)param
#define BUFFER_TIME_100NS (5 * 10000000)
#define MAX_AV_PLANES 8


typedef HRESULT(STDAPICALLTYPE *PFN_ActivateAudioInterfaceAsync)(
	LPCWSTR, REFIID, PROPVARIANT *,
	IActivateAudioInterfaceCompletionHandler *,
	IActivateAudioInterfaceAsyncOperation **);

typedef HRESULT(STDAPICALLTYPE *PFN_RtwqUnlockWorkQueue)(DWORD);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqLockSharedWorkQueue)(PCWSTR usageClass,
															 LONG basePriority,
															 DWORD *taskId,
															 DWORD *id);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqCreateAsyncResult)(IUnknown *,
														   IRtwqAsyncCallback *,
														   IUnknown *,
														   IRtwqAsyncResult **);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqPutWorkItem)(DWORD, LONG,
													 IRtwqAsyncResult *);
typedef HRESULT(STDAPICALLTYPE *PFN_RtwqPutWaitingWorkItem)(HANDLE, LONG,
															IRtwqAsyncResult *,
															RTWQWORKITEM_KEY *);


size_t os_wcs_to_utf8(const wchar_t *str, size_t len, char *dst,
					  size_t dst_size);
size_t wchar_to_utf8(const wchar_t *in, size_t insize, char *out,
					 size_t outsize, int flags);
uint64_t os_gettime_ns(void);
static inline uint64_t get_clockfreq(void);


#include "system_audio_module.h"
class SystemModule : public SystemModuleInterface {
    public:

    SystemModule();
    ~SystemModule();


    // Initialization and terminate.
    int32_t Init();
    int32_t Terminate();

    // System control.
    // int32_t SetSystemMute(bool enable);
    // int32_t SystemMute(bool* enabled) const;
    // bool SystemIsInitialized ();
    // int32_t SetSystemVolume(uint32_t volume);
    // int32_t SystemVolume(uint32_t* volume) const;

    // Settings.
    int32_t SetRecordingDevice(uint16_t index);

    // enumerate outputs
    int32_t EnumerateDevice(uint16_t index);

    // System source.
    rtc::scoped_refptr<AudioSource> CreateSource();
    void ResetSource();
    int32_t StopRecording();
    int32_t StartRecording();
    int32_t RecordingChannels();


    void Initialize();
	static void ClearBuffer(IMMDevice *device);
    static void InitFormat(const WAVEFORMATEX *wfex,
                        int& speakers,
                        int& format, uint32_t &sampleRate);
	static ComPtr<IAudioClient> InitClient(
		IMMDevice *device,
		int& channels, int& format,
		uint32_t &samples_per_sec);
    static ComPtr<IMMDevice> InitDevice(IMMDeviceEnumerator *enumerator);
    bool ProcessCaptureData();
    static DWORD WINAPI CaptureThread(LPVOID param);
    std::string GetDeviceName(IMMDevice *device);
    static int ConvertSpeakerLayout(DWORD layout, WORD channels);
	static ComPtr<IAudioCaptureClient> InitCapture(IAudioClient *client,
												   HANDLE receiveSignal);

	void ConvertBuffer(std::vector<int8_t>& data);

	ComPtr<IMMNotificationClient> notify;
	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<IAudioClient> client;
	ComPtr<IAudioCaptureClient> capture;

	std::wstring default_id;
	std::string device_id;
	std::string device_name;
	WinModule mmdevapi_module;
	PFN_ActivateAudioInterfaceAsync activate_audio_interface_async = NULL;
	PFN_RtwqUnlockWorkQueue rtwq_unlock_work_queue = NULL;
	PFN_RtwqLockSharedWorkQueue rtwq_lock_shared_work_queue = NULL;
	PFN_RtwqCreateAsyncResult rtwq_create_async_result = NULL;
	PFN_RtwqPutWorkItem rtwq_put_work_item = NULL;
	PFN_RtwqPutWaitingWorkItem rtwq_put_waiting_work_item = NULL;
	bool rtwq_supported = false;
	std::string window_class;
	std::string title;
	std::string executable;
	HWND hwnd = NULL;
	DWORD process_id = 0;
	bool useDeviceTiming = false;
	bool isDefaultDevice = false;

	bool previouslyFailed = false;
	WinHandle reconnectThread = NULL;

	WinHandle captureThread;
	WinHandle idleSignal;
	WinHandle stopSignal;
	WinHandle receiveSignal;
	WinHandle restartSignal;
	WinHandle reconnectExitSignal;
	WinHandle exitSignal;
	WinHandle initSignal;
	DWORD reconnectDuration = 0;
	WinHandle reconnectSignal;

	int channels;
	int format; // byte size of frame.
	uint32_t sampleRate;

	uint64_t framesProcessed = 0;
	bool stop = false;

	std::vector<int8_t> capture_buffer;
	std::vector<int8_t> over_capture_buffer;
	std::vector<int16_t> release_capture_buffer = std::vector<int16_t>(480 * 2);
};