#include "system_audio_module.h"

  AudioSourceInfo::AudioSourceInfo(int id, std::string title) : id(id), title(title) {}
  int AudioSourceInfo::GetId() const {return id;}
  std::string AudioSourceInfo::GetTitle() const {return title;}