#ifndef THRU_MIDI_H
#define THRU_MIDI_H

#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

#include <vector>

namespace thru {

  std::vector<MIDIINCAPS> getInputDevices();
  std::vector<MIDIOUTCAPS> getOutputDevices();

  void openInputDevice(int inputDeviceId);
  void openOutputDevice(int outputDeviceId);

  void runMidi();
}

#endif