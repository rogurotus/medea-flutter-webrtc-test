// #pragma once

// #include "modules/audio_device/linux/audio_device_pulse_linux.h"
// #include "custom_audio.h"

// class ADM;

// class MicrophoneModule : webrtc::AudioDeviceLinuxPulse {
//     public: 
//     ADM* adm = nullptr;
//     webrtc::AudioDeviceBuffer* cb = nullptr;

//     MicrophoneModule();
//     InitStatus Init() override;
//     CustomAudioSource* createSource();
//     int32_t ProcessRecordedData(int8_t* bufferData,
//                                   uint32_t bufferSizeInSamples,
//                                   uint32_t recDelay) override;
// };