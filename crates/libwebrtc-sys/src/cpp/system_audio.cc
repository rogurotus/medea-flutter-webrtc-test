// // audioCapture.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
// //

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
#include "desktop.h"


#define RECONNECT_INTERVAL 3000
#define UNUSED_PARAMETER(param) (void)param
#define BUFFER_TIME_100NS (5 * 10000000)
#define MAX_AV_PLANES 8

struct win_version_info
{
	int major;
	int minor;
	int build;
	int revis;
};

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

static inline uint64_t get_clockfreq(void)
{
	static bool have_clockfreq = false;
	static LARGE_INTEGER clock_freq;

	if (!have_clockfreq)
	{
		QueryPerformanceFrequency(&clock_freq);
		have_clockfreq = true;
	}

	return clock_freq.QuadPart;
}

uint64_t os_gettime_ns(void)
{
	LARGE_INTEGER current_time;
	double time_val;

	QueryPerformanceCounter(&current_time);
	time_val = (double)current_time.QuadPart;
	time_val *= 1000000000.0;
	time_val /= (double)get_clockfreq();

	return (uint64_t)time_val;
}

/**
 * Source audio output structure.  Used with obs_source_output_audio to output
 * source audio.  Audio is automatically resampled and remixed as necessary.
 */
struct obs_source_audio
{
	const uint8_t *data[MAX_AV_PLANES];
	uint32_t frames;

	enum speaker_layout speakers;
	enum audio_format format;
	uint32_t samples_per_sec;

	uint64_t timestamp;
};

/**
 * The speaker layout describes where the speakers are located in the room.
 * For OBS it dictates:
 *  *  how many channels are available and
 *  *  which channels are used for which speakers.
 *
 * Standard channel layouts where retrieved from ffmpeg documentation at:
 *     https://trac.ffmpeg.org/wiki/AudioChannelManipulation
 */
enum speaker_layout
{
	SPEAKERS_UNKNOWN,	  /**< Unknown setting, fallback is stereo. */
	SPEAKERS_MONO,		  /**< Channels: MONO */
	SPEAKERS_STEREO,	  /**< Channels: FL, FR */
	SPEAKERS_2POINT1,	  /**< Channels: FL, FR, LFE */
	SPEAKERS_4POINT0,	  /**< Channels: FL, FR, FC, RC */
	SPEAKERS_4POINT1,	  /**< Channels: FL, FR, FC, LFE, RC */
	SPEAKERS_5POINT1,	  /**< Channels: FL, FR, FC, LFE, RL, RR */
	SPEAKERS_7POINT1 = 8, /**< Channels: FL, FR, FC, LFE, RL, RR, SL, SR */
};

enum audio_format
{
	AUDIO_FORMAT_UNKNOWN,

	AUDIO_FORMAT_U8BIT,
	AUDIO_FORMAT_16BIT,
	AUDIO_FORMAT_32BIT,
	AUDIO_FORMAT_FLOAT,

	AUDIO_FORMAT_U8BIT_PLANAR,
	AUDIO_FORMAT_16BIT_PLANAR,
	AUDIO_FORMAT_32BIT_PLANAR,
	AUDIO_FORMAT_FLOAT_PLANAR,
};

size_t wchar_to_utf8(const wchar_t *in, size_t insize, char *out,
					 size_t outsize, int flags)
{
	int i_insize = (int)insize;
	int ret;

	if (i_insize == 0)
		i_insize = (int)wcslen(in);

	ret = WideCharToMultiByte(CP_UTF8, 0, in, i_insize, out, (int)outsize,
							  NULL, NULL);

	UNUSED_PARAMETER(flags);
	return (ret > 0) ? (size_t)ret : 0;
}

size_t os_wcs_to_utf8(const wchar_t *str, size_t len, char *dst,
					  size_t dst_size)
{
	size_t in_len;
	size_t out_len;

	if (!str)
		return 0;

	in_len = (len != 0) ? len : wcslen(str);
	out_len = dst ? (dst_size - 1) : wchar_to_utf8(str, in_len, NULL, 0, 0);

	if (dst)
	{
		if (!dst_size)
			return 0;

		if (out_len)
			out_len =
				wchar_to_utf8(str, in_len, dst, out_len + 1, 0);

		dst[out_len] = 0;
	}

	return out_len;
}

static speaker_layout ConvertSpeakerLayout(DWORD layout, WORD channels)
{
	switch (layout)
	{
	case KSAUDIO_SPEAKER_2POINT1:
		return SPEAKERS_2POINT1;
	case KSAUDIO_SPEAKER_SURROUND:
		return SPEAKERS_4POINT0;
	case KSAUDIO_SPEAKER_5POINT1_SURROUND:
		return SPEAKERS_5POINT1;
	case KSAUDIO_SPEAKER_7POINT1_SURROUND:
		return SPEAKERS_7POINT1;
	}

	return (speaker_layout)channels;
}

std::string GetDeviceName(IMMDevice *device)
{
	std::string device_name;
	ComPtr<IPropertyStore> store;
	HRESULT res;

	if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, store.Assign())))
	{
		PROPVARIANT nameVar;

		PropVariantInit(&nameVar);
		res = store->GetValue(PKEY_Device_FriendlyName, &nameVar);

		if (SUCCEEDED(res) && nameVar.pwszVal && *nameVar.pwszVal)
		{
			size_t len = wcslen(nameVar.pwszVal);
			size_t size;

			size = os_wcs_to_utf8(nameVar.pwszVal, len, nullptr,
								  0) +
				   1;
			device_name.resize(size);
			os_wcs_to_utf8(nameVar.pwszVal, len, &device_name[0],
						   size);
		}
	}

    std::cout << device_name << std::endl;
	return device_name;
}

class Cap;

class WASAPINotify : public IMMNotificationClient
{
	long refs = 0; /* auto-incremented to 1 by ComPtr */
	Cap *source;

public:
	WASAPINotify(Cap *source_) : source(source_) {}

	STDMETHODIMP_(ULONG)
	AddRef()
	{
		return (ULONG)os_atomic_inc_long(&refs);
	}

	STDMETHODIMP_(ULONG)
	STDMETHODCALLTYPE Release()
	{
		long val = os_atomic_dec_long(&refs);
		if (val == 0)
			delete this;
		return (ULONG)val;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ptr)
	{
		if (riid == IID_IUnknown)
		{
			*ptr = (IUnknown *)this;
		}
		else if (riid == __uuidof(IMMNotificationClient))
		{
			*ptr = (IMMNotificationClient *)this;
		}
		else
		{
			*ptr = nullptr;
			return E_NOINTERFACE;
		}

		os_atomic_inc_long(&refs);
		return S_OK;
	}

	STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role,
										LPCWSTR id)
	{
		// source->SetDefaultDevice(flow, role, id);
		return S_OK;
	}

	STDMETHODIMP OnDeviceAdded(LPCWSTR) { return S_OK; }
	STDMETHODIMP OnDeviceRemoved(LPCWSTR) { return S_OK; }
	STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) { return S_OK; }
	STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
	{
		return S_OK;
	}
};

class Cap
{
public:
	std::function<void(void*)> _lambda;
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

	speaker_layout speakers;
	audio_format format;
	uint32_t sampleRate;

	uint64_t framesProcessed = 0;

// 	// static DWORD WINAPI ReconnectThread(LPVOID param);
	static DWORD WINAPI CaptureThread(LPVOID param)
	{
		os_set_thread_name("win-wasapi: capture thread");

		const HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
		const bool com_initialized = SUCCEEDED(hr);
		if (!com_initialized)
		{
			// blog(LOG_ERROR,
			//	"[WASAPISource::CaptureThread]"
			//	" CoInitializeEx failed: 0x%08X",
			//	hr);
		}

		DWORD unused = 0;
		// const HANDLE handle = AvSetMmThreadCharacteristics(L"Audio", &unused);

		Cap *source = (Cap *)param;

		const HANDLE inactive_sigs[] = {
			source->exitSignal,
			source->stopSignal,
			source->initSignal,
		};

		const HANDLE active_sigs[] = {
			source->exitSignal,
			source->stopSignal,
			source->receiveSignal,
			source->restartSignal,
		};

		DWORD sig_count = _countof(inactive_sigs);
		const HANDLE *sigs = inactive_sigs;

		bool exit = false;
		while (!exit)
		{
			bool idle = false;
			bool stop = false;
			bool reconnect = false;
			do
			{
				/* Windows 7 does not seem to wake up for LOOPBACK */
				const DWORD dwMilliseconds =
					((sigs == active_sigs) &&
					 (true)) // todo TRUE?
						? 10
						: INFINITE;

				const DWORD ret = WaitForMultipleObjects(
					sig_count, sigs, false, dwMilliseconds);
				switch (ret)
				{
				case WAIT_OBJECT_0:
				{
					exit = true;
					stop = true;
					idle = true;
					break;
				}

				case WAIT_OBJECT_0 + 1:
					stop = true;
					idle = true;
					break;

				case WAIT_OBJECT_0 + 2:
				case WAIT_TIMEOUT:
					if (sigs == inactive_sigs)
					{
						assert(ret != WAIT_TIMEOUT);

						if (source->TryInitialize())
						{
							sig_count =
								_countof(active_sigs);
							sigs = active_sigs;
						}
						else
						{
							if (source->reconnectDuration ==
								0)
							{
								// blog(LOG_INFO,
								//	"WASAPI: Device '%s' failed to start (source: %s)",
								//	source->device_id
								//	.c_str(),
								//	obs_source_get_name(
								//		source->source));
							}
							stop = true;
							reconnect = true;
							source->reconnectDuration =
								RECONNECT_INTERVAL;
						}
					}
					else
					{
						stop = !source->ProcessCaptureData();
						if (stop)
						{
							// blog(LOG_INFO,
							//	"Device '%s' invalidated.  Retrying (source: %s)",
							//	source->device_name.c_str(),
							//	obs_source_get_name(
							//		source->source));
							stop = true;
							reconnect = true;
							source->reconnectDuration =
								RECONNECT_INTERVAL;
						}
					}
					break;

				default:
					assert(sigs == active_sigs);
					assert(ret == WAIT_OBJECT_0 + 3);
					stop = true;
					reconnect = true;
					source->reconnectDuration = 0;
					ResetEvent(source->restartSignal);
				}
			} while (!stop);

			sig_count = _countof(inactive_sigs);
			sigs = inactive_sigs;

			if (source->client)
			{
				source->client->Stop();

				source->capture.Clear();
				source->client.Clear();
			}

			if (idle)
			{
				SetEvent(source->idleSignal);
			}
			else if (reconnect)
			{
				// blog(LOG_INFO,
				//	"Device '%s' invalidated.  Retrying (source: %s)",
				//	source->device_name.c_str(),
				//	obs_source_get_name(source->source));
				SetEvent(source->reconnectSignal);
			}
		}

		// if (handle)
		//	AvRevertMmThreadCharacteristics(handle);

		if (com_initialized)
			CoUninitialize();

		return 0;
	}

	bool ProcessCaptureData()
	{
		HRESULT res;
		LPBYTE buffer;
		UINT32 frames;
		DWORD flags;
		UINT64 pos, ts;
		UINT captureSize = 0;

		while (true)
		{

			res = capture->GetNextPacketSize(&captureSize);
			if (FAILED(res))
			{
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					// blog(LOG_WARNING,
					//	"[WASAPISource::ProcessCaptureData]"
					//	" capture->GetNextPacketSize"
					//	" failed: %lX",
					//	res);
					return false;
			}

			if (!captureSize)
				break;

			res = capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
			if (FAILED(res))
			{
				if (res != AUDCLNT_E_DEVICE_INVALIDATED)
					// blog(LOG_WARNING,
					//	"[WASAPISource::ProcessCaptureData]"
					//	" capture->GetBuffer"
					//	" failed: %lX",
					//	res);
					return false;
			}

			// std::cout << "AAAAAA " << sampleRate << std::endl;
			obs_source_audio data = {};
			data.data[0] = (const uint8_t *)buffer;
			data.frames = (uint32_t)frames;
			data.speakers = speakers;
			data.samples_per_sec = sampleRate;
			data.format = format;
			data.timestamp = useDeviceTiming ? ts * 100
											 : os_gettime_ns();

			if (!useDeviceTiming)
				data.timestamp -= util_mul_div64(
					frames, UINT64_C(1000000000),
					sampleRate);

            // for (int i = 0; i < 8; ++i) {
            //     std::cout << (int)data.data[i] << " ";
            // } 
            // std::cout << std::endl;

            // HERE
			// std::cout << "AAAAAA2" << std::endl;
			_lambda(&data);
			// std::cout << "AAAAAA3" << std::endl;

			// here data callback
			// obs_source_output_audio(source, &data);

			capture->ReleaseBuffer(frames);
		}

		return true;
	}

	void Start()
	{
		SetEvent(initSignal);
	}

	void Stop()
	{
		SetEvent(stopSignal);

		if (reconnectThread.Valid())
		{
			WaitForSingleObject(idleSignal, INFINITE);
		}
		else
		{
			const HANDLE sigs[] = {reconnectSignal, idleSignal};
			WaitForMultipleObjects(_countof(sigs), sigs, false, INFINITE);
		}

		SetEvent(exitSignal);

		if (reconnectThread.Valid())
		{
			SetEvent(reconnectExitSignal);
			WaitForSingleObject(reconnectThread, INFINITE);
		}

		WaitForSingleObject(captureThread, INFINITE);
	}

	// init default output device
	static ComPtr<IMMDevice> InitDevice(IMMDeviceEnumerator *enumerator)
	{
		ComPtr<IMMDevice> device;

		const bool input = false;
		HRESULT res = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.Assign());
		if (FAILED(res))
			throw HRError("Failed GetDefaultAudioEndpoint", res);

		return device;
	}

	static ComPtr<IAudioClient> InitClient(
		IMMDevice *device,
		speaker_layout &speakers, audio_format &format,
		uint32_t &samples_per_sec)
	{
		WAVEFORMATEXTENSIBLE wfextensible;
		CoTaskMemPtr<WAVEFORMATEX> wfex;
		const WAVEFORMATEX *pFormat;
		HRESULT res;
		ComPtr<IAudioClient> client;

		res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
							   nullptr, (void **)client.Assign());
		if (FAILED(res))
			throw HRError("Failed to activate client context", res);

		res = client->GetMixFormat(&wfex);
		if (FAILED(res))
			throw HRError("Failed to get mix format", res);

		pFormat = wfex.Get();

		Cap::InitFormat(pFormat, speakers, format, samples_per_sec);

		DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

		// (type != SourceType::Input)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

		res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags,
								 BUFFER_TIME_100NS, 0, pFormat, nullptr);
		if (FAILED(res))
			throw HRError("Failed to initialize audio client", res);

		return client;
	}

	// set audio format?
	static void InitFormat(const WAVEFORMATEX *wfex,
						   enum speaker_layout &speakers,
						   enum audio_format &format, uint32_t &sampleRate)
	{
		DWORD layout = 0;

		if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE *)wfex;
			layout = ext->dwChannelMask;
		}

		/* WASAPI is always float */
		speakers = ConvertSpeakerLayout(layout, wfex->nChannels);
		format = AUDIO_FORMAT_FLOAT;
		sampleRate = wfex->nSamplesPerSec;
	}

	// render buffer magic
	static void ClearBuffer(IMMDevice *device)
	{
		CoTaskMemPtr<WAVEFORMATEX> wfex;
		HRESULT res;
		LPBYTE buffer;
		UINT32 frames;
		ComPtr<IAudioClient> client;

		res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
							   (void **)client.Assign());
		if (FAILED(res))
			throw HRError("Failed to activate client context", res);

		res = client->GetMixFormat(&wfex);
		if (FAILED(res))
			throw HRError("Failed to get mix format", res);

		res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, BUFFER_TIME_100NS,
								 0, wfex, nullptr);
		if (FAILED(res))
			throw HRError("Failed to initialize audio client", res);

		/* Silent loopback fix. Prevents audio stream from stopping and */
		/* messing up timestamps and other weird glitches during silence */
		/* by playing a silent sample all over again. */

		res = client->GetBufferSize(&frames);
		if (FAILED(res))
			throw HRError("Failed to get buffer size", res);

		ComPtr<IAudioRenderClient> render;
		res = client->GetService(IID_PPV_ARGS(render.Assign()));
		if (FAILED(res))
			throw HRError("Failed to get render client", res);

		res = render->GetBuffer(frames, &buffer);
		if (FAILED(res))
			throw HRError("Failed to get buffer", res);

		memset(buffer, 0, (size_t)frames * (size_t)wfex->nBlockAlign);

		render->ReleaseBuffer(frames, 0);
	}

	static ComPtr<IAudioCaptureClient> InitCapture(IAudioClient *client,
												   HANDLE receiveSignal)
	{
		ComPtr<IAudioCaptureClient> capture;
		HRESULT res = client->GetService(IID_PPV_ARGS(capture.Assign()));
		if (FAILED(res))
			throw HRError("Failed to create capture context", res);

		res = client->SetEventHandle(receiveSignal);
		if (FAILED(res))
			throw HRError("Failed to set event handle", res);

		res = client->Start();
		if (FAILED(res))
			throw HRError("Failed to start capture client", res);

		return capture;
	}

	void Initialize()
	{
		ComPtr<IMMDevice> device;

		// get default output device
		device = InitDevice(enumerator);

		device_name = GetDeviceName(device);
		// std::cout << "NAME - " << device_name << std::endl;

		ResetEvent(receiveSignal);

		ComPtr<IAudioClient> temp_client = InitClient(
			device, speakers, format, sampleRate);

		//spec magic (sourceType == SourceType::DeviceOutput)
		ClearBuffer(device);

		ComPtr<IAudioCaptureClient> temp_capture =
			InitCapture(temp_client, receiveSignal);

		client = std::move(temp_client);
		capture = std::move(temp_capture);

		// blog(LOG_INFO, "WASAPI: Device '%s' [%" PRIu32 " Hz] initialized",
		//	device_name.c_str(), sampleRate);
	}

	bool TryInitialize()
	{
		bool success = false;
		try
		{
			Initialize();
			success = true;
		}
		catch (HRError &error)
		{
			if (!previouslyFailed)
			{
				// blog(LOG_WARNING,
				//	"[WASAPISource::TryInitialize]:[%s] %s: %lX",
				//	device_name.empty() ? device_id.c_str()
				//	: device_name.c_str(),
				//	error.str, error.hr);
			}
		}
		catch (const char *error)
		{
			if (!previouslyFailed)
			{
				// blog(LOG_WARNING,
				//	"[WASAPISource::TryInitialize]:[%s] %s",
				//	device_name.empty() ? device_id.c_str()
				//	: device_name.c_str(),
				//	error);
			}
		}

		previouslyFailed = !success;
		return success;
	}

	Cap(std::function<void(void*) > lambda) : _lambda(lambda)
	{
		// mmdevapi_module = LoadLibrary(L"Mmdevapi");
		// if (mmdevapi_module)
		// {
		// 	activate_audio_interface_async =
		// 		(PFN_ActivateAudioInterfaceAsync)GetProcAddress(
		// 			mmdevapi_module, "ActivateAudioInterfaceAsync");
		// }

		// // UpdateSettings(BuildUpdateParams(settings));
		// // LogSettings();

		idleSignal = CreateEvent(nullptr, true, false, nullptr);
		if (!idleSignal.Valid())
			throw "Could not create idle signal";

		stopSignal = CreateEvent(nullptr, true, false, nullptr);
		if (!stopSignal.Valid())
			throw "Could not create stop signal";

		receiveSignal = CreateEvent(nullptr, false, false, nullptr);
		if (!receiveSignal.Valid())
			throw "Could not create receive signal";

		restartSignal = CreateEvent(nullptr, true, false, nullptr);
		if (!restartSignal.Valid())
			throw "Could not create restart signal";

		reconnectExitSignal = CreateEvent(nullptr, true, false, nullptr);
		if (!reconnectExitSignal.Valid())
			throw "Could not create reconnect exit signal";

		exitSignal = CreateEvent(nullptr, true, false, nullptr);
		if (!exitSignal.Valid())
			throw "Could not create exit signal";

		initSignal = CreateEvent(nullptr, false, false, nullptr);
		if (!initSignal.Valid())
			throw "Could not create init signal";

		reconnectSignal = CreateEvent(nullptr, false, false, nullptr);
		if (!reconnectSignal.Valid())
			throw "Could not create reconnect signal";

		notify = new WASAPINotify(this);
		if (!notify)
			throw "Could not create WASAPINotify";

		CoInitialize(0); // todo??

		HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
									  CLSCTX_ALL,
									  IID_PPV_ARGS(enumerator.Assign()));
		if (FAILED(hr))
			throw HRError("Failed to create enumerator", hr);

		hr = enumerator->RegisterEndpointNotificationCallback(notify);
		if (FAILED(hr))
			throw HRError("Failed to register endpoint callback", hr);

		captureThread = CreateThread(nullptr, 0,
									 Cap::CaptureThread, this,
									 0, nullptr);
		if (!captureThread.Valid())
		{
			enumerator->UnregisterEndpointNotificationCallback(
				notify);
			throw "Failed to create capture thread";
		}

		Start();
		std::cout <<"NAME" << device_name << std::endl;
	}

	~Cap()
	{

		enumerator->UnregisterEndpointNotificationCallback(notify);
		Stop();
	}
};


DesktopAudioSource::DesktopAudioSource() {
	std::cout <<"RA2Z" << std::endl;


	auto lambda = [&] (void* x) {
		obs_source_audio* a = (obs_source_audio*)x;
        UpdateFrame((const int16_t *)(a->data), 480, 48000, 1);
	};
	Cap* cap = new Cap(lambda);

}


