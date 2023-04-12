#include "api/media_stream_interface.h"
#include <mutex>
#include "custom_audio.h"

class DesktopAudioSource : public AudioSource {
    public:
    DesktopAudioSource();
};