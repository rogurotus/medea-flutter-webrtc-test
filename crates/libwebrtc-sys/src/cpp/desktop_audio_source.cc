#include "libwebrtc-sys/include/desktop_audio_source.h"
#include <pulse/pulseaudio.h>
#include <chrono>
#include <thread>

void tempa24(pa_context* c, void* pThis);

struct pulse_data {
  pa_stream* stream;

  /* user settings */
  char* device;
  bool is_default;
  bool input;

  /* server info */
  pa_sample_format_t format;
  uint_fast32_t samples_per_sec;
  uint_fast32_t bytes_per_frame;
  uint_fast8_t channels;
  uint64_t first_ts;

  /* statistics */
  uint_fast32_t packets;
  uint_fast64_t frames;
};

class PA {
 public:
  PA() {
    printf("DEBUG 1");
    _paMainloop = pa_threaded_mainloop_new();
    auto retVal = pa_threaded_mainloop_start(_paMainloop);
    printf("DEBUG 2");
    if (retVal != PA_OK) {
      printf("failed to start main loop, error=%i", retVal);
    }

    pa_threaded_mainloop_lock(_paMainloop);

    _paMainloopApi = pa_threaded_mainloop_get_api(_paMainloop);
    if (!_paMainloopApi) {
      printf("could not create mainloop API");
      pa_threaded_mainloop_unlock(_paMainloop);
    }

    _paContext = pa_context_new(_paMainloopApi, "MY WEBRTC VoiceEngine");
    if (!_paContext) {
      printf("could not create context");
      pa_threaded_mainloop_unlock(_paMainloop);
    }

    // Set state callback function
    pa_context_set_state_callback(_paContext, tempa24, this);

    // Connect the context to a server (default)
    _paStateChanged = false;
    retVal = pa_context_connect(_paContext, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

    if (retVal != PA_OK) {
      printf("failed to connect context, error=%i", retVal);
      pa_threaded_mainloop_unlock(_paMainloop);
    }

    // Wait for state change
    while (!_paStateChanged) {
      pa_threaded_mainloop_wait(_paMainloop);
    }

    // Now check to see what final state we reached.
    pa_context_state_t state = pa_context_get_state(_paContext);

    if (state != PA_CONTEXT_READY) {
      if (state == PA_CONTEXT_FAILED) {
      } else if (state == PA_CONTEXT_TERMINATED) {
      } else {
        // Shouldn't happen, because we only signal on one of those three
        // states
      }
      pa_threaded_mainloop_unlock(_paMainloop);
    }

    pa_threaded_mainloop_unlock(_paMainloop);
  }
  // PulseAudio
  uint16_t _paDeviceIndex;
  bool _paStateChanged;

  pa_threaded_mainloop* _paMainloop;
  pa_mainloop_api* _paMainloopApi;
  pa_context* _paContext;

  pa_stream* _recStream;
  pa_stream* _playStream;
  uint32_t _recStreamFlags;
  uint32_t _playStreamFlags;
  pa_buffer_attr _playBufferAttr;
  pa_buffer_attr _recBufferAttr;

  void PaContextStateCallbackHandler(pa_context* c) {
    pa_context_state_t state = pa_context_get_state(c);
    switch (state) {
      case PA_CONTEXT_UNCONNECTED:
        break;
      case PA_CONTEXT_CONNECTING:
      case PA_CONTEXT_AUTHORIZING:
      case PA_CONTEXT_SETTING_NAME:
        break;
      case PA_CONTEXT_FAILED:
      case PA_CONTEXT_TERMINATED:
        _paStateChanged = true;
        pa_threaded_mainloop_signal(_paMainloop, 0);
        break;
      case PA_CONTEXT_READY:
        _paStateChanged = true;
        pa_threaded_mainloop_signal(_paMainloop, 0);
        break;
    }
  }

  int_fast32_t pulse_context_ready() {
    pa_threaded_mainloop_lock(_paMainloop);

    if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(_paContext))) {
      pa_threaded_mainloop_unlock(_paMainloop);
      return -1;
    }

    while (pa_context_get_state(_paContext) != PA_CONTEXT_READY)
      pa_threaded_mainloop_wait(_paMainloop);

    pa_threaded_mainloop_unlock(_paMainloop);
    return 0;
  }

  void pulse_signal(int wait_for_accept) {
    pa_threaded_mainloop_signal(_paMainloop, wait_for_accept);
  }

  int_fast32_t pulse_get_source_info(pa_source_info_cb_t cb,
                                     const char* name,
                                     void* userdata) {
    if (pulse_context_ready() < 0)
      return -1;

    pa_threaded_mainloop_lock(_paMainloop);

    printf("HERE1");
    pa_operation* op =
        pa_context_get_source_info_by_name(_paContext, name, cb, userdata);
    printf("HERE2");

    if (!op) {
      pa_threaded_mainloop_unlock(_paMainloop);
      return -1;
    }
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
      pa_threaded_mainloop_wait(_paMainloop);
    pa_operation_unref(op);

    pa_threaded_mainloop_unlock(_paMainloop);

    return 0;
  }

  int_fast32_t pulse_get_idx_source_info(pa_source_info_cb_t cb,
                                     int idx,
                                     void* userdata) {
    if (pulse_context_ready() < 0)
      return -1;

    pa_threaded_mainloop_lock(_paMainloop);

    printf("HERE1");
    pa_operation* op =
        pa_context_get_source_info_by_index(_paContext, idx, cb, userdata);
    printf("HERE2");

    if (!op) {
      pa_threaded_mainloop_unlock(_paMainloop);
      return -1;
    }
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
      pa_threaded_mainloop_wait(_paMainloop);
    pa_operation_unref(op);

    pa_threaded_mainloop_unlock(_paMainloop);

    return 0;
  }

int_fast32_t pulseaudio_context_ready()
{
    pa_threaded_mainloop_lock(_paMainloop);


	if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(_paContext))) {
		pa_threaded_mainloop_unlock(_paMainloop);

		return -1;
	}

	while (pa_context_get_state(_paContext) != PA_CONTEXT_READY)
		pa_threaded_mainloop_wait(_paMainloop);

		pa_threaded_mainloop_unlock(_paMainloop);

	return 0;
}

int_fast32_t pulseaudio_get_server_info(pa_server_info_cb_t cb, void *userdata)
{
	if (pulseaudio_context_ready() < 0)
		return -1;

    pa_threaded_mainloop_lock(_paMainloop);


	pa_operation *op =
		pa_context_get_server_info(_paContext, cb, userdata);

	if (!op) {
		pa_threaded_mainloop_unlock(_paMainloop);
		return -1;
	}

	while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
		pa_threaded_mainloop_wait(_paMainloop);
  
	pa_operation_unref(op);

	pa_threaded_mainloop_unlock(_paMainloop);

	return 0;
}
};

void tempa24(pa_context* c, void* pThis) {
  static_cast<PA*>(pThis)->PaContextStateCallbackHandler(c);
}

static void mycb(pa_context* c,
                 const pa_source_info* i,
                 int eol,
                 void* userdata) {
  PA* pa = (PA*) userdata;
  if (i)
  {

  printf("\nDATA, %s\n", i->name);
  printf("\nDATA, %s\n", i->description);
  printf("\nDATA, %s\n", i->monitor_of_sink_name);
  printf("\nDATA, %s\n", i->driver);
  }
  pa->pulse_signal(0);

}

static void mycb2(pa_context *c, const pa_server_info*i, void *userdata) {
  PA* pa = (PA*) userdata;
  printf("DATA, %s\n", i->default_source_name);
  printf("DATA, %s\n", i->default_sink_name);
  pa->pulse_signal(0);
}

DesktopAudioSource::DesktopAudioSource() {
  printf("WASM3\n");
  PA* pa = new PA();
  printf("WASM2\n");

  pa->pulseaudio_get_server_info(mycb2, pa);
  // pa->pulse_get_idx_source_info(mycb, 0, pa);
  pa->pulse_get_source_info(mycb, "bluez_sink.F8_AB_E5_7D_EC_E0.handsfree_head_unit.monitor", pa);
  // pa->pulse_get_idx_source_info(mycb, 1, pa);
  // pa->pulse_get_idx_source_info(mycb, 2, pa);
  // pa->pulse_get_idx_source_info(mycb, 3, pa);
  // pa->pulse_get_idx_source_info(mycb, 4, pa);
  // pa->pulse_get_idx_source_info(mycb, 5, pa);
  // pa->pulse_get_idx_source_info(mycb, 6, pa);
  // pa->pulse_get_idx_source_info(mycb, 7, pa);

  printf("WASM\n");
  auto th = std::thread([=] {
    int data[64] = {0,   256, 0,   256, 128, 256, 128, 256, 128, 256, 128,
                    128, 256, 128, 256, 128, 256, 128, 128, 128, 128, 128,
                    0,   0,   0,   0,   0,   0,   0,   128, 128, 128, 128,
                    128, 128, 128, 128, 256, 256, 256, 256, 256, 256, 256,
                    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
                    128, 128, 128, 128, 128, 128, 128, 128, 128};
    while (true) {
      for (int i = 0; i < vec.size(); ++i) {
        printf("DATA\n");
        vec[i]->OnData(data, 16, 16, 2, 16);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  });
  th.detach();
}
// TODO(deadbeef): Makes all the interfaces pure virtual after they're
// implemented in chromium.

// Sets the volume of the source. `volume` is in  the range of [0, 10].
// TODO(tommi): This method should be on the track and ideally volume should
// be applied in the track in a way that does not affect clones of the track.
void DesktopAudioSource::SetVolume(double volume) {
  printf("WASM2\n");
};

// Registers/unregisters observers to the audio source.
void DesktopAudioSource::RegisterAudioObserver(AudioObserver* observer) {
  printf("WASM3\n");
};
void DesktopAudioSource::UnregisterAudioObserver(AudioObserver* observer) {
  printf("WASM4\n");
};

// TODO(tommi): Make pure virtual.
void DesktopAudioSource::AddSink(webrtc::AudioTrackSinkInterface* sink) {
  printf("WASM5\n");
  vec.push_back(sink);
};

void DesktopAudioSource::RemoveSink(webrtc::AudioTrackSinkInterface* sink) {
  printf("WASM6\n");
  for (int i = 0; i < vec.size(); ++i) {
    if (vec[i] == sink) {
      vec.erase(vec.begin() + i);
      return;
    }
  }
};

// Returns options for the AudioSource.
// (for some of the settings this approach is broken, e.g. setting
// audio network adaptation on the source is the wrong layer of abstraction).
const cricket::AudioOptions DesktopAudioSource::options() const {
  printf("WASM7\n");
  return cricket::AudioOptions();
}

void DesktopAudioSource::RegisterObserver(webrtc::ObserverInterface* observer) {
  printf("WASM8\n");
};
void DesktopAudioSource::UnregisterObserver(
    webrtc::ObserverInterface* observer) {
  printf("WASM9\n");
};

webrtc::MediaSourceInterface::SourceState DesktopAudioSource::state() const {
  printf("WASM10\n");
  return SourceState::kLive;
}

bool DesktopAudioSource::remote() const {
  printf("WASM11\n");
  return true;
}
