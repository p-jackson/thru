#include "log.h"

#include "midi.h"

#include <algorithm>
#include <iostream>

using namespace thru;

int main() {
  const auto numInputDevs = midiInGetNumDevs();
  log() << numInputDevs << " midi input devices found.\n";
  if (!numInputDevs)
    return 0;

  const auto inputDevices = getInputDevices();
  for (auto& inputDevice : inputDevices)
    log() << " - " << inputDevice.szPname << "\n";

  const auto inputDeviceId = 0;

  const auto outputDevices = getOutputDevices();
  auto outputDeviceId = -1;
  for (auto i = 0U; i < outputDevices.size(); ++i) {
    if (inputDevices[0].wMid != outputDevices[i].wMid || inputDevices[0].wPid != outputDevices[i].wPid)
      continue;

    outputDeviceId = i;
    break;
  }

  if (outputDeviceId == -1)
    return 0;

  log() << "Found matching output device\n";

  openInputDevice(inputDeviceId);
  openOutputDevice(outputDeviceId);

  runMidi();

  std::getchar();
}
