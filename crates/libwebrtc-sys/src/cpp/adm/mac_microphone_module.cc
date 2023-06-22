/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if WEBRTC_MAC

#include <ApplicationServices/ApplicationServices.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach.h>   // mach_task_self()
#include <sys/sysctl.h>  // sysctlbyname()
#include "api/make_ref_counted.h"
#include "adm/macos_microphone_module.h"
#include "modules/third_party/portaudio/pa_ringbuffer.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/logging.h"

#define WEBRTC_CA_RETURN_ON_ERR(expr)                                \
  do {                                                               \
    err = expr;                                                      \
    if (err != noErr) {                                              \
      logCAMsg(rtc::LS_ERROR, "Error in " #expr, (const char*)&err); \
      return -1;                                                     \
    }                                                                \
  } while (0)
#define WEBRTC_CA_LOG_ERR(expr)                                      \
  do {                                                               \
    err = expr;                                                      \
    if (err != noErr) {                                              \
      logCAMsg(rtc::LS_ERROR, "Error in " #expr, (const char*)&err); \
    }                                                                \
  } while (0)
#define WEBRTC_CA_LOG_WARN(expr)                                       \
  do {                                                                 \
    err = expr;                                                        \
    if (err != noErr) {                                                \
      logCAMsg(rtc::LS_WARNING, "Error in " #expr, (const char*)&err); \
    }                                                                  \
  } while (0)

int32_t MicrophoneModule::InitMicrophoneLocked() {
  if (_recording) {
    return -1;
  }
  if (InitDevice(_inputDeviceIndex, _inputDeviceID, true) == -1) {
    return -1;
  }
  if (_inputDeviceID == _outputDeviceID) {
    _twoDevices = false;
  } else {
    _twoDevices = true;
  }
  if (_mixerManager.OpenMicrophone(_inputDeviceID) == -1) {
    return -1;
  }
  return 0;
}

enum { MaxNumberDevices = 64 };

bool MicrophoneModule::KeyPressed() {
  bool key_down = false;
  // Loop through all Mac virtual key constant values.
  for (unsigned int key_index = 0; key_index < arraysize(prev_key_state_);
       ++key_index) {
    bool keyState =
        CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, key_index);
    // A false -> true change in keymap means a key is pressed.
    key_down |= (keyState && !prev_key_state_[key_index]);
    // Save current state.
    prev_key_state_[key_index] = keyState;
  }
  return key_down;
}

OSStatus MicrophoneModule::inConverterProc(
    AudioConverterRef,
    UInt32* numberDataPackets,
    AudioBufferList* data,
    AudioStreamPacketDescription** /*dataPacketDescription*/,
    void* userData) {
  MicrophoneModule* ptrThis = static_cast<MicrophoneModule*>(userData);
  RTC_DCHECK(ptrThis != NULL);
  return ptrThis->implInConverterProc(numberDataPackets, data);
}

OSStatus MicrophoneModule::implInConverterProc(UInt32* numberDataPackets,
                                               AudioBufferList* data) {
  RTC_DCHECK(data->mNumberBuffers == 1);
  ring_buffer_size_t numSamples =
      *numberDataPackets * _inStreamFormat.mChannelsPerFrame;
  while (PaUtil_GetRingBufferReadAvailable(_paCaptureBuffer) < numSamples) {
    mach_timespec_t timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = webrtc::TIMER_PERIOD_MS;
    kern_return_t kernErr = semaphore_timedwait(_captureSemaphore, timeout);
    if (kernErr == KERN_OPERATION_TIMED_OUT) {
      int32_t signal = _captureDeviceIsAlive;
      if (signal == 0) {
        // The capture device is no longer alive; stop the worker thread.
        *numberDataPackets = 0;
        return 1;
      }
    } else if (kernErr != KERN_SUCCESS) {
      RTC_LOG(LS_ERROR) << "semaphore_wait() error: " << kernErr;
    }
  }
  // Pass the read pointer directly to the converter to avoid a memcpy.
  void* dummyPtr;
  ring_buffer_size_t dummySize;
  PaUtil_GetRingBufferReadRegions(_paCaptureBuffer, numSamples,
                                  &data->mBuffers->mData, &numSamples,
                                  &dummyPtr, &dummySize);
  PaUtil_AdvanceRingBufferReadIndex(_paCaptureBuffer, numSamples);
  data->mBuffers->mNumberChannels = _inStreamFormat.mChannelsPerFrame;
  *numberDataPackets = numSamples / _inStreamFormat.mChannelsPerFrame;
  data->mBuffers->mDataByteSize =
      *numberDataPackets * _inStreamFormat.mBytesPerPacket;
  return 0;
}

int32_t MicrophoneModule::InitSpeakerLocked() {
  if (_playing) {
    return -1;
  }
  if (InitDevice(_outputDeviceIndex, _outputDeviceID, false) == -1) {
    return -1;
  }
  if (_inputDeviceID == _outputDeviceID) {
    _twoDevices = false;
  } else {
    _twoDevices = true;
  }
  if (_mixerManager.OpenSpeaker(_outputDeviceID) == -1) {
    return -1;
  }
  return 0;
}

int32_t MicrophoneModule::SpeakerIsAvailableLocked(bool& available) {
  bool wasInitialized = _mixerManager.SpeakerIsInitialized();
  // Make an attempt to open up the
  // output mixer corresponding to the currently selected output device.
  //
  if (!wasInitialized && InitSpeakerLocked() == -1) {
    available = false;
    return 0;
  }
  // Given that InitSpeaker was successful, we know that a valid speaker
  // exists.
  available = true;
  // Close the initialized output mixer
  //
  if (!wasInitialized) {
    _mixerManager.CloseSpeaker();
  }
  return 0;
}

OSStatus MicrophoneModule::implOutConverterProc(UInt32* numberDataPackets,
                                                AudioBufferList* data) {
  RTC_DCHECK(data->mNumberBuffers == 1);
  ring_buffer_size_t numSamples =
      *numberDataPackets * _outDesiredFormat.mChannelsPerFrame;
  data->mBuffers->mNumberChannels = _outDesiredFormat.mChannelsPerFrame;
  // Always give the converter as much as it wants, zero padding as required.
  data->mBuffers->mDataByteSize =
      *numberDataPackets * _outDesiredFormat.mBytesPerPacket;
  data->mBuffers->mData = _renderConvertData;
  memset(_renderConvertData, 0, sizeof(_renderConvertData));
  PaUtil_ReadRingBuffer(_paRenderBuffer, _renderConvertData, numSamples);
  kern_return_t kernErr = semaphore_signal_all(_renderSemaphore);
  if (kernErr != KERN_SUCCESS) {
    RTC_LOG(LS_ERROR) << "semaphore_signal_all() error: " << kernErr;
    return 1;
  }
  return 0;
}

OSStatus MicrophoneModule::outConverterProc(AudioConverterRef,
                                            UInt32* numberDataPackets,
                                            AudioBufferList* data,
                                            AudioStreamPacketDescription**,
                                            void* userData) {
  MicrophoneModule* ptrThis = (MicrophoneModule*)userData;
  RTC_DCHECK(ptrThis != NULL);
  return ptrThis->implOutConverterProc(numberDataPackets, data);
}

OSStatus MicrophoneModule::implDeviceIOProc(const AudioBufferList* inputData,
                                            const AudioTimeStamp* inputTime,
                                            AudioBufferList* outputData,
                                            const AudioTimeStamp* outputTime) {
  OSStatus err = noErr;
  UInt64 outputTimeNs = AudioConvertHostTimeToNanos(outputTime->mHostTime);
  UInt64 nowNs = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
  if (!_twoDevices && _recording) {
    implInDeviceIOProc(inputData, inputTime);
  }
  // Check if we should close down audio device
  // Double-checked locking optimization to remove locking overhead
  if (_doStop) {
    webrtc::MutexLock lock(&mutex_);
    if (_doStop) {
      if (_twoDevices || (!_recording && !_playing)) {
        // In the case of a shared device, the single driving ioProc
        // is stopped here
        WEBRTC_CA_LOG_ERR(AudioDeviceStop(_outputDeviceID, _deviceIOProcID));
        WEBRTC_CA_LOG_WARN(
            AudioDeviceDestroyIOProcID(_outputDeviceID, _deviceIOProcID));
        if (err == noErr) {
          RTC_LOG(LS_VERBOSE) << "Playout or shared device stopped";
        }
      }
      _doStop = false;
      _stopEvent.Set();
      return 0;
    }
  }
  if (!_playing) {
    // This can be the case when a shared device is capturing but not
    // rendering. We allow the checks above before returning to avoid a
    // timeout when capturing is stopped.
    return 0;
  }
  RTC_DCHECK(_outStreamFormat.mBytesPerFrame != 0);
  UInt32 size =
      outputData->mBuffers->mDataByteSize / _outStreamFormat.mBytesPerFrame;
  // TODO(xians): signal an error somehow?
  err = AudioConverterFillComplexBuffer(_renderConverter, outConverterProc,
                                        this, &size, outputData, NULL);
  if (err != noErr) {
    if (err == 1) {
      // This is our own error.
      RTC_LOG(LS_ERROR) << "Error in AudioConverterFillComplexBuffer()";
      return 1;
    } else {
      logCAMsg(rtc::LS_ERROR, "Error in AudioConverterFillComplexBuffer()",
               (const char*)&err);
      return 1;
    }
  }
  ring_buffer_size_t bufSizeSamples =
      PaUtil_GetRingBufferReadAvailable(_paRenderBuffer);
  int32_t renderDelayUs =
      static_cast<int32_t>(1e-3 * (outputTimeNs - nowNs) + 0.5);
  renderDelayUs += static_cast<int32_t>(
      (1.0e6 * bufSizeSamples) / _outDesiredFormat.mChannelsPerFrame /
          _outDesiredFormat.mSampleRate +
      0.5);
  _renderDelayUs = renderDelayUs;
  return 0;
}

OSStatus MicrophoneModule::deviceIOProc(AudioDeviceID,
                                        const AudioTimeStamp*,
                                        const AudioBufferList* inputData,
                                        const AudioTimeStamp* inputTime,
                                        AudioBufferList* outputData,
                                        const AudioTimeStamp* outputTime,
                                        void* clientData) {
  MicrophoneModule* ptrThis = (MicrophoneModule*)clientData;
  RTC_DCHECK(ptrThis != NULL);
  ptrThis->implDeviceIOProc(inputData, inputTime, outputData, outputTime);
  // AudioDeviceIOProc functions are supposed to return 0
  return 0;
}

OSStatus MicrophoneModule::implInDeviceIOProc(const AudioBufferList* inputData,
                                              const AudioTimeStamp* inputTime) {
  OSStatus err = noErr;
  UInt64 inputTimeNs = AudioConvertHostTimeToNanos(inputTime->mHostTime);
  UInt64 nowNs = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
  // Check if we should close down audio device
  // Double-checked locking optimization to remove locking overhead
  if (_doStopRec) {
    webrtc::MutexLock lock(&mutex_);
    if (_doStopRec) {
      // This will be signalled only when a shared device is not in use.
      WEBRTC_CA_LOG_ERR(AudioDeviceStop(_inputDeviceID, _inDeviceIOProcID));
      WEBRTC_CA_LOG_WARN(
          AudioDeviceDestroyIOProcID(_inputDeviceID, _inDeviceIOProcID));
      if (err == noErr) {
        RTC_LOG(LS_VERBOSE) << "Recording device stopped";
      }
      _doStopRec = false;
      _stopEventRec.Set();
      return 0;
    }
  }
  if (!_recording) {
    // Allow above checks to avoid a timeout on stopping capture.
    return 0;
  }
  ring_buffer_size_t bufSizeSamples =
      PaUtil_GetRingBufferReadAvailable(_paCaptureBuffer);
  int32_t captureDelayUs =
      static_cast<int32_t>(1e-3 * (nowNs - inputTimeNs) + 0.5);
  captureDelayUs += static_cast<int32_t>((1.0e6 * bufSizeSamples) /
                                             _inStreamFormat.mChannelsPerFrame /
                                             _inStreamFormat.mSampleRate +
                                         0.5);
  _captureDelayUs = captureDelayUs;
  RTC_DCHECK(inputData->mNumberBuffers == 1);
  ring_buffer_size_t numSamples = inputData->mBuffers->mDataByteSize *
                                  _inStreamFormat.mChannelsPerFrame /
                                  _inStreamFormat.mBytesPerPacket;
  PaUtil_WriteRingBuffer(_paCaptureBuffer, inputData->mBuffers->mData,
                         numSamples);
  kern_return_t kernErr = semaphore_signal_all(_captureSemaphore);
  if (kernErr != KERN_SUCCESS) {
    RTC_LOG(LS_ERROR) << "semaphore_signal_all() error: " << kernErr;
  }
  return err;
}

OSStatus MicrophoneModule::inDeviceIOProc(AudioDeviceID,
                                          const AudioTimeStamp*,
                                          const AudioBufferList* inputData,
                                          const AudioTimeStamp* inputTime,
                                          AudioBufferList*,
                                          const AudioTimeStamp*,
                                          void* clientData) {
  MicrophoneModule* ptrThis = (MicrophoneModule*)clientData;
  RTC_DCHECK(ptrThis != NULL);
  ptrThis->implInDeviceIOProc(inputData, inputTime);
  // AudioDeviceIOProc functions are supposed to return 0
  return 0;
}

OSStatus MicrophoneModule::SetDesiredPlayoutFormat() {
  // Our preferred format to work with.
  _outDesiredFormat.mSampleRate = webrtc::N_PLAY_SAMPLES_PER_SEC;
  _outDesiredFormat.mChannelsPerFrame = _playChannels;
  _renderDelayOffsetSamples =
      _renderBufSizeSamples - webrtc::N_BUFFERS_OUT *
                                  webrtc::ENGINE_PLAY_BUF_SIZE_IN_SAMPLES *
                                  _outDesiredFormat.mChannelsPerFrame;
  _outDesiredFormat.mBytesPerPacket =
      _outDesiredFormat.mChannelsPerFrame * sizeof(SInt16);
  // In uncompressed audio, a packet is one frame.
  _outDesiredFormat.mFramesPerPacket = 1;
  _outDesiredFormat.mBytesPerFrame =
      _outDesiredFormat.mChannelsPerFrame * sizeof(SInt16);
  _outDesiredFormat.mBitsPerChannel = sizeof(SInt16) * 8;
  _outDesiredFormat.mFormatFlags =
      kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
#ifdef WEBRTC_ARCH_BIG_ENDIAN
  _outDesiredFormat.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
  _outDesiredFormat.mFormatID = kAudioFormatLinearPCM;
  OSStatus err = noErr;
  WEBRTC_CA_RETURN_ON_ERR(AudioConverterNew(
      &_outDesiredFormat, &_outStreamFormat, &_renderConverter));
  // Try to set buffer size to desired value set to 20ms.
  const uint16_t kPlayBufDelayFixed = 20;
  UInt32 bufByteCount = static_cast<UInt32>(
      (_outStreamFormat.mSampleRate / 1000.0) * kPlayBufDelayFixed *
      _outStreamFormat.mChannelsPerFrame * sizeof(Float32));
  if (_outStreamFormat.mFramesPerPacket != 0) {
    if (bufByteCount % _outStreamFormat.mFramesPerPacket != 0) {
      bufByteCount = (static_cast<UInt32>(bufByteCount /
                                          _outStreamFormat.mFramesPerPacket) +
                      1) *
                     _outStreamFormat.mFramesPerPacket;
    }
  }
  // Ensure the buffer size is within the range provided by the device.
  AudioObjectPropertyAddress propertyAddress = {
      kAudioDevicePropertyDataSource, kAudioDevicePropertyScopeOutput, 0};
  propertyAddress.mSelector = kAudioDevicePropertyBufferSizeRange;
  AudioValueRange range;
  UInt32 size = sizeof(range);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _outputDeviceID, &propertyAddress, 0, NULL, &size, &range));
  if (range.mMinimum > bufByteCount) {
    bufByteCount = range.mMinimum;
  } else if (range.mMaximum < bufByteCount) {
    bufByteCount = range.mMaximum;
  }
  propertyAddress.mSelector = kAudioDevicePropertyBufferSize;
  size = sizeof(bufByteCount);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectSetPropertyData(
      _outputDeviceID, &propertyAddress, 0, NULL, size, &bufByteCount));
  // Get render device latency.
  propertyAddress.mSelector = kAudioDevicePropertyLatency;
  UInt32 latency = 0;
  size = sizeof(UInt32);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _outputDeviceID, &propertyAddress, 0, NULL, &size, &latency));
  _renderLatencyUs =
      static_cast<uint32_t>((1.0e6 * latency) / _outStreamFormat.mSampleRate);
  // Get render stream latency.
  propertyAddress.mSelector = kAudioDevicePropertyStreams;
  AudioStreamID stream = 0;
  size = sizeof(AudioStreamID);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _outputDeviceID, &propertyAddress, 0, NULL, &size, &stream));
  propertyAddress.mSelector = kAudioStreamPropertyLatency;
  size = sizeof(UInt32);
  latency = 0;
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _outputDeviceID, &propertyAddress, 0, NULL, &size, &latency));
  _renderLatencyUs +=
      static_cast<uint32_t>((1.0e6 * latency) / _outStreamFormat.mSampleRate);
  RTC_LOG(LS_VERBOSE) << "initial playout status: _renderDelayOffsetSamples="
                      << _renderDelayOffsetSamples
                      << ", _renderDelayUs=" << _renderDelayUs
                      << ", _renderLatencyUs=" << _renderLatencyUs;
  return 0;
}

bool MicrophoneModule::SpeakerIsInitialized() const {
  return (_mixerManager.SpeakerIsInitialized());
}

int updateFrameCount = 0;

bool MicrophoneModule::CaptureWorkerThread() {
  OSStatus err = noErr;
  UInt32 noRecSamples = webrtc::ENGINE_REC_BUF_SIZE_IN_SAMPLES *
                        _inDesiredFormat.mChannelsPerFrame;
  SInt16 recordBuffer[noRecSamples];
  UInt32 size = webrtc::ENGINE_REC_BUF_SIZE_IN_SAMPLES;
  AudioBufferList engineBuffer;
  engineBuffer.mNumberBuffers = 1;  // Interleaved channels.
  engineBuffer.mBuffers->mNumberChannels = _inDesiredFormat.mChannelsPerFrame;
  engineBuffer.mBuffers->mDataByteSize =
      _inDesiredFormat.mBytesPerPacket * noRecSamples;
  engineBuffer.mBuffers->mData = recordBuffer;
  err = AudioConverterFillComplexBuffer(_captureConverter, inConverterProc,
                                        this, &size, &engineBuffer, NULL);
  if (err != noErr) {
    if (err == 1) {
      // This is our own error.
      return false;
    } else {
      logCAMsg(rtc::LS_ERROR, "Error in AudioConverterFillComplexBuffer()",
               (const char*)&err);
      return false;
    }
  }
  if (source != nullptr) {
    updateFrameCount++;
    if (updateFrameCount % 100 == 0) {
      RTC_LOG(LS_ERROR) << "UpdateFrameCount: " << updateFrameCount;
    }
    source->UpdateFrame(recordBuffer, size, webrtc::N_REC_SAMPLES_PER_SEC,
                        _inDesiredFormat.mChannelsPerFrame);
  }
  // TODO(xians): what if the returned size is incorrect?
  if (size == webrtc::ENGINE_REC_BUF_SIZE_IN_SAMPLES) {
    int32_t msecOnPlaySide;
    int32_t msecOnRecordSide;
    int32_t captureDelayUs = _captureDelayUs;
    int32_t renderDelayUs = _renderDelayUs;
    msecOnPlaySide =
        static_cast<int32_t>(1e-3 * (renderDelayUs + _renderLatencyUs) + 0.5);
    msecOnRecordSide =
        static_cast<int32_t>(1e-3 * (captureDelayUs + _captureLatencyUs) + 0.5);
  }
  return true;
}

int32_t MicrophoneModule::InitRecording() {
  RTC_LOG(LS_INFO) << "InitRecording";
  webrtc::MutexLock lock(&mutex_);
  if (_recording) {
    return -1;
  }
  if (!_inputDeviceIsSpecified) {
    return -1;
  }
  if (_recIsInitialized) {
    return 0;
  }
  // Initialize the microphone (devices might have been added or removed)
  if (InitMicrophoneLocked() == -1) {
    RTC_LOG(LS_WARNING) << "InitMicrophone() failed";
  }
  if (!SpeakerIsInitialized()) {
    // Make this call to check if we are using
    // one or two devices (_twoDevices)
    bool available = false;
    if (SpeakerIsAvailableLocked(available) == -1) {
      RTC_LOG(LS_WARNING) << "SpeakerIsAvailable() failed";
    }
  }
  OSStatus err = noErr;
  UInt32 size = 0;
  PaUtil_FlushRingBuffer(_paCaptureBuffer);
  _captureDelayUs = 0;
  _captureLatencyUs = 0;
  _captureDeviceIsAlive = 1;
  _doStopRec = false;
  // Get current stream description
  AudioObjectPropertyAddress propertyAddress = {
      kAudioDevicePropertyStreamFormat, kAudioDevicePropertyScopeInput, 0};
  memset(&_inStreamFormat, 0, sizeof(_inStreamFormat));
  size = sizeof(_inStreamFormat);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _inputDeviceID, &propertyAddress, 0, NULL, &size, &_inStreamFormat));
  if (_inStreamFormat.mFormatID != kAudioFormatLinearPCM) {
    logCAMsg(rtc::LS_ERROR, "Unacceptable input stream format -> mFormatID",
             (const char*)&_inStreamFormat.mFormatID);
    return -1;
  }
  if (_inStreamFormat.mChannelsPerFrame > webrtc::N_DEVICE_CHANNELS) {
    RTC_LOG(LS_ERROR)
        << "Too many channels on input device (mChannelsPerFrame = "
        << _inStreamFormat.mChannelsPerFrame << ")";
    return -1;
  }
  const int io_block_size_samples = _inStreamFormat.mChannelsPerFrame *
                                    _inStreamFormat.mSampleRate / 100 *
                                    webrtc::N_BLOCKS_IO;
  if (io_block_size_samples > _captureBufSizeSamples) {
    RTC_LOG(LS_ERROR) << "Input IO block size (" << io_block_size_samples
                      << ") is larger than ring buffer ("
                      << _captureBufSizeSamples << ")";
    return -1;
  }
  RTC_LOG(LS_VERBOSE) << "Input stream format:";
  RTC_LOG(LS_VERBOSE) << "mSampleRate = " << _inStreamFormat.mSampleRate
                      << ", mChannelsPerFrame = "
                      << _inStreamFormat.mChannelsPerFrame;
  RTC_LOG(LS_VERBOSE) << "mBytesPerPacket = " << _inStreamFormat.mBytesPerPacket
                      << ", mFramesPerPacket = "
                      << _inStreamFormat.mFramesPerPacket;
  RTC_LOG(LS_VERBOSE) << "mBytesPerFrame = " << _inStreamFormat.mBytesPerFrame
                      << ", mBitsPerChannel = "
                      << _inStreamFormat.mBitsPerChannel;
  RTC_LOG(LS_VERBOSE) << "mFormatFlags = " << _inStreamFormat.mFormatFlags;
  logCAMsg(rtc::LS_VERBOSE, "mFormatID",
           (const char*)&_inStreamFormat.mFormatID);
  // Our preferred format to work with
  if (_inStreamFormat.mChannelsPerFrame >= 2 && (_recChannels == 2)) {
    _inDesiredFormat.mChannelsPerFrame = 2;
  } else {
    // Disable stereo recording when we only have one channel on the device.
    _inDesiredFormat.mChannelsPerFrame = 1;
    _recChannels = 1;
    RTC_LOG(LS_VERBOSE) << "Stereo recording unavailable on this device";
  }
  _inDesiredFormat.mSampleRate = webrtc::N_REC_SAMPLES_PER_SEC;
  _inDesiredFormat.mBytesPerPacket =
      _inDesiredFormat.mChannelsPerFrame * sizeof(SInt16);
  _inDesiredFormat.mFramesPerPacket = 1;
  _inDesiredFormat.mBytesPerFrame =
      _inDesiredFormat.mChannelsPerFrame * sizeof(SInt16);
  _inDesiredFormat.mBitsPerChannel = sizeof(SInt16) * 8;
  _inDesiredFormat.mFormatFlags =
      kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
#ifdef WEBRTC_ARCH_BIG_ENDIAN
  _inDesiredFormat.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
  _inDesiredFormat.mFormatID = kAudioFormatLinearPCM;
  WEBRTC_CA_RETURN_ON_ERR(AudioConverterNew(&_inStreamFormat, &_inDesiredFormat,
                                            &_captureConverter));
  // First try to set buffer size to desired value (10 ms * N_BLOCKS_IO)
  // TODO(xians): investigate this block.
  UInt32 bufByteCount =
      (UInt32)((_inStreamFormat.mSampleRate / 1000.0) * 10.0 *
               webrtc::N_BLOCKS_IO * _inStreamFormat.mChannelsPerFrame *
               sizeof(Float32));
  if (_inStreamFormat.mFramesPerPacket != 0) {
    if (bufByteCount % _inStreamFormat.mFramesPerPacket != 0) {
      bufByteCount =
          ((UInt32)(bufByteCount / _inStreamFormat.mFramesPerPacket) + 1) *
          _inStreamFormat.mFramesPerPacket;
    }
  }
  // Ensure the buffer size is within the acceptable range provided by the
  // device.
  propertyAddress.mSelector = kAudioDevicePropertyBufferSizeRange;
  AudioValueRange range;
  size = sizeof(range);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _inputDeviceID, &propertyAddress, 0, NULL, &size, &range));
  if (range.mMinimum > bufByteCount) {
    bufByteCount = range.mMinimum;
  } else if (range.mMaximum < bufByteCount) {
    bufByteCount = range.mMaximum;
  }
  propertyAddress.mSelector = kAudioDevicePropertyBufferSize;
  size = sizeof(bufByteCount);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectSetPropertyData(
      _inputDeviceID, &propertyAddress, 0, NULL, size, &bufByteCount));
  // Get capture device latency
  propertyAddress.mSelector = kAudioDevicePropertyLatency;
  UInt32 latency = 0;
  size = sizeof(UInt32);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _inputDeviceID, &propertyAddress, 0, NULL, &size, &latency));
  _captureLatencyUs = (UInt32)((1.0e6 * latency) / _inStreamFormat.mSampleRate);
  // Get capture stream latency
  propertyAddress.mSelector = kAudioDevicePropertyStreams;
  AudioStreamID stream = 0;
  size = sizeof(AudioStreamID);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _inputDeviceID, &propertyAddress, 0, NULL, &size, &stream));
  propertyAddress.mSelector = kAudioStreamPropertyLatency;
  size = sizeof(UInt32);
  latency = 0;
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      _inputDeviceID, &propertyAddress, 0, NULL, &size, &latency));
  _captureLatencyUs +=
      (UInt32)((1.0e6 * latency) / _inStreamFormat.mSampleRate);
  // Listen for format changes
  // TODO(xians): should we be using kAudioDevicePropertyDeviceHasChanged?
  propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
  WEBRTC_CA_LOG_WARN(AudioObjectAddPropertyListener(
      _inputDeviceID, &propertyAddress, &objectListenerProc, this));
  // Listen for processor overloads
  propertyAddress.mSelector = kAudioDeviceProcessorOverload;
  WEBRTC_CA_LOG_WARN(AudioObjectAddPropertyListener(
      _inputDeviceID, &propertyAddress, &objectListenerProc, this));
  if (_twoDevices) {
    WEBRTC_CA_RETURN_ON_ERR(AudioDeviceCreateIOProcID(
        _inputDeviceID, inDeviceIOProc, this, &_inDeviceIOProcID));
  } else if (!_playIsInitialized) {
    WEBRTC_CA_RETURN_ON_ERR(AudioDeviceCreateIOProcID(
        _inputDeviceID, deviceIOProc, this, &_deviceIOProcID));
  }
  // Mark recording side as initialized
  _recIsInitialized = true;
  return 0;
}

int32_t MicrophoneModule::GetNumberDevices(const AudioObjectPropertyScope scope,
                                           AudioDeviceID scopedDeviceIds[],
                                           const uint32_t deviceListLength) {
  OSStatus err = noErr;
  AudioObjectPropertyAddress propertyAddress = {
      kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster};
  UInt32 size = 0;
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyDataSize(
      kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size));
  if (size == 0) {
    RTC_LOG(LS_WARNING) << "No devices";
    return 0;
  }
  UInt32 numberDevices = size / sizeof(AudioDeviceID);
  const auto deviceIds = std::make_unique<AudioDeviceID[]>(numberDevices);
  AudioBufferList* bufferList = NULL;
  UInt32 numberScopedDevices = 0;
  // First check if there is a default device and list it
  UInt32 hardwareProperty = 0;
  if (scope == kAudioDevicePropertyScopeOutput) {
    hardwareProperty = kAudioHardwarePropertyDefaultOutputDevice;
  } else {
    hardwareProperty = kAudioHardwarePropertyDefaultInputDevice;
  }
  AudioObjectPropertyAddress propertyAddressDefault = {
      hardwareProperty, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster};
  AudioDeviceID usedID;
  UInt32 uintSize = sizeof(UInt32);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                     &propertyAddressDefault, 0,
                                                     NULL, &uintSize, &usedID));
  if (usedID != kAudioDeviceUnknown) {
    scopedDeviceIds[numberScopedDevices] = usedID;
    numberScopedDevices++;
  } else {
    RTC_LOG(LS_WARNING) << "GetNumberDevices(): Default device unknown";
  }
  // Then list the rest of the devices
  bool listOK = true;
  WEBRTC_CA_LOG_ERR(AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                               &propertyAddress, 0, NULL, &size,
                                               deviceIds.get()));
  if (err != noErr) {
    listOK = false;
  } else {
    propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
    propertyAddress.mScope = scope;
    propertyAddress.mElement = 0;
    for (UInt32 i = 0; i < numberDevices; i++) {
      // Check for input channels
      WEBRTC_CA_LOG_ERR(AudioObjectGetPropertyDataSize(
          deviceIds[i], &propertyAddress, 0, NULL, &size));
      if (err == kAudioHardwareBadDeviceError) {
        // This device doesn't actually exist; continue iterating.
        continue;
      } else if (err != noErr) {
        listOK = false;
        break;
      }
      bufferList = (AudioBufferList*)malloc(size);
      WEBRTC_CA_LOG_ERR(AudioObjectGetPropertyData(
          deviceIds[i], &propertyAddress, 0, NULL, &size, bufferList));
      if (err != noErr) {
        listOK = false;
        break;
      }
      if (bufferList->mNumberBuffers > 0) {
        if (numberScopedDevices >= deviceListLength) {
          RTC_LOG(LS_ERROR) << "Device list is not long enough";
          listOK = false;
          break;
        }
        scopedDeviceIds[numberScopedDevices] = deviceIds[i];
        numberScopedDevices++;
      }
      free(bufferList);
      bufferList = NULL;
    }  // for
  }
  if (!listOK) {
    if (bufferList) {
      free(bufferList);
      bufferList = NULL;
    }
    return -1;
  }
  return numberScopedDevices;
}

int32_t MicrophoneModule::InitDevice(const uint16_t userDeviceIndex,
                                     AudioDeviceID& deviceId,
                                     const bool isInput) {
  OSStatus err = noErr;
  UInt32 size = 0;
  AudioObjectPropertyScope deviceScope;
  AudioObjectPropertySelector defaultDeviceSelector;
  AudioDeviceID deviceIds[MaxNumberDevices];
  if (isInput) {
    deviceScope = kAudioDevicePropertyScopeInput;
    defaultDeviceSelector = kAudioHardwarePropertyDefaultInputDevice;
  } else {
    deviceScope = kAudioDevicePropertyScopeOutput;
    defaultDeviceSelector = kAudioHardwarePropertyDefaultOutputDevice;
  }
  AudioObjectPropertyAddress propertyAddress = {
      defaultDeviceSelector, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster};
  // Get the actual device IDs
  int numberDevices =
      GetNumberDevices(deviceScope, deviceIds, MaxNumberDevices);
  if (numberDevices < 0) {
    return -1;
  } else if (numberDevices == 0) {
    RTC_LOG(LS_ERROR) << "InitDevice(): No devices";
    return -1;
  }
  bool isDefaultDevice = false;
  deviceId = kAudioDeviceUnknown;
  if (userDeviceIndex == 0) {
    // Try to use default system device
    size = sizeof(AudioDeviceID);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
        kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, &deviceId));
    if (deviceId == kAudioDeviceUnknown) {
      RTC_LOG(LS_WARNING) << "No default device exists";
    } else {
      isDefaultDevice = true;
    }
  }
  if (!isDefaultDevice) {
    deviceId = deviceIds[userDeviceIndex];
  }
  // Obtain device name and manufacturer for logging.
  // Also use this as a test to ensure a user-set device ID is valid.
  char devName[128];
  char devManf[128];
  memset(devName, 0, sizeof(devName));
  memset(devManf, 0, sizeof(devManf));
  propertyAddress.mSelector = kAudioDevicePropertyDeviceName;
  propertyAddress.mScope = deviceScope;
  propertyAddress.mElement = 0;
  size = sizeof(devName);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(deviceId, &propertyAddress,
                                                     0, NULL, &size, devName));
  propertyAddress.mSelector = kAudioDevicePropertyDeviceManufacturer;
  size = sizeof(devManf);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(deviceId, &propertyAddress,
                                                     0, NULL, &size, devManf));
  if (isInput) {
    RTC_LOG(LS_INFO) << "Input device: " << devManf << " " << devName;
  } else {
    RTC_LOG(LS_INFO) << "Output device: " << devManf << " " << devName;
  }
  return 0;
}

int32_t MicrophoneModule::HandleDeviceChange() {
  OSStatus err = noErr;
  RTC_LOG(LS_VERBOSE) << "kAudioHardwarePropertyDevices";
  // A device has changed. Check if our registered devices have been removed.
  // Ensure the devices have been initialized, meaning the IDs are valid.
  if (MicrophoneIsInitialized()) {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDeviceIsAlive, kAudioDevicePropertyScopeInput, 0};
    UInt32 deviceIsAlive = 1;
    UInt32 size = sizeof(UInt32);
    err = AudioObjectGetPropertyData(_inputDeviceID, &propertyAddress, 0, NULL,
                                     &size, &deviceIsAlive);
    if (err == kAudioHardwareBadDeviceError || deviceIsAlive == 0) {
      RTC_LOG(LS_WARNING) << "Capture device is not alive (probably removed)";
      _captureDeviceIsAlive = 0;
      _mixerManager.CloseMicrophone();
    } else if (err != noErr) {
      logCAMsg(rtc::LS_ERROR, "Error in AudioDeviceGetProperty()",
               (const char*)&err);
      return -1;
    }
  }
  if (SpeakerIsInitialized()) {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDeviceIsAlive, kAudioDevicePropertyScopeOutput, 0};
    UInt32 deviceIsAlive = 1;
    UInt32 size = sizeof(UInt32);
    err = AudioObjectGetPropertyData(_outputDeviceID, &propertyAddress, 0, NULL,
                                     &size, &deviceIsAlive);
    if (err == kAudioHardwareBadDeviceError || deviceIsAlive == 0) {
      RTC_LOG(LS_WARNING) << "Render device is not alive (probably removed)";
      _renderDeviceIsAlive = 0;
      _mixerManager.CloseSpeaker();
    } else if (err != noErr) {
      logCAMsg(rtc::LS_ERROR, "Error in AudioDeviceGetProperty()",
               (const char*)&err);
      return -1;
    }
  }
  return 0;
}

int32_t MicrophoneModule::HandleStreamFormatChange(
    const AudioObjectID objectId,
    const AudioObjectPropertyAddress propertyAddress) {
  OSStatus err = noErr;
  RTC_LOG(LS_VERBOSE) << "Stream format changed";
  if (objectId != _inputDeviceID && objectId != _outputDeviceID) {
    return 0;
  }
  // Get the new device format
  AudioStreamBasicDescription streamFormat;
  UInt32 size = sizeof(streamFormat);
  WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
      objectId, &propertyAddress, 0, NULL, &size, &streamFormat));
  if (streamFormat.mFormatID != kAudioFormatLinearPCM) {
    logCAMsg(rtc::LS_ERROR, "Unacceptable input stream format -> mFormatID",
             (const char*)&streamFormat.mFormatID);
    return -1;
  }
  if (streamFormat.mChannelsPerFrame > webrtc::N_DEVICE_CHANNELS) {
    RTC_LOG(LS_ERROR) << "Too many channels on device (mChannelsPerFrame = "
                      << streamFormat.mChannelsPerFrame << ")";
    return -1;
  }
  RTC_LOG(LS_VERBOSE) << "Stream format:";
  RTC_LOG(LS_VERBOSE) << "mSampleRate = " << streamFormat.mSampleRate
                      << ", mChannelsPerFrame = "
                      << streamFormat.mChannelsPerFrame;
  RTC_LOG(LS_VERBOSE) << "mBytesPerPacket = " << streamFormat.mBytesPerPacket
                      << ", mFramesPerPacket = "
                      << streamFormat.mFramesPerPacket;
  RTC_LOG(LS_VERBOSE) << "mBytesPerFrame = " << streamFormat.mBytesPerFrame
                      << ", mBitsPerChannel = " << streamFormat.mBitsPerChannel;
  RTC_LOG(LS_VERBOSE) << "mFormatFlags = " << streamFormat.mFormatFlags;
  logCAMsg(rtc::LS_VERBOSE, "mFormatID", (const char*)&streamFormat.mFormatID);
  if (propertyAddress.mScope == kAudioDevicePropertyScopeInput) {
    const int io_block_size_samples = streamFormat.mChannelsPerFrame *
                                      streamFormat.mSampleRate / 100 *
                                      webrtc::N_BLOCKS_IO;
    if (io_block_size_samples > _captureBufSizeSamples) {
      RTC_LOG(LS_ERROR) << "Input IO block size (" << io_block_size_samples
                        << ") is larger than ring buffer ("
                        << _captureBufSizeSamples << ")";
      return -1;
    }
    memcpy(&_inStreamFormat, &streamFormat, sizeof(streamFormat));
    if (_inStreamFormat.mChannelsPerFrame >= 2 && (_recChannels == 2)) {
      _inDesiredFormat.mChannelsPerFrame = 2;
    } else {
      // Disable stereo recording when we only have one channel on the device.
      _inDesiredFormat.mChannelsPerFrame = 1;
      _recChannels = 1;
      RTC_LOG(LS_VERBOSE) << "Stereo recording unavailable on this device";
    }
    // Recreate the converter with the new format
    // TODO(xians): make this thread safe
    WEBRTC_CA_RETURN_ON_ERR(AudioConverterDispose(_captureConverter));
    WEBRTC_CA_RETURN_ON_ERR(AudioConverterNew(&streamFormat, &_inDesiredFormat,
                                              &_captureConverter));
  } else {
    memcpy(&_outStreamFormat, &streamFormat, sizeof(streamFormat));
    // Our preferred format to work with
    if (_outStreamFormat.mChannelsPerFrame < 2) {
      _playChannels = 1;
      RTC_LOG(LS_VERBOSE) << "Stereo playout unavailable on this device";
    }
    WEBRTC_CA_RETURN_ON_ERR(SetDesiredPlayoutFormat());
  }
  return 0;
}

int32_t MicrophoneModule::HandleDataSourceChange(
    const AudioObjectID objectId,
    const AudioObjectPropertyAddress propertyAddress) {
  OSStatus err = noErr;
  if (_macBookPro &&
      propertyAddress.mScope == kAudioDevicePropertyScopeOutput) {
    RTC_LOG(LS_VERBOSE) << "Data source changed";
    _macBookProPanRight = false;
    UInt32 dataSource = 0;
    UInt32 size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(
        objectId, &propertyAddress, 0, NULL, &size, &dataSource));
    if (dataSource == 'ispk') {
      _macBookProPanRight = true;
      RTC_LOG(LS_VERBOSE)
          << "MacBook Pro using internal speakers; stereo panning right";
    } else {
      RTC_LOG(LS_VERBOSE) << "MacBook Pro not using internal speakers";
    }
  }
  return 0;
}
int32_t MicrophoneModule::HandleProcessorOverload(
    const AudioObjectPropertyAddress propertyAddress) {
  // TODO(xians): we probably want to notify the user in some way of the
  // overload. However, the Windows interpretations of these errors seem to
  // be more severe than what ProcessorOverload is thrown for.
  //
  // We don't log the notification, as it's sent from the HAL's IO thread. We
  // don't want to slow it down even further.
  if (propertyAddress.mScope == kAudioDevicePropertyScopeInput) {
    // RTC_LOG(LS_WARNING) << "Capture processor // overload";
    //_callback->ProblemIsReported(
    // SndCardStreamObserver::ERecordingProblem);
  } else {
    // RTC_LOG(LS_WARNING) << "Render processor overload";
    //_callback->ProblemIsReported(
    // SndCardStreamObserver::EPlaybackProblem);
  }
  return 0;
}

OSStatus MicrophoneModule::implObjectListenerProc(
    const AudioObjectID objectId,
    const UInt32 numberAddresses,
    const AudioObjectPropertyAddress addresses[]) {
  RTC_LOG(LS_VERBOSE) << "AudioDeviceMac::implObjectListenerProc()";
  for (UInt32 i = 0; i < numberAddresses; i++) {
    if (addresses[i].mSelector == kAudioHardwarePropertyDevices) {
      HandleDeviceChange();
    } else if (addresses[i].mSelector == kAudioDevicePropertyStreamFormat) {
      HandleStreamFormatChange(objectId, addresses[i]);
    } else if (addresses[i].mSelector == kAudioDevicePropertyDataSource) {
      HandleDataSourceChange(objectId, addresses[i]);
    } else if (addresses[i].mSelector == kAudioDeviceProcessorOverload) {
      HandleProcessorOverload(addresses[i]);
    }
  }
  return 0;
}

// CoreAudio errors are best interpreted as four character strings.
void MicrophoneModule::logCAMsg(const rtc::LoggingSeverity sev,
                                const char* msg,
                                const char* err) {
  RTC_DCHECK(msg != NULL);
  RTC_DCHECK(err != NULL);
#ifdef WEBRTC_ARCH_BIG_ENDIAN
  switch (sev) {
    case rtc::LS_ERROR:
      RTC_LOG(LS_ERROR) << msg << ": " << err[0] << err[1] << err[2] << err[3];
      break;
    case rtc::LS_WARNING:
      RTC_LOG(LS_WARNING) << msg << ": " << err[0] << err[1] << err[2]
                          << err[3];
      break;
    default:
      break;
  }
#else
  // We need to flip the characters in this case.
  switch (sev) {
    case rtc::LS_ERROR:
      RTC_LOG(LS_ERROR) << msg << ": " << err[3] << err[2] << err[1] << err[0];
      break;
    case rtc::LS_WARNING:
      RTC_LOG(LS_WARNING) << msg << ": " << err[3] << err[2] << err[1]
                          << err[0];
      break;
    default:
      break;
  }
#endif
}

OSStatus MicrophoneModule::objectListenerProc(
    AudioObjectID objectId,
    UInt32 numberAddresses,
    const AudioObjectPropertyAddress addresses[],
    void* clientData) {
  MicrophoneModule* ptrThis = (MicrophoneModule*)clientData;
  RTC_DCHECK(ptrThis != NULL);
  ptrThis->implObjectListenerProc(objectId, numberAddresses, addresses);
  // AudioObjectPropertyListenerProc functions are supposed to return 0
  return 0;
}

/// ============================================================================
/// ============================== PUBLIC METHODS ==============================
/// ============================================================================

MicrophoneModule::MicrophoneModule(rtc::Thread* worker_thread)
    : _ptrAudioBuffer(NULL),
      _mixerManager(),
      _inputDeviceIndex(0),
      _outputDeviceIndex(0),
      _inputDeviceID(kAudioObjectUnknown),
      _outputDeviceID(kAudioObjectUnknown),
      _inputDeviceIsSpecified(false),
      _outputDeviceIsSpecified(false),
      _recChannels(webrtc::N_REC_CHANNELS),
      _playChannels(webrtc::N_PLAY_CHANNELS),
      _captureBufData(NULL),
      _renderBufData(NULL),
      _initialized(false),
      _isShutDown(false),
      _recording(false),
      _playing(false),
      _recIsInitialized(false),
      _playIsInitialized(false),
      _renderDeviceIsAlive(1),
      _captureDeviceIsAlive(1),
      _twoDevices(true),
      _doStop(false),
      _doStopRec(false),
      _macBookPro(false),
      _macBookProPanRight(false),
      _captureLatencyUs(0),
      _renderLatencyUs(0),
      _captureDelayUs(0),
      _renderDelayUs(0),
      _renderDelayOffsetSamples(0),
      _paCaptureBuffer(NULL),
      _paRenderBuffer(NULL),
      _captureBufSizeSamples(0),
      _renderBufSizeSamples(0),
      prev_key_state_(),
      worker_thread(worker_thread) {
  RTC_DLOG(LS_INFO) << __FUNCTION__ << " created";
  memset(_renderConvertData, 0, sizeof(_renderConvertData));
  memset(&_outStreamFormat, 0, sizeof(AudioStreamBasicDescription));
  memset(&_outDesiredFormat, 0, sizeof(AudioStreamBasicDescription));
  memset(&_inStreamFormat, 0, sizeof(AudioStreamBasicDescription));
  memset(&_inDesiredFormat, 0, sizeof(AudioStreamBasicDescription));
}

MicrophoneModule::~MicrophoneModule() {
  RTC_DLOG(LS_INFO) << __FUNCTION__ << " destroyed";
  if (!_isShutDown) {
    Terminate();
  }
  RTC_DCHECK(capture_worker_thread_.empty());
  //  RTC_DCHECK(render_worker_thread_.empty());
  if (_paRenderBuffer) {
    delete _paRenderBuffer;
    _paRenderBuffer = NULL;
  }
  if (_paCaptureBuffer) {
    delete _paCaptureBuffer;
    _paCaptureBuffer = NULL;
  }
  if (_renderBufData) {
    delete[] _renderBufData;
    _renderBufData = NULL;
  }
  if (_captureBufData) {
    delete[] _captureBufData;
    _captureBufData = NULL;
  }
  kern_return_t kernErr = KERN_SUCCESS;
  kernErr = semaphore_destroy(mach_task_self(), _renderSemaphore);
  if (kernErr != KERN_SUCCESS) {
    RTC_LOG(LS_ERROR) << "semaphore_destroy() error: " << kernErr;
  }
  kernErr = semaphore_destroy(mach_task_self(), _captureSemaphore);
  if (kernErr != KERN_SUCCESS) {
    RTC_LOG(LS_ERROR) << "semaphore_destroy() error: " << kernErr;
  }
}

int32_t MicrophoneModule::Init() {
  webrtc::MutexLock lock(&mutex_);
  if (_initialized) {
    return 0;
  }
  OSStatus err = noErr;
  _isShutDown = false;
  // PortAudio ring buffers require an elementCount which is a power of two.
  if (_renderBufData == NULL) {
    UInt32 powerOfTwo = 1;
    while (powerOfTwo < webrtc::PLAY_BUF_SIZE_IN_SAMPLES) {
      powerOfTwo <<= 1;
    }
    _renderBufSizeSamples = powerOfTwo;
    _renderBufData = new SInt16[_renderBufSizeSamples];
  }
  if (_paRenderBuffer == NULL) {
    _paRenderBuffer = new PaUtilRingBuffer;
    ring_buffer_size_t bufSize = -1;
    bufSize = PaUtil_InitializeRingBuffer(
        _paRenderBuffer, sizeof(SInt16), _renderBufSizeSamples, _renderBufData);
    if (bufSize == -1) {
      RTC_LOG(LS_ERROR) << "PaUtil_InitializeRingBuffer() error";
      return -1;
    }
  }
  if (_captureBufData == NULL) {
    UInt32 powerOfTwo = 1;
    while (powerOfTwo < webrtc::REC_BUF_SIZE_IN_SAMPLES) {
      powerOfTwo <<= 1;
    }
    _captureBufSizeSamples = powerOfTwo;
    _captureBufData = new Float32[_captureBufSizeSamples];
  }
  if (_paCaptureBuffer == NULL) {
    _paCaptureBuffer = new PaUtilRingBuffer;
    ring_buffer_size_t bufSize = -1;
    bufSize =
        PaUtil_InitializeRingBuffer(_paCaptureBuffer, sizeof(Float32),
                                    _captureBufSizeSamples, _captureBufData);
    if (bufSize == -1) {
      RTC_LOG(LS_ERROR) << "PaUtil_InitializeRingBuffer() error";
      return -1;
    }
  }
  kern_return_t kernErr = KERN_SUCCESS;
  kernErr = semaphore_create(mach_task_self(), &_renderSemaphore,
                             SYNC_POLICY_FIFO, 0);
  if (kernErr != KERN_SUCCESS) {
    RTC_LOG(LS_ERROR) << "semaphore_create() error: " << kernErr;
    return -1;
  }
  kernErr = semaphore_create(mach_task_self(), &_captureSemaphore,
                             SYNC_POLICY_FIFO, 0);
  if (kernErr != KERN_SUCCESS) {
    RTC_LOG(LS_ERROR) << "semaphore_create() error: " << kernErr;
    return -1;
  }
  // Setting RunLoop to NULL here instructs HAL to manage its own thread for
  // notifications. This was the default behaviour on OS X 10.5 and earlier,
  // but now must be explicitly specified. HAL would otherwise try to use the
  // main thread to issue notifications.
  AudioObjectPropertyAddress propertyAddress = {
      kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster};
  CFRunLoopRef runLoop = NULL;
  UInt32 size = sizeof(CFRunLoopRef);
  int aoerr = AudioObjectSetPropertyData(
      kAudioObjectSystemObject, &propertyAddress, 0, NULL, size, &runLoop);
  if (aoerr != noErr) {
    RTC_LOG(LS_ERROR) << "Error in AudioObjectSetPropertyData: "
                      << (const char*)&aoerr;
    return -1;
  }
  // Listen for any device changes.
  propertyAddress.mSelector = kAudioHardwarePropertyDevices;
  WEBRTC_CA_LOG_ERR(AudioObjectAddPropertyListener(
      kAudioObjectSystemObject, &propertyAddress, &objectListenerProc, this));
  // Determine if this is a MacBook Pro
  _macBookPro = false;
  _macBookProPanRight = false;
  char buf[128];
  size_t length = sizeof(buf);
  memset(buf, 0, length);
  int intErr = sysctlbyname("hw.model", buf, &length, NULL, 0);
  if (intErr != 0) {
    RTC_LOG(LS_ERROR) << "Error in sysctlbyname(): " << err;
  } else {
    RTC_LOG(LS_VERBOSE) << "Hardware model: " << buf;
    if (strncmp(buf, "MacBookPro", 10) == 0) {
      _macBookPro = true;
    }
  }
  _initialized = true;
  return 0;
}

int32_t MicrophoneModule::Terminate() {
  if (!_initialized) {
    return 0;
  }
  if (_recording) {
    RTC_LOG(LS_ERROR) << "Recording must be stopped";
    return -1;
  }
  if (_playing) {
    RTC_LOG(LS_ERROR) << "Playback must be stopped";
    return -1;
  }
  webrtc::MutexLock lock(&mutex_);
  _mixerManager.Close();
  OSStatus err = noErr;
  int retVal = 0;
  AudioObjectPropertyAddress propertyAddress = {
      kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster};
  WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(
      kAudioObjectSystemObject, &propertyAddress, &objectListenerProc, this));
  err = AudioHardwareUnload();
  if (err != noErr) {
    logCAMsg(rtc::LS_ERROR, "Error in AudioHardwareUnload()",
             (const char*)&err);
    retVal = -1;
  }
  _isShutDown = true;
  _initialized = false;
  _outputDeviceIsSpecified = false;
  _inputDeviceIsSpecified = false;
  return retVal;
}

int32_t MicrophoneModule::InitMicrophone() {
  webrtc::MutexLock lock(&mutex_);
  return InitMicrophoneLocked();
}

int32_t MicrophoneModule::MicrophoneMuteIsAvailable(bool* available) {
  bool isAvailable(false);
  bool wasInitialized = _mixerManager.MicrophoneIsInitialized();
  // Make an attempt to open up the
  // input mixer corresponding to the currently selected input device.
  //
  if (!wasInitialized && InitMicrophone() == -1) {
    // If we end up here it means that the selected microphone has no volume
    // control, hence it is safe to state that there is no boost control
    // already at this stage.
    *available = false;
    return 0;
  }
  // Check if the selected microphone has a mute control
  //
  _mixerManager.MicrophoneMuteIsAvailable(isAvailable);
  *available = isAvailable;
  // Close the initialized input mixer
  //
  if (!wasInitialized) {
    _mixerManager.CloseMicrophone();
  }
  return 0;
}

int32_t MicrophoneModule::SetMicrophoneMute(bool enable) {
  return (_mixerManager.SetMicrophoneMute(enable));
}

int32_t MicrophoneModule::MicrophoneMute(bool* enabled) const {
  bool muted(0);
  if (_mixerManager.MicrophoneMute(muted) == -1) {
    return -1;
  }
  *enabled = muted;
  return 0;
}

bool MicrophoneModule::MicrophoneIsInitialized() {
  return (_mixerManager.MicrophoneIsInitialized());
}

int32_t MicrophoneModule::MicrophoneVolumeIsAvailable(bool* available) {
  bool wasInitialized = _mixerManager.MicrophoneIsInitialized();
  // Make an attempt to open up the
  // input mixer corresponding to the currently selected output device.
  //
  if (!wasInitialized && InitMicrophone() == -1) {
    // If we end up here it means that the selected microphone has no volume
    // control.
    *available = false;
    return 0;
  }
  // Given that InitMicrophone was successful, we know that a volume control
  // exists
  //
  *available = true;
  // Close the initialized input mixer
  //
  if (!wasInitialized) {
    _mixerManager.CloseMicrophone();
  }
  return 0;
}

int32_t MicrophoneModule::SetMicrophoneVolume(uint32_t volume) {
  return (_mixerManager.SetMicrophoneVolume(volume));
}

int32_t MicrophoneModule::MicrophoneVolume(uint32_t* volume) const {
  uint32_t level(0);
  if (_mixerManager.MicrophoneVolume(level) == -1) {
    RTC_LOG(LS_WARNING) << "failed to retrieve current microphone level";
    return -1;
  }
  *volume = level;
  return 0;
}

int32_t MicrophoneModule::MaxMicrophoneVolume(uint32_t* maxVolume) const {
  uint32_t maxVol(0);
  if (_mixerManager.MaxMicrophoneVolume(maxVol) == -1) {
    return -1;
  }
  *maxVolume = maxVol;
  return 0;
}

int32_t MicrophoneModule::MinMicrophoneVolume(uint32_t* minVolume) const {
  uint32_t minVol(0);
  if (_mixerManager.MinMicrophoneVolume(minVol) == -1) {
    return -1;
  }
  *minVolume = minVol;
  return 0;
}

// Settings.
int32_t MicrophoneModule::SetRecordingDevice(uint16_t index) {
  auto start = _recIsInitialized;
  StopRecording();
  if (_recIsInitialized) {
    return -1;
  }
  AudioDeviceID recDevices[MaxNumberDevices];
  uint32_t nDevices = GetNumberDevices(kAudioDevicePropertyScopeInput,
                                       recDevices, MaxNumberDevices);
  RTC_LOG(LS_VERBOSE) << "number of available waveform-audio input devices is "
                      << nDevices;
  if (index > (nDevices - 1)) {
    RTC_LOG(LS_ERROR) << "device index is out of range [0," << (nDevices - 1)
                      << "]";
    return -1;
  }
  _inputDeviceIndex = index;
  _inputDeviceIsSpecified = true;

  if (start) {
    InitRecording();
    StartRecording();
  }

  return 0;
}

std::atomic<int> MicrophoneSource::sources_num = 0;
MicrophoneSource::MicrophoneSource(MicrophoneModuleInterface* module,
                                   rtc::Thread* thread) {
  this->module = module;
  this->worker_thread = thread;

  if (sources_num == 0) {
    module->StartRecording();
  }
  ++sources_num;
}

MicrophoneSource::~MicrophoneSource() {
  --sources_num;
  if (sources_num == 0) {
    worker_thread->BlockingCall([this] {
      this->module->StopRecording();
      this->module->ResetSource();
    });
  }
}

rtc::scoped_refptr<AudioSource> MicrophoneModule::CreateSource() {
  if (!_recording) {
    InitRecording();
  }

  if (!source) {
    source = rtc::scoped_refptr<MicrophoneSource>(new MicrophoneSource(this));
  }
  auto result = source;
  source->Release();
  return result;
}

void MicrophoneModule::ResetSource() {
  webrtc::MutexLock lock(&mutex_);
  source = nullptr;
}

int32_t MicrophoneModule::StopRecording() {
  webrtc::MutexLock lock(&mutex_);
  if (!_recIsInitialized) {
    return 0;
  }
  OSStatus err = noErr;
  int32_t captureDeviceIsAlive = _captureDeviceIsAlive;
  if (_twoDevices && captureDeviceIsAlive == 1) {
    // Recording side uses its own dedicated device and IOProc.
    if (_recording) {
      _recording = false;
      _doStopRec = true;  // Signal to io proc to stop audio device
      mutex_.Unlock();    // Cannot be under lock, risk of deadlock
      if (!_stopEventRec.Wait(webrtc::TimeDelta::Seconds(2))) {
        webrtc::MutexLock lockScoped(&mutex_);
        RTC_LOG(LS_WARNING) << "Timed out stopping the capture IOProc."
                               "We may have failed to detect a device removal.";
        WEBRTC_CA_LOG_WARN(AudioDeviceStop(_inputDeviceID, _inDeviceIOProcID));
        WEBRTC_CA_LOG_WARN(
            AudioDeviceDestroyIOProcID(_inputDeviceID, _inDeviceIOProcID));
      }
      mutex_.Lock();

      _doStopRec = false;
      RTC_LOG(LS_INFO) << "Recording stopped (input device)";
    } else if (_recIsInitialized) {
      WEBRTC_CA_LOG_WARN(
          AudioDeviceDestroyIOProcID(_inputDeviceID, _inDeviceIOProcID));
      RTC_LOG(LS_INFO) << "Recording uninitialized (input device)";
    }
  } else {
    // We signal a stop for a shared device even when rendering has
    // not yet ended. This is to ensure the IOProc will return early as
    // intended (by checking `_recording`) before accessing
    // resources we free below (e.g. the capture converter).
    //
    // In the case of a shared devcie, the IOProc will verify
    // rendering has ended before stopping itself.
    if (_recording && captureDeviceIsAlive == 1) {
      _recording = false;
      _doStop = true;   // Signal to io proc to stop audio device
      mutex_.Unlock();  // Cannot be under lock, risk of deadlock
      if (!_stopEvent.Wait(webrtc::TimeDelta::Seconds(2))) {
        webrtc::MutexLock lockScoped(&mutex_);
        RTC_LOG(LS_WARNING) << "Timed out stopping the shared IOProc."
                               "We may have failed to detect a device removal.";
        // We assume rendering on a shared device has stopped as well if
        // the IOProc times out.
        WEBRTC_CA_LOG_WARN(AudioDeviceStop(_outputDeviceID, _deviceIOProcID));
        WEBRTC_CA_LOG_WARN(
            AudioDeviceDestroyIOProcID(_outputDeviceID, _deviceIOProcID));
      }
      mutex_.Lock();

      _doStop = false;
      RTC_LOG(LS_INFO) << "Recording stopped (shared device)";
    } else if (_recIsInitialized && !_playing && !_playIsInitialized) {
      WEBRTC_CA_LOG_WARN(
          AudioDeviceDestroyIOProcID(_outputDeviceID, _deviceIOProcID));
      RTC_LOG(LS_INFO) << "Recording uninitialized (shared device)";
    }
  }
  // Setting this signal will allow the worker thread to be stopped.
  _captureDeviceIsAlive = 0;
  if (!capture_worker_thread_.empty()) {
    mutex_.Unlock();
    capture_worker_thread_.Finalize();
    mutex_.Lock();
  }
  WEBRTC_CA_LOG_WARN(AudioConverterDispose(_captureConverter));
  // Remove listeners.
  AudioObjectPropertyAddress propertyAddress = {
      kAudioDevicePropertyStreamFormat, kAudioDevicePropertyScopeInput, 0};
  WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(
      _inputDeviceID, &propertyAddress, &objectListenerProc, this));
  propertyAddress.mSelector = kAudioDeviceProcessorOverload;
  WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(
      _inputDeviceID, &propertyAddress, &objectListenerProc, this));
  _recIsInitialized = false;
  _recording = false;
  return 0;
}

int32_t MicrophoneModule::StartRecording() {
  webrtc::MutexLock lock(&mutex_);
  if (!_recIsInitialized) {
    return -1;
  }
  if (_recording) {
    return 0;
  }
  if (!_initialized) {
    RTC_LOG(LS_ERROR) << "Recording worker thread has not been started";
    return -1;
  }
  RTC_DCHECK(capture_worker_thread_.empty());
  capture_worker_thread_ = rtc::PlatformThread::SpawnJoinable(
      [this] {
        while (CaptureWorkerThread()) {
        }
      },
      "CaptureWorkerThread",
      rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime));
  OSStatus err = noErr;
  if (_twoDevices) {
    WEBRTC_CA_RETURN_ON_ERR(
        AudioDeviceStart(_inputDeviceID, _inDeviceIOProcID));
  } else if (!_playing) {
    WEBRTC_CA_RETURN_ON_ERR(AudioDeviceStart(_inputDeviceID, _deviceIOProcID));
  }
  _recording = true;
  return 0;
}

int32_t MicrophoneModule::RecordingChannels() {
  return _recChannels;
}

#endif