#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include <SD.h>

#define CHUNK_SIZE 50

// The number representations of the ascii code for
// 'MThd' and 'MTrk'
#define MTHD 1297377380
#define MTRK 1297379947

// #define MIDI_DEBUG

struct Note {
  int pos;
  uint8_t len;
  uint8_t key;
  Note() {
    pos = 0;
    len = 2;
    key = 0;
  }
  Note(int _pos, char _len, char _key) {
    pos = _pos;
    len = _len;
    key = _key;
  }
};

union uint32_u {
  uint32_t data;
  byte bytes[4];
};

union uint16_u {
  uint16_t data;
  byte bytes[2];
};

// struct BlockPointers {
//   int currentBlock;
//   int maxBlocks;
//   // All the block pointers for a given midi channel;
//   // Stores an array
//   int * position; // Array of positions
//   int * time; // Array of times
// };

class MidiFile {
public:
  // Metadata
  int microseconds; // Microseconds per quarter note
  uint16_t format, tracks, division;
  // BlockPointers pointers [];

  Note notes[10];
  // Note tempNotes[2 * CHUNK_SIZE];
  File midiFile;
  void init() {
    // Temp song
    notes[0].key = 4;
    notes[1].key = 2;
    notes[2].key = 0;
    notes[3].key = 2;
    notes[4].key = 4;
    notes[5].key = 4;
    notes[6].key = 4;
    notes[7].key = 2;
    notes[8].key = 2;
    notes[9].key = 2;
    for ( int i = 0; i < 10; i++) {
      notes[i].pos = i*2;
    }
    
    notes[7].pos = 14 + 2;
    notes[8].pos = 16 + 2;
    notes[9].pos = 18 + 2;
    
    #ifdef MIDI_DEBUG
      for ( int i = 0; i < 10; i++) {
        Serial.print(notes[i].pos); Serial.print(' ');
        Serial.print(notes[i].len); Serial.print(' ');
        Serial.print(notes[i].key); Serial.println();
      }
    #endif
  }
  
  Note getNote(int i) {
    // TODO: Do some hot swapping stuff
    return notes[i];
  }
  void openFile(char * filename) {
    Serial.println("-------");
    if ( !SD.exists(filename) ) {
      Serial.println("Missing midi");
      return;
    }
    midiFile = SD.open(filename);
    Serial.println("Opened midi");
    
    // Opens the mififile and reads all the important information about it.

    // Loop through chunks (first pass)
    while(midiFile.position() < midiFile.size()) {
      #ifdef MIDI_DEBUG
        Serial.print("Chunk Pos: ");
        Serial.print(midiFile.position());
      #endif
      uint32_t header = readInt32(midiFile);
      switch( header ) {
        case MTHD:
          Serial.println(" - Header");
          readHeader(midiFile); // Scan chunk for tempo information
        break;
        case MTRK:
          Serial.println(" - Track");
          readTrack(midiFile); // Count chunks with actual music
        break;
      }
    }

    // Loop through chunks (second pass)

      // Ignore chunks without music

      // Only record key down 0x09 and key up 0x08 events

      // Every CHUNK_SIZE notes, record the pointer and the start time

    // pointers
  }
  
  void readHeader(File &f) {
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    format = readInt16(f);
    tracks = readInt16(f);
    division = readInt16(f);
    #ifdef MIDI_DEBUG
      Serial.print("Fmt: "); Serial.println(format);
      Serial.print("Trk: "); Serial.println(tracks);
      Serial.print("Div: "); Serial.println(division);
    #endif
    f.seek(end); // This should not be required, but just in case
  }
  
  /**
   * Checks if a track has midi data
   */
  bool checkTrackForMidi(File &f) {
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    uint8_t size = 0; // Size used for midi run on messages// Do track reading stuff here
    bool hasMidi = false;
    while(f.position() < end) {
      uint32_t delta = readIntMidi(f);
      uint8_t type = readInt8(f);
      switch(type) {
        case 0xFF: // Meta event
          uint8_t metaType = readInt8(f);
          uint32_t length = readIntMidi(f);
          f.seek(f.position() + length);
          break;
        case 0xF0: // System Exclusive event
        case 0xF7:
          uint32_t length = readIntMidi(f);
          // Ignore sysex events, we're not dealing with it here.
          f.seek(f.position() + length);
          break;
        case 0xc0 ... 0xdf:	// MIDI message with 1 parameter
          size = 2;
          readInt8(f);
          break;
        case 0x80 ... 0xBf:	// MIDI message with 2 parameters
	      case 0xe0 ... 0xef:
          size = 3;
          readInt8(f); readInt8(f);
          if ( type >> 4 == 8 || type >> 4 == 9 ) {
            hasMidi = true;
          }
          break;
        case 0x00 ... 0x7f:	// MIDI run on message
          for (uint8_t i = 2; i < size; i++) {
            readInt8(f);	// Read next byte and dispose
          }	
          break;
      }
      if ( hasMidi ) break;
    }
    return hasMidi;
  }
  
  void readTrack(File &f) {
    // https://github.com/5shekel/midi.player/blob/master/libs/MD_MIDIFile/MD_MIDITrack.cpp
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    uint8_t size = 0; // Size used for midi run on messages
    #ifdef MIDI_DEBUG
      Serial.print("Chunk len: "); Serial.println(length);
    #endif
    // Do track reading stuff here
    while(f.position() < end) {
      uint32_t delta = readIntMidi(f);
      uint8_t type = readInt8(f);
      
      // Serial.print("d"); Serial.print(delta);
      // Serial.print(' ');
      // printBits(type, 8);
      
      switch(type) {
        case 0xFF: // Meta event
          readMeta(f);
          break;
        case 0xF0: // System Exclusive event
        case 0xF7:
          readSysex(f);
          break;
        case 0x80 ... 0xBf:	// MIDI message with 2 parameters
	      case 0xe0 ... 0xef:
          size = 3;
          readMidi(f, type, size);
          break;
        case 0xc0 ... 0xdf:	// MIDI message with 1 parameter
          size = 2;
          readMidi(f, type, size);
          break;
        case 0x00 ... 0x7f:	// MIDI run on message
          for (uint8_t i = 2; i < size; i++) {
            readInt8(f);	// Read next byte and dispose
          }	
          break;
      }
    }
    f.seek(end);
  }
  
  void readMeta(File &f) {
    #ifdef MIDI_DEBUG
      Serial.println(" - Meta");
    #endif
    uint8_t metaType = readInt8(f);
    uint32_t length = readIntMidi(f);
    
    switch ( metaType ) {
      case 0x51: // Tempo event
        Serial.println("Tempo event");
      break;
      case 0x03: // Text event
        Serial.println("Text event");
        char * text = (char*)malloc(length);
        f.read(text, length);
        Serial.print('\t')
        Serial.println(text);
      break;
    }
    f.seek(f.position() + length);
  }
  
  void readSysex(File &f) {
    #ifdef MIDI_DEBUG
      Serial.println(" - Sysex");
    #endif
    uint32_t length = readIntMidi(f);
    // Ignore sysex events, we're not dealing with it here.
    f.seek(f.position() + length);
  }
  
  void readMidi(File &f, uint8_t type, uint8_t size) {
    #ifdef MIDI_DEBUG
      Serial.print(" - Midi ");
    #endif
    uint8_t first4 = type >> 4; 
    uint8_t data1 = size > 1 ? readInt8(f) : 0;
    uint8_t data2 = size > 2 ? readInt8(f) : 0;
    
    #ifdef MIDI_DEBUG
      Serial.println(first4);
    #endif
    
    // There are lots of other event s, but this program will ignore it
    if ( first4 == 8 || first4 == 9 ) {
      uint8_t channel = type & 15; // 00001111
      Serial.print(channel); Serial.print(" ");
      Serial.print(data1); Serial.print(" ");
      if ( first4 == 8 ) {
        Serial.println("Up");
      } else {
        Serial.println("Down");
      }
    }
  }
private:
  
  // uint16_t flip16(uint16_t n) {
  //   uint16_t flip;
  //   flip.bytes[0] = bitRead(n, 0);
  //   flip.bytes[1] = bitRead(n, 1);
  // }
  // 
  // uint16_t flip32(uint32_t n) {
  //   uint32_t flip;
  //   flip.bytes[0] = bitRead(n, 0);
  //   flip.bytes[1] = bitRead(n, 1);
  //   flip.bytes[2] = bitRead(n, 2);
  //   flip.bytes[3] = bitRead(n, 3);
  // }

  void printBits(uint8_t b, uint8_t n) {
    for ( int i = 0; i < n; i++ ) {
      Serial.print(bitRead(b, 7-i));
    }
    Serial.println("b");
  }
  
  uint32_t readInt32(File &f) {
    uint32_u ret;
    byte buffer[4];
    f.read(buffer,4);
    ret.bytes[0] = buffer[3];
    ret.bytes[1] = buffer[2];
    ret.bytes[2] = buffer[1];
    ret.bytes[3] = buffer[0];
    return ret.data;
  }
  uint16_t readInt16(File &f) {
    uint16_u ret;
    byte buffer[2];
    f.read(buffer,2);
    ret.bytes[0] = buffer[1];
    ret.bytes[1] = buffer[0];
    return ret.data;
  }
  uint8_t readInt8(File &f) {
    return f.read();
  }
  uint32_t readIntMidi(File &f) {
    // Do integer wrangling here and progress the file
    byte buffer[4];
    uint32_t read = 0;
    uint8_t byteCount = 0; // Byte count
    // Count the bytes
    for ( uint8_t i = 0 ; i < 4; i++ ) {
      buffer[i] = f.read();
      bool bit = bitRead(buffer[i], 7); // Read the leftmost bit
      byteCount++;
      if ( !bit ) break;
    }
    // Get the number from the back
    uint8_t pos = 0;
    for ( int8_t i = byteCount - 1; i >= 0; i-- ) {
      for (uint8_t j = 0; j < 7; j++) {
        bitWrite(read, pos, bitRead(buffer[i], j));
        pos++;
      }
    }
    return read;
  }
  
  void reverse_array( byte array[], int arraylength ) {
    for (int i = 0; i < (arraylength / 2); i++) {
        byte temporary = array[i];                 // temporary wasn't declared
        array[i] = array[(arraylength - 1) - i];
        array[(arraylength - 1) - i] = temporary;
    }
}


};



#endif
