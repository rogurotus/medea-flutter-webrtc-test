/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma warning(disable : 4995)  // name was marked as #pragma deprecated

#if (_MSC_VER >= 1310) && (_MSC_VER < 1400)
// Reports the major and minor versions of the compiler.
// For example, 1310 for Microsoft Visual C++ .NET 2003. 1310 represents version
// 13 and a 1.0 point release. The Visual C++ 2005 compiler version is 1400.
// Type cl /? at the command line to see the major and minor versions of your
// compiler along with the build number.
#pragma message(">> INFO: Windows Core Audio is not supported in VS 2003")
#endif

#include "modules/audio_device/audio_device_config.h"

#ifdef WEBRTC_WINDOWS_CORE_AUDIO_BUILD

// clang-format off
// To get Windows includes in the right order, this must come before the Windows
// includes below.
#include "windows_microphone_module.h"
// clang-format on

#include <string.h>

#include <comdef.h>
#include <dmo.h>
#include <functiondiscoverykeys_devpkey.h>
#include <mmsystem.h>
#include <strsafe.h>
#include <uuids.h>
#include <windows.h>

#include <iomanip>

#include "api/make_ref_counted.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread.h"
#include "rtc_base/string_utils.h"
#include "rtc_base/thread_annotations.h"
#include "system_wrappers/include/sleep.h"

// Macro that calls a COM method returning HRESULT value.
#define EXIT_ON_ERROR(hres) \
  do {                      \
    if (FAILED(hres))       \
      goto Exit;            \
  } while (0)

// Macro that continues to a COM error.
#define CONTINUE_ON_ERROR(hres) \
  do {                          \
    if (FAILED(hres))           \
      goto Next;                \
  } while (0)

// Macro that releases a COM object if not NULL.
#define SAFE_RELEASE(p) \
  do {                  \
    if ((p)) {          \
      (p)->Release();   \
      (p) = NULL;       \
    }                   \
  } while (0)

#define ROUND(x) ((x) >= 0 ? (int)((x) + 0.5) : (int)((x)-0.5))

// REFERENCE_TIME time units per millisecond
#define REFTIMES_PER_MILLISEC 10000

typedef struct tagTHREADNAME_INFO {
  DWORD dwType;      // must be 0x1000
  LPCSTR szName;     // pointer to name (in user addr space)
  DWORD dwThreadID;  // thread ID (-1=caller thread)
  DWORD dwFlags;     // reserved for future use, must be zero
} THREADNAME_INFO;

namespace webrtc {
namespace {

enum { COM_THREADING_MODEL = COINIT_MULTITHREADED };

enum { kAecCaptureStreamIndex = 0, kAecRenderStreamIndex = 1 };

// An implementation of IMediaBuffer, as required for
// IMediaObject::ProcessOutput(). After consuming data provided by
// ProcessOutput(), call SetLength() to update the buffer availability.
//
// Example implementation:
// http://msdn.microsoft.com/en-us/library/dd376684(v=vs.85).aspx
class MediaBufferImpl final : public IMediaBuffer {
 public:
  explicit MediaBufferImpl(DWORD maxLength)
      : _data(new BYTE[maxLength]),
        _length(0),
        _maxLength(maxLength),
        _refCount(0) {}

  // IMediaBuffer methods.
  STDMETHOD(GetBufferAndLength(BYTE** ppBuffer, DWORD* pcbLength)) {
    if (!ppBuffer || !pcbLength) {
      return E_POINTER;
    }

    *ppBuffer = _data;
    *pcbLength = _length;

    return S_OK;
  }

  STDMETHOD(GetMaxLength(DWORD* pcbMaxLength)) {
    if (!pcbMaxLength) {
      return E_POINTER;
    }

    *pcbMaxLength = _maxLength;
    return S_OK;
  }

  STDMETHOD(SetLength(DWORD cbLength)) {
    if (cbLength > _maxLength) {
      return E_INVALIDARG;
    }

    _length = cbLength;
    return S_OK;
  }

  // IUnknown methods.
  STDMETHOD_(ULONG, AddRef()) { return InterlockedIncrement(&_refCount); }

  STDMETHOD(QueryInterface(REFIID riid, void** ppv)) {
    if (!ppv) {
      return E_POINTER;
    } else if (riid != IID_IMediaBuffer && riid != IID_IUnknown) {
      return E_NOINTERFACE;
    }

    *ppv = static_cast<IMediaBuffer*>(this);
    AddRef();
    return S_OK;
  }

  STDMETHOD_(ULONG, Release()) {
    LONG refCount = InterlockedDecrement(&_refCount);
    if (refCount == 0) {
      delete this;
    }

    return refCount;
  }

 private:
  ~MediaBufferImpl() { delete[] _data; }

  BYTE* _data;
  DWORD _length;
  const DWORD _maxLength;
  LONG _refCount;
};
}
}  // namespace

#endif WEBRTC_WINDOWS_CORE_AUDIO_BUILD