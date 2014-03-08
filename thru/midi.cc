#include "midi.h"

#include "log.h"
#include "midi_message.h"

#include <atomic>
#include <chrono>
#include <vector>
#include <future>

using namespace thru;

namespace {
  HMIDIIN g_input;
  HMIDIOUT g_output;
  const int g_outputChannel = 1;
  const int g_lowerMuteKey = 29;
  const int g_upperMuteKey = 35;

  const int g_recordPlayKey = 29;
  bool g_recording = false;
  std::atomic<bool> g_stop{ true };
  std::vector<std::pair<MidiMessage, DWORD>> g_recordedMessages;

  void logMessageType(MessageType type) {
    switch (type) {
    case MessageType::NoteOff:
      log() << "Off  | ";
      break;
    case MessageType::NoteOn:
      log() << "On   | ";
      break;
    case MessageType::ControlChange:
      log() << "CCh  | ";
      break;
    case MessageType::ProgramChange:
      log() << "PCh  | ";
      break;
    default:
      log() << "X    | ";
      break;
    }
  }

  bool isSilentMessage(MidiMessage message) {
    if (message.type() != MessageType::NoteOn && message.type() != MessageType::NoteOff)
      return false;

    return message.key() >= g_lowerMuteKey && message.key() <= g_upperMuteKey;
  }

  void logMessage(MidiMessage message, DWORD timing) {
    if (g_recording)
      log() << "[R] ";
    else if (!g_stop)
      log() << "[>] ";
    else
      log() << "    ";

    auto channel = message.channel();
    log() << channel;
    if (channel < 10)
      log() << "  | ";
    else
      log() << " | ";

    logMessageType(message.type());

    auto key = message.key();
    if (key == -1)
      log() << "    | ";
    else {
      log() << key;
      if (key < 10)
        log() << "   | ";
      else if (key < 100)
        log() << "  | ";
      else
        log() << " | ";
    }

    auto vel = message.velocity();
    if (vel == -1)
      log() << "    ";
    else {
      log() << vel;
      if (vel < 10)
        log() << "   ";
      else if (vel < 100)
        log() << "  ";
      else
        log() << " ";
    }

    log() << "             " << timing << "\n";
  }

  void playRecorded(const std::vector<std::pair<MidiMessage, DWORD>>& recorded) {
    if (recorded.empty())
      return;

    using clock = std::chrono::high_resolution_clock;

    while (!g_stop) {
      auto firstTime = recorded[0].second;
      auto firstLocalTime = clock::now();

      for (const auto& m : recorded) {
        while (std::chrono::milliseconds{ m.second - firstTime } > clock::now() - firstLocalTime) {
          // Wait
          if (g_stop)
            return;
        }

        if (m.second == recorded.back().second)
          // This was the finish recording event, loop back to the start
          break;

        midiOutShortMsg(g_output, m.first.m_data);
      }
    }
  }

  void handleInData(MidiMessage message, DWORD timing) {
    if (isSilentMessage(message))
      log() << "Silence!\n";
    else {
      auto outMessage = message;
      outMessage.channel(g_outputChannel);
      midiOutShortMsg(g_output, outMessage.m_data);
    }

    if (message.key() == g_recordPlayKey && message.type() == MessageType::NoteOn) {
      if (g_recording) {
        g_recording = false;
        g_stop = false;

        // We place the last key press in the recording so we get the loop timing correct
        MidiMessage msg{ message };
        msg.channel(g_outputChannel);
        g_recordedMessages.emplace_back(msg, timing);

        auto recordedCopy = g_recordedMessages;
        std::async([recordedCopy] {
          playRecorded(recordedCopy);
        });
      }
      else if (!g_stop) {
        g_stop = true;
      }
      else {
        g_recording = true;
        g_recordedMessages.clear();
      }
    }

    if (g_recording && !isSilentMessage(message)) {
      MidiMessage msg{ message };
      msg.channel(g_outputChannel);
      g_recordedMessages.emplace_back(msg, timing);
    }

    logMessage(message, timing);
  }

  void CALLBACK onInput(HMIDIIN, UINT uMsg, DWORD, DWORD dwParam1, DWORD dwParam2) {
    switch (uMsg) {
    case MIM_OPEN:
      log() << "Opened input device\n";
      break;
    case MIM_CLOSE:
      log() << "Closed input device\n";
      break;
    case MIM_DATA:
      handleInData(MidiMessage(dwParam1), dwParam2);
      break;
    }
  }

}

namespace thru {

  std::vector<MIDIINCAPS> getInputDevices() {
    auto total = midiInGetNumDevs();
    std::vector<MIDIINCAPS> result(total);
    for (decltype(total) i = 0; i < total; ++i)
      midiInGetDevCaps(i, &result[i], sizeof(MIDIINCAPS));
    return result;
  }

  std::vector<MIDIOUTCAPS> getOutputDevices() {
    auto total = midiOutGetNumDevs();
    std::vector<MIDIOUTCAPS> result(total);
    for (decltype(total) i = 0; i < total; ++i)
      midiOutGetDevCaps(i, &result[i], sizeof(MIDIOUTCAPS));
    return result;
  }

  void openInputDevice(int inputDeviceId) {
    const auto flags = CALLBACK_FUNCTION | MIDI_IO_STATUS;
    const auto callback = reinterpret_cast<DWORD_PTR>(onInput);
    auto result = midiInOpen(&g_input, inputDeviceId, callback, 0, flags);
    if (result != MMSYSERR_NOERROR)
      log() << "Failed to open input device\n";
  }

  void openOutputDevice(int outputDeviceId) {
    auto result = midiOutOpen(&g_output, outputDeviceId, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR)
      log() << "Failed to open output device\n";
    else
      log() << "Opened output device, will use channel " << g_outputChannel << "\n";
  }

  void runMidi() {
    g_recordedMessages.reserve(64000);

    midiInStart(g_input);
    log() << "Started midi ...\n\n";
    log() << "    Ch | Type | Key | Vel\n";
    log() << "    =====================\n";
  }

}