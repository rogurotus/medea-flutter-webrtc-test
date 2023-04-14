
#include "windows_system_audio_module.h"


static inline uint64_t util_mul_div64(uint64_t num, uint64_t mul, uint64_t div)
{
#if defined(_MSC_VER) && defined(_M_X64)
	unsigned __int64 high;
	const unsigned __int64 low = _umul128(num, mul, &high);
	unsigned __int64 rem;
	return _udiv128(high, low, div, &rem);
#else
	const uint64_t rem = num % div;
	return (num / div) * mul + (rem * mul) / div;
#endif
}

static inline long os_atomic_dec_long(volatile long* val)
{
	return _InterlockedDecrement(val);
}

static inline long os_atomic_inc_long(volatile long* val)
{
	return _InterlockedIncrement(val);
}


void os_set_thread_name(const char* name)
{
#if defined(__APPLE__)
	pthread_setname_np(name);
#elif defined(__FreeBSD__)
	pthread_set_name_np(pthread_self(), name);
#elif defined(__GLIBC__) && !defined(__MINGW32__)
	if (strlen(name) <= 15) {
		pthread_setname_np(pthread_self(), name);
	}
	else {
		char* thread_name = bstrdup_n(name, 15);
		pthread_setname_np(pthread_self(), thread_name);
		bfree(thread_name);
	}
#endif
}


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



class WASAPINotify : public IMMNotificationClient {
  long refs = 0; /* auto-incremented to 1 by ComPtr */
  SystemModule* source;

 public:
  WASAPINotify(SystemModule* source_) : source(source_) {}

  STDMETHODIMP_(ULONG)
  AddRef() { return (ULONG)os_atomic_inc_long(&refs); }

  STDMETHODIMP_(ULONG)
  STDMETHODCALLTYPE Release() {
    long val = os_atomic_dec_long(&refs);
    if (val == 0)
      delete this;
    return (ULONG)val;
  }

  STDMETHODIMP QueryInterface(REFIID riid, void** ptr) {
    if (riid == IID_IUnknown) {
      *ptr = (IUnknown*)this;
    } else if (riid == __uuidof(IMMNotificationClient)) {
      *ptr = (IMMNotificationClient*)this;
    } else {
      *ptr = nullptr;
      return E_NOINTERFACE;
    }

    os_atomic_inc_long(&refs);
    return S_OK;
  }

  STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR id) {
    // source->SetDefaultDevice(flow, role, id);
    return S_OK;
  }

  STDMETHODIMP OnDeviceAdded(LPCWSTR) { return S_OK; }
  STDMETHODIMP OnDeviceRemoved(LPCWSTR) { return S_OK; }
  STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) { return S_OK; }
  STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) {
    return S_OK;
  }
};

SystemModule::SystemModule() {
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

  CoInitialize(0);  // todo??

  HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, IID_PPV_ARGS(enumerator.Assign()));
  if (FAILED(hr))
    throw HRError("Failed to create enumerator", hr);

  hr = enumerator->RegisterEndpointNotificationCallback(notify);
  if (FAILED(hr))
    throw HRError("Failed to register endpoint callback", hr);

  captureThread =
      CreateThread(nullptr, 0, SystemModule::CaptureThread, this, 0, nullptr);
  if (!captureThread.Valid()) {
    enumerator->UnregisterEndpointNotificationCallback(notify);
    throw "Failed to create capture thread";
  }

  StartRecording();
}

int32_t SystemModule::Terminate() {
  return 0;
}

int32_t SystemModule::StartRecording()
{
      SetEvent(initSignal);
      return 0;
}

SystemModule::~SystemModule()
{
  enumerator->UnregisterEndpointNotificationCallback(notify);
  StopRecording();
}

	// init default output device
ComPtr<IMMDevice> SystemModule::InitDevice(IMMDeviceEnumerator *enumerator)
	{
		ComPtr<IMMDevice> device;

		const bool input = false;
		HRESULT res = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.Assign());
		if (FAILED(res))
			throw HRError("Failed GetDefaultAudioEndpoint", res);

		return device;
	}

ComPtr<IAudioClient> SystemModule::InitClient(
		IMMDevice *device,
		int& channels, int& format,
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

		SystemModule::InitFormat(pFormat, channels, format, samples_per_sec);

		DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

		// (type != SourceType::Input)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

		res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags,
								 BUFFER_TIME_100NS, 0, pFormat, nullptr);
		if (FAILED(res))
			throw HRError("Failed to initialize audio client", res);

		return client;
	}

DWORD WINAPI SystemModule::CaptureThread(LPVOID param) {
  os_set_thread_name("win-wasapi: capture thread");

  const HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  const bool com_initialized = SUCCEEDED(hr);
  if (!com_initialized) {
    // blog(LOG_ERROR,
    //	"[WASAPISource::CaptureThread]"
    //	" CoInitializeEx failed: 0x%08X",
    //	hr);
  }

  DWORD unused = 0;
  // const HANDLE handle = AvSetMmThreadCharacteristics(L"Audio", &unused);

  SystemModule* source = (SystemModule*)param;

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
  const HANDLE* sigs = inactive_sigs;

  bool exit = false;
  while (!exit) {
    bool idle = false;
    bool stop = false;
    bool reconnect = false;
    do {
      /* Windows 7 does not seem to wake up for LOOPBACK */
      const DWORD dwMilliseconds =
          ((sigs == active_sigs) && (true))  // todo TRUE?
              ? 10
              : INFINITE;

      const DWORD ret =
          WaitForMultipleObjects(sig_count, sigs, false, dwMilliseconds);
      switch (ret) {
        case WAIT_OBJECT_0: {
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
          if (sigs == inactive_sigs) {
            assert(ret != WAIT_TIMEOUT);
            if (source->Init()) {
              sig_count = _countof(active_sigs);
              sigs = active_sigs;
            } else {
              if (source->reconnectDuration == 0) {
                // blog(LOG_INFO,
                //	"WASAPI: Device '%s' failed to start (source: %s)",
                //	source->device_id
                //	.c_str(),
                //	obs_source_get_name(
                //		source->source));
              }
              stop = true;
              reconnect = true;
              source->reconnectDuration = RECONNECT_INTERVAL;
            }
          } else {
            stop = !source->ProcessCaptureData();
            if (stop) {
              // blog(LOG_INFO,
              //	"Device '%s' invalidated.  Retrying (source: %s)",
              //	source->device_name.c_str(),
              //	obs_source_get_name(
              //		source->source));
              stop = true;
              reconnect = true;
              source->reconnectDuration = RECONNECT_INTERVAL;
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

    if (source->client) {
      source->client->Stop();

      source->capture.Clear();
      source->client.Clear();
    }

    if (idle) {
      SetEvent(source->idleSignal);
    } else if (reconnect) {
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

int32_t SystemModule::SetRecordingDevice(uint16_t index) {
  return 0;
}

int32_t SystemModule::EnumerateDevice(uint16_t index) {
  return 0;
}

void SystemModule::ConvertBuffer(std::vector<int8_t>& data) {
  auto data_ptr = (float*)data.data();
  
  release_capture_buffer.clear();
  int k = 0;
  for (int i; i < channels; ++i) {
    for (int j = i; j < data.size() / format; j += channels, ++k) {
      float inSample = data_ptr[j];
      int16_t outSample;

      if (inSample >= 0) {
        outSample = (int16_t)lrintf(inSample * 32767);
      } else {
        outSample = (int16_t)lrintf(inSample * 32768);
      }
      std::cout << outSample;
      release_capture_buffer.push_back(outSample);
    }
  }
}

bool SystemModule::ProcessCaptureData() {
  if (source) {
    if (stop) {
      return false;
    }

    HRESULT res;
    LPBYTE buffer;
    UINT32 frames;
    DWORD flags;
    UINT64 pos, ts;
    UINT captureSize = 0;

    while (true) {
      res = capture->GetNextPacketSize(&captureSize);
      if (FAILED(res)) {
        if (res != AUDCLNT_E_DEVICE_INVALIDATED)
          // blog(LOG_WARNING,
          //      "[WASAPISource::ProcessCaptureData]"
          //      " capture->GetNextPacketSize"
          //      " failed: %lX",
          //      res);
          return false;
      }

      if (!captureSize) 
      {
        source->SetMute(true);
        break;
      } else {
        source->SetMute(false);
      }

      res = capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
      if (FAILED(res)) {
        if (res != AUDCLNT_E_DEVICE_INVALIDATED)
          // blog(LOG_WARNING,
          //      "[WASAPISource::ProcessCaptureData]"
          //      " capture->GetBuffer"
          //      " failed: %lX",
          //      res);
          return false;
      }

      for (int i = 0; i < frames * format * channels; ++i) {
        // std::cout << "ADD - " << i << std::endl;
        capture_buffer.push_back(buffer[i]);
        
        // std::cout << "WTF " << capture_buffer.size() << " - " << ((sampleRate / 100) * format * channels) << std::endl;
        if (capture_buffer.size() == ((sampleRate / 100) * format * channels)) {

          float* a2 = (float*)capture_buffer.data();
              std::vector<int16_t> res;
              for (int i = 0; i< 480 * 2; i += 2) {
                  float inSample = a2[i];
                  int16_t outSample;

                  if (inSample >= 0) {
                      outSample = (int16_t) lrintf(inSample * 32767.0);
                  } else {
                      outSample = (int16_t) lrintf(inSample * 32768.0);
                  }
                  res.push_back(outSample);
              }
              for (int i = 1; i< 480 * 2; i += 2) {
                  float inSample = a2[i];
                  int16_t outSample;

                  if (inSample >= 0) {
                      outSample = (int16_t) lrintf(inSample * 32767.0);
                  } else {
                      outSample = (int16_t) lrintf(inSample * 32768.0);
                  }
                  res.push_back(outSample);
              }
              source->UpdateFrame((const int16_t *)(res.data()), 480, 48000, 2);
              capture_buffer.clear();
        }
        // std::cout << "WTF " << (capture_buffer.size() == ((sampleRate / 100) * format * channels)) << std::endl;

      }
      
      // std::cout << "HERE4" << std::endl;

      capture->ReleaseBuffer(frames);
    }

    return true;
  } else {
    return false;  // todo wait start;
  }
}

int32_t SystemModule::Init() {
  bool success = false;
  try {
    Initialize();
    success = true;
  } catch (HRError& error) {
    if (!previouslyFailed) {
      // blog(LOG_WARNING,
      //	"[WASAPISource::TryInitialize]:[%s] %s: %lX",
      //	device_name.empty() ? device_id.c_str()
      //	: device_name.c_str(),
      //	error.str, error.hr);
    }
  } catch (const char* error) {
    if (!previouslyFailed) {
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

std::string SystemModule::GetDeviceName(IMMDevice *device)
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

	return device_name;
}

void SystemModule::Initialize() {
  ComPtr<IMMDevice> device;

  // get default output device
  device = InitDevice(enumerator);

  device_name = GetDeviceName(device);
  // std::cout << "NAME - " << device_name << std::endl;

  ResetEvent(receiveSignal);

  ComPtr<IAudioClient> temp_client =
      InitClient(device, channels, format, sampleRate);

  // spec magic (sourceType == SourceType::DeviceOutput)
  ClearBuffer(device);

  ComPtr<IAudioCaptureClient> temp_capture =
      InitCapture(temp_client, receiveSignal);

  client = std::move(temp_client);
  capture = std::move(temp_capture);

  // blog(LOG_INFO, "WASAPI: Device '%s' [%" PRIu32 " Hz] initialized",
  //	device_name.c_str(), sampleRate);
}


int SystemModule::ConvertSpeakerLayout(DWORD layout, WORD channels) {
  switch (layout) {
    case KSAUDIO_SPEAKER_2POINT1:
      return 3;
    case KSAUDIO_SPEAKER_SURROUND:
      return 4;
    case KSAUDIO_SPEAKER_5POINT1_SURROUND:
      return 6;
    case KSAUDIO_SPEAKER_7POINT1_SURROUND:
      return 8;
  }

  return channels;
}

void SystemModule::InitFormat(const WAVEFORMATEX* wfex,
                              int& channels,
                              int& format,
                              uint32_t& sampleRate) {
  DWORD layout = 0;

  if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    WAVEFORMATEXTENSIBLE* ext = (WAVEFORMATEXTENSIBLE*)wfex;
    layout = ext->dwChannelMask;
  }

  /* WASAPI is always float */
  channels = ConvertSpeakerLayout(layout, wfex->nChannels);
  format = 4;
  sampleRate = wfex->nSamplesPerSec;
}


ComPtr<IAudioCaptureClient> SystemModule::InitCapture(IAudioClient* client,
                                                      HANDLE receiveSignal) {
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

rtc::scoped_refptr<AudioSource> SystemModule::CreateSource() {
  if (!source) {
    source = rtc::scoped_refptr<SystemSource>(new SystemSource(this));
  }
  auto result = source;
  source->Release();
  return result;
}

int SystemSource::sources_num = 0;
SystemSource::SystemSource(SystemModuleInterface* module) {
  this->module = module;
  if (sources_num == 0) {
    module->StartRecording();
  }
  ++sources_num;
}



SystemSource::~SystemSource() {
  --sources_num;
  if (sources_num == 0) {
    module->StopRecording();
    module->ResetSource();
  }
}

int32_t SystemModule::RecordingChannels() {
  return channels;
}

void SystemModule::ResetSource() {
  source = nullptr;
}

int SystemModule::StopRecording() {

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

  stop = true;
  return 0;
}

void SystemModule::ClearBuffer(IMMDevice* device) {
  CoTaskMemPtr<WAVEFORMATEX> wfex;
  HRESULT res;
  LPBYTE buffer;
  UINT32 frames;
  ComPtr<IAudioClient> client;

  res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                         (void**)client.Assign());
  if (FAILED(res))
    throw HRError("Failed to activate client context", res);

  res = client->GetMixFormat(&wfex);
  if (FAILED(res))
    throw HRError("Failed to get mix format", res);

  res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, BUFFER_TIME_100NS, 0,
                           wfex, nullptr);
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