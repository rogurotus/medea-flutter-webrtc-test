
#define WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE 1
#include <iostream>
#include "api/task_queue/task_queue_factory.h"
#include "modules/audio_device/audio_device_impl.h"

#include <memory>

#include "api/media_stream_interface.h"
#include "api/sequence_checker.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/audio_device_generic.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/audio_device_defines.h"
#include "modules/audio_device/linux/audio_mixer_manager_pulse_linux.h"
#include "modules/audio_device/linux/pulseaudiosymboltable_linux.h"
#include "rtc_base/event.h"
#include "rtc_base/platform_thread.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"
#include "custom_audio.h"

#include "modules/audio_mixer/audio_mixer_impl.h"

#include "api/audio/audio_mixer.h"

#include "api/audio/audio_frame.h"

#if defined(WEBRTC_USE_X11)
#include <X11/Xlib.h>
#endif


// We define this flag if it's missing from our headers, because we want to be
// able to compile against old headers but still use PA_STREAM_ADJUST_LATENCY
// if run against a recent version of the library.
#ifndef PA_STREAM_ADJUST_LATENCY
#define PA_STREAM_ADJUST_LATENCY 0x2000U
#endif
#ifndef PA_STREAM_START_MUTED
#define PA_STREAM_START_MUTED 0x1000U
#endif

// Set this constant to 0 to disable latency reading
const uint32_t WEBRTC_PA_REPORT_LATENCY = 1;

// Constants from implementation by Tristan Schmelcher [tschmelcher@google.com]

// First PulseAudio protocol version that supports PA_STREAM_ADJUST_LATENCY.
const uint32_t WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION = 13;

// Some timing constants for optimal operation. See
// https://tango.0pointer.de/pipermail/pulseaudio-discuss/2008-January/001170.html
// for a good explanation of some of the factors that go into this.

// Playback.

// For playback, there is a round-trip delay to fill the server-side playback
// buffer, so setting too low of a latency is a buffer underflow risk. We will
// automatically increase the latency if a buffer underflow does occur, but we
// also enforce a sane minimum at start-up time. Anything lower would be
// virtually guaranteed to underflow at least once, so there's no point in
// allowing lower latencies.
const uint32_t WEBRTC_PA_PLAYBACK_LATENCY_MINIMUM_MSECS = 20;

// Every time a playback stream underflows, we will reconfigure it with target
// latency that is greater by this amount.
const uint32_t WEBRTC_PA_PLAYBACK_LATENCY_INCREMENT_MSECS = 20;

// We also need to configure a suitable request size. Too small and we'd burn
// CPU from the overhead of transfering small amounts of data at once. Too large
// and the amount of data remaining in the buffer right before refilling it
// would be a buffer underflow risk. We set it to half of the buffer size.
const uint32_t WEBRTC_PA_PLAYBACK_REQUEST_FACTOR = 2;

// Capture.

// For capture, low latency is not a buffer overflow risk, but it makes us burn
// CPU from the overhead of transfering small amounts of data at once, so we set
// a recommended value that we use for the kLowLatency constant (but if the user
// explicitly requests something lower then we will honour it).
// 1ms takes about 6-7% CPU. 5ms takes about 5%. 10ms takes about 4.x%.
const uint32_t WEBRTC_PA_LOW_CAPTURE_LATENCY_MSECS = 10;

// There is a round-trip delay to ack the data to the server, so the
// server-side buffer needs extra space to prevent buffer overflow. 20ms is
// sufficient, but there is no penalty to making it bigger, so we make it huge.
// (750ms is libpulse's default value for the _total_ buffer size in the
// kNoLatencyRequirements case.)
const uint32_t WEBRTC_PA_CAPTURE_BUFFER_EXTRA_MSECS = 750;

const uint32_t WEBRTC_PA_MSECS_PER_SEC = 1000;

// Init _configuredLatencyRec/Play to this value to disable latency requirements
const int32_t WEBRTC_PA_NO_LATENCY_REQUIREMENTS = -1;

// Set this const to 1 to account for peeked and used data in latency
// calculation
const uint32_t WEBRTC_PA_CAPTURE_BUFFER_LATENCY_ADJUSTMENT = 0;

typedef webrtc::adm_linux_pulse::PulseAudioSymbolTable WebRTCPulseSymbolTable;
WebRTCPulseSymbolTable* GetPulseSymbolTable2();

class ADM;

class temp{
 public:

  CustomAudioSource* source = nullptr;

  temp();

  int32_t StartRecording();

  int Init();
  CustomAudioSource* createSource();
  void PaLock();
  void PaUnLock();
  int32_t LatencyUsecs(pa_stream* stream);
    static void PaSourceInfoCallback(pa_context* c,
                                   const pa_source_info* i,
                                   int eol,
                                   void* pThis);
  void PaSourceInfoCallbackHandler(const pa_source_info* i, int eol);

  bool RecThreadProcess();
  int32_t ReadRecordedData(const void* bufferData, size_t bufferSize);
  int32_t ProcessRecordedData(int8_t* bufferData,
                              uint32_t bufferSizeInSamples,
                              uint32_t recDelay);
  bool KeyPressed() const;
  int16_t RecordingDevices();

  void EnableReadCallback();
  void DisableReadCallback();
  static void PaStreamReadCallback(pa_stream* a /*unused1*/,
                                   size_t b /*unused2*/,
                                   void* pThis);
  void PaStreamReadCallbackHandler();
  static void PaContextStateCallback(pa_context* c, void* pThis);
    static void PaServerInfoCallback(pa_context* c,
                                   const pa_server_info* i,
                                   void* pThis);
    void PaContextStateCallbackHandler(pa_context* c);
  void PaServerInfoCallbackHandler(const pa_server_info* i);

  int32_t InitPulseAudio();
  int32_t TerminatePulseAudio();
  void WaitForOperationCompletion(pa_operation* paOperation) const;
  int32_t CheckPulseAudioVersion();
  int32_t InitSamplingFrequency();
  int32_t StopRecording();
  int32_t InitRecording();
  int32_t InitMicrophone();
  int32_t GetDefaultDeviceInfo(bool recDevice, char* name, uint16_t& index);
  static void PaStreamStateCallback(pa_stream* p, void* pThis);
  void PaStreamStateCallbackHandler(pa_stream* p);
  static void PaStreamOverflowCallback(pa_stream* unused, void* pThis);
  void PaStreamOverflowCallbackHandler();
  int32_t SetRecordingDevice(uint16_t index);

  int32_t StereoRecordingIsAvailable(bool& available);
  int32_t SetStereoRecording(bool enable);
  int32_t StereoRecording(bool& enabled) const;



  bool _inputDeviceIsSpecified = false;
  int sample_rate_hz_ = 0;
  uint8_t _recChannels = 1;
  bool _initialized = false;
  bool _recording = false;
  bool _recIsInitialized = false;
  bool quit_ = false;
  bool _startRec = false;
  uint32_t _sndCardPlayDelay = 0;
  int _deviceIndex = -1;
  int16_t _numRecDevices = 0;
  char* _playDeviceName = nullptr;
  char* _recDeviceName = nullptr;
  char* _playDisplayDeviceName = nullptr;
  char* _recDisplayDeviceName = nullptr;
  int8_t* _recBuffer = nullptr;
  size_t _recordBufferSize = 0;
  size_t _recordBufferUsed = 0;
  const void* _tempSampleData = nullptr;
  size_t _tempSampleDataSize = 0;
  int32_t _configuredLatencyRec = 0;
  uint16_t _paDeviceIndex = -1;
  bool _paStateChanged = false;
  pa_threaded_mainloop* _paMainloop = nullptr;
  pa_mainloop_api* _paMainloopApi = nullptr;
  pa_context* _paContext = nullptr;
  pa_stream* _recStream = nullptr;
  uint32_t _recStreamFlags = 0;
  pa_stream* _playStream = nullptr;

  rtc::Event _timeEventRec;
  webrtc::Mutex mutex_;
  int16_t _numPlayDevices;
  pa_buffer_attr _recBufferAttr;
  int _inputDeviceIndex;
  rtc::Event _recStartEvent;
  rtc::PlatformThread _ptrThreadRec;
  char _paServerVersion[32];
  webrtc::AudioMixerManagerLinuxPulse _mixerManager;
  webrtc::AudioDeviceBuffer* _ptrAudioBuffer;
  webrtc::AudioFrame frame1;
  webrtc::AudioFrame frame2;

  #if defined(WEBRTC_USE_X11)
  char _oldKeyState[32];
  Display* _XDisplay;
  #endif

  ADM* adm = nullptr;
  webrtc::AudioDeviceBuffer* cb = nullptr;
};


class ADM : public webrtc::AudioDeviceModuleImpl {
 public:
  ADM(AudioLayer audio_layer, webrtc::TaskQueueFactory* task_queue_factory);
  int32_t StartRecording() override { return 0; };

  static rtc::scoped_refptr<AudioDeviceModule> Create(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  // static
  static rtc::scoped_refptr<AudioDeviceModuleForTest> CreateForTest(
      AudioLayer audio_layer,
      webrtc::TaskQueueFactory* task_queue_factory);

  void Process();

  rtc::scoped_refptr<webrtc::AudioMixerImpl> mixer = webrtc::AudioMixerImpl::Create();
  temp* da = new temp();
  CustomAudioSource* createSource();
  rtc::PlatformThread th;
};