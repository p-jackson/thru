#ifndef THRU_MIDI_MESSAGE_H
#define THRU_MIDI_MESSAGE_H

namespace thru {

  enum class MessageType {
    NoteOn,
    NoteOff,
    ControlChange,
    ProgramChange,

    Unknown
  };

  class MidiMessage {
  public:
    unsigned long m_data;

    explicit MidiMessage(unsigned long d) : m_data{ d } {}

    int channel() {
      return (lobyte(loword(m_data)) & 0xf) + 1;
    }

    void channel(int c) {
      m_data &= 0xfffffff0;
      m_data |= (c - 1) & 0xf;
    }

    MessageType type() {
      switch ((lobyte(loword(m_data)) & 0xf0) >> 4) {
      case 8: return MessageType::NoteOff;
      case 9: return MessageType::NoteOn;
      case 11: return MessageType::ControlChange;
      case 12: return MessageType::ProgramChange;
      default: return MessageType::Unknown;
      }
    }

    int key() {
      auto t = type();
      if (t == MessageType::NoteOn || t == MessageType::NoteOff)
        return firstDataByte();
      else
        return -1;
    }

    int control() {
      if (type() == MessageType::ControlChange)
        return firstDataByte();
      else
        return -1;
    }

    int velocity() {
      auto t = type();
      if (t == MessageType::NoteOn || t == MessageType::NoteOff || t == MessageType::ControlChange)
        return secondDataByte();
      else
        return -1;
    }

  private:
    int firstDataByte() {
      return (m_data >> 8) & 0x7f;
    }

    int secondDataByte() {
      return (m_data >> 16) & 0x7f;
    }

    static unsigned long loword(unsigned long l) {
      return l & 0xffff;
    }

    static unsigned long hiword(unsigned long l) {
      return (l >> 16) & 0xffff;
    }

    static unsigned long lobyte(unsigned long l) {
      return l & 0xff;
    }

    static unsigned long hibyte(unsigned long l) {
      return (l >> 8) && 0xff;
    }
  };

}

#endif