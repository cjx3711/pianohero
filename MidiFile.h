#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include <SD.h>

#define BLOCK_SIZE 6

// The number representations of the ascii code for
// 'MThd' and 'MTrk'
#define MTHD 1297377380
#define MTRK 1297379947

#define MAX32 4294967295

// https://github.com/5shekel/midi.player/blob/master/libs/MD_MIDIFile/MD_MIDITrack.cpp

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

struct BlockPointers {
  uint16_t currentBlock;
  uint16_t maxBlocks;
  // All the block pointers for a given midi channel;
  // Stores an array
  uint32_t * position; // Array of positions
  uint32_t * time; // Array of times
};

class MidiFile {
public:
  // Metadata
  int microseconds; // Microseconds per quarter note
  uint16_t format, tracks, division;
  BlockPointers * trackBlocks;
  uint8_t trackCount;

  Note notes[10];
  // Note tempNotes[2 * BLOCK_SIZE];
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

    
    trackCount = 0;
    // Loop through chunks (first pass) - Get the header data, count the tracks with music in it.
    // Gets the header information and counts the number of tracks with midi information
    while(midiFile.position() < midiFile.size()) {
      #ifdef MIDI_DEBUG
        Serial.print("Chunk Pos: ");
        Serial.print(midiFile.position());
      #endif
      uint32_t header = readInt32(midiFile);
      switch( header ) {
        case MTHD: {
          readHeader(midiFile); // Scan chunk for tempo information
          break;
        }
        case MTRK: {
          uint32_t pos = midiFile.position();
          if ( countNotesInTrack(midiFile, true) ) { // Count chunks with actual music
            trackCount++;
          } else {
            midiFile.seek(pos);
            readMetaTrack(midiFile); // Get all the metadata out of the track
          }
          break;
        }
      }
    }
    
    
    uint16_t noteCounts[trackCount]; // This stores the notes per track.
    uint32_t trackLocations[trackCount]; // This stores the locations of the tracks
    midiFile.seek(0);
    
    // Loop through chunks (second pass)
    //    Count the number of notes in each track.
    uint8_t noteCountIndex = 0;
    while(midiFile.position() < midiFile.size()) {
      uint32_t header = readInt32(midiFile);
      switch( header ) {
        case MTRK: {
          uint32_t pos = midiFile.position();
          uint32_t noteCount = countNotesInTrack(midiFile);
          if ( noteCount > 0 ) {
            noteCounts[noteCountIndex] = noteCount;
            trackLocations[noteCountIndex] = pos;
            noteCountIndex++;
          }
          break;
        }
        case MTHD: {
          skipHeader(midiFile); // Scan chunk for tempo information
          break;
        }
      }
      if ( noteCountIndex >= trackCount ) break;
    }
    
    Serial.println("Note count / position:");
    for ( uint8_t i = 0; i < trackCount; i++ ) {
      Serial.print("   "); Serial.print(noteCounts[i]); Serial.print('\t'); Serial.println(trackLocations[i]); Serial.println();
    }
    
    // Creating the block arrays
    uint8_t blockIndex = 0;
    trackBlocks = (BlockPointers*) malloc(sizeof(BlockPointers) * trackCount);
    for ( uint8_t i = 0; i < trackCount; i++ ) {
      trackBlocks[i].currentBlock = 0;
      trackBlocks[i].maxBlocks = (noteCounts[i] / BLOCK_SIZE) + 1;
      trackBlocks[i].position = (uint32_t*) malloc(sizeof(uint32_t*) * trackBlocks[i].maxBlocks);
      trackBlocks[i].time = (uint32_t*) malloc(sizeof(uint32_t*) * trackBlocks[i].maxBlocks);
    }
    midiFile.seek(0);
    
    // Loop through chunks with music (third pass)
    for ( uint8_t i = 0; i < trackCount; i++ ) {
      midiFile.seek(trackLocations[i]);
      Serial.print("Seeking: "); Serial.println(midiFile.position());
      getBlockPositionsInTrack(midiFile, i);
      
      for ( uint16_t j = 0; j < trackBlocks[i].maxBlocks; j++ ) {
        Serial.print("File: ");
        Serial.print(trackBlocks[i].position[j]);
        Serial.print(" Time: ");
        Serial.println(trackBlocks[i].time[j]);
      }
    }
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
  void skipHeader(File &f) {
    uint32_t length = readInt32(f);
    f.seek(f.position() + length);
  }
  
  /**
   * Counts the number of notes in a midi track.
   * If the second param is set to true, it will break once a
   * note is detected, returning 1.
   */
  uint32_t countNotesInTrack(File &f, bool breakOnFirst = false) {
    Serial.print(f.position());
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    uint8_t size = 0; // Size used for midi run on messages
    uint32_t noteCount = 0;
    while(f.position() < end) {
      uint32_t delta = readIntMidi(f);
      uint8_t type = readInt8(f);
      switch(type) {
        case 0xFF: // Meta event
          readInt8(f); f.seek(f.position() + readIntMidi(f));  break; // Read a meta type then skip by the length
        case 0xF0: // System Exclusive event
        case 0xF7: 
          f.seek(f.position() + readIntMidi(f)); break; // Ignore sysex events, we're not dealing with it here.
        case 0xc0 ... 0xdf: // MIDI message with 1 parameter
          size = 2; readInt8(f); break;
        case 0x00 ... 0x7f: // MIDI run on message
          for (uint8_t i = 2; i < size; i++) readInt8(f); break;// Read next byte and dispose
        case 0x80 ... 0xBf: // MIDI message with 2 parameters
        case 0xe0 ... 0xef:
          size = 3;
          readInt8(f); readInt8(f);
          if ( type >> 4 == 9 ) noteCount++; // This is a key press event. We only need to count keypresses.
          break;
      }
      if ( breakOnFirst && noteCount > 0 ) break; // Once we find a single note, we can break out of the loop
    }
    f.seek(end);
    return noteCount;
  }
  
  /**
   * Reads the metadata from a meta track
   */
  void readMetaTrack(File &f) {
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    uint8_t size = 0; // Size used for midi run on messages
    
    // Do track reading stuff here
    while(f.position() < end) {
      uint32_t delta = readIntMidi(f);
      uint8_t type = readInt8(f);
      switch(type) {
        case 0xFF: // Meta event
          readMeta(f); break;
        case 0xF0: // System Exclusive event
        case 0xF7: 
          f.seek(f.position() + readIntMidi(f)); break; // Ignore sysex events, we're not dealing with it here.
        case 0xc0 ... 0xdf: // MIDI message with 1 parameter
          size = 2; readInt8(f); break;
        case 0x00 ... 0x7f: // MIDI run on message
          for (uint8_t i = 2; i < size; i++) readInt8(f); break;// Read next byte and dispose
        case 0x80 ... 0xBf: // MIDI message with 2 parameters
        case 0xe0 ... 0xef:
          size = 3; readInt8(f); readInt8(f);
          break;
      }
    }
    f.seek(end);
  }
  
  /**
   * Reads the midi notes to determine where the start of the blocks are
   * in a single track.
   */
  void getBlockPositionsInTrack(File &f, uint8_t blockNumber) {
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    uint8_t size = 0; // Size used for midi run on messages
    
    uint16_t currentCount = 0;
    uint16_t currentTime = 0;
    
    // Do track reading stuff here
    while(f.position() < end) {
      uint32_t pos = f.position();
      uint32_t delta = readIntMidi(f);
      uint8_t type = readInt8(f);
      switch(type) {
        case 0xFF: // Meta event
          readMeta(f); break;
        case 0xF0: // System Exclusive event
        case 0xF7:
          skipSysex(f); break;
        case 0x80 ... 0xBf:	// MIDI message with 2 parameters
	      case 0xe0 ... 0xef: {
          size = 3;
          uint8_t key = readInt8(f) - 9; readInt8(f);
          uint8_t midiType = type >> 4;
          if ( midiType == 9 ) { // When it detects a keydown, increment the count
            currentTime += delta;
            if ( currentCount == 0 ) {
              // Add position and time to the block
              trackBlocks[blockNumber].position[trackBlocks[blockNumber].currentBlock] = pos;
              trackBlocks[blockNumber].time[trackBlocks[blockNumber].currentBlock] = currentTime;
              trackBlocks[blockNumber].currentBlock++;
            }
            currentCount++;
            if ( currentCount >= BLOCK_SIZE ) {
              currentCount = 0;
            }
          } else if ( midiType == 8 ) {
            currentTime += delta;
          }
          break;
        }
        case 0xc0 ... 0xdf:	// MIDI message with 1 parameter
          size = 2; readInt8(f); break;
        case 0x00 ... 0x7f:	// MIDI run on message
          for (uint8_t i = 2; i < size; i++) readInt8(f); break;// Read next byte and dispose
      }
    }
    currentCount = 0;
    trackBlocks[blockNumber].currentBlock = 0;
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
        uint32_t tempo = readInt24(f);
      break;
      
      case 0x51: // Tempo event
        Serial.println("Time Signature");
        uint32_t timeSNum = readInt8(f);
        uint32_t timeSDen = readInt8(f);
        uint32_t clocksPerTick = readInt8(f);
        uint32_t notesPer24Clocks = readInt8(f);
      break;
      
      case 0x03: // Text event
        Serial.println("Text event");
        char * text = (char*)malloc(length);
        f.read(text, length);
        Serial.print('\t');
        Serial.println(text);
      break;
    }
    f.seek(f.position() + length);
  }
  
  void skipSysex(File &f) {
    // Ignore sysex events, we're not dealing with it here.
    f.seek(f.position() + readIntMidi(f));
  }
  
  /**
   * Reads the midi chunk. Size is assumed to be 3
   */
  // void read3Midi(File &f, uint8_t type) {
  //   #ifdef MIDI_DEBUG
  //     Serial.print(" - Midi ");
  //   #endif
  //   uint8_t first4 = type >> 4; 
  //   uint8_t data1 = readInt8(f); // Key (for 0x8 and 0x9 events)
  //   uint8_t data2 = readInt8(f); // Valocity (for 0x8 and 0x9 events)
  //   
  //   #ifdef MIDI_DEBUG
  //     Serial.println(first4);
  //   #endif
  //   
  //   // There are lots of other event s, but this program will ignore it
  //   if ( first4 == 9 ) { // When it detects a keydown, increment the count
  //     
  //   }
  //   
  //   // if ( first4 == 8 || first4 == 9 ) {
  //   //   uint8_t channel = type & 15; // 00001111
  //   //   Serial.print(channel); Serial.print(" ");
  //   //   Serial.print(data1); Serial.print(" ");
  //   //   if ( first4 == 8 ) {
  //   //     Serial.println("Up");
  //   //   } else {
  //   //     Serial.println("Down");
  //   //   }
  //   // }
  // }
private:
  
  
  void printBits(uint8_t b, uint8_t n) {
    for ( int i = 0; i < n; i++ ) {
      Serial.print(bitRead(b, 7-i));
    }
    Serial.println("b");
  }
  
  // ============== Functions to read data from the file ==========
  uint32_t readInt32(File &f) { // Reads an int32 
    uint32_u ret;
    byte buffer[4];
    f.read(buffer,4);
    ret.bytes[0] = buffer[3];
    ret.bytes[1] = buffer[2];
    ret.bytes[2] = buffer[1];
    ret.bytes[3] = buffer[0];
    return ret.data;
  }
  uint32_t readInt24(File &f) { // Reads 3 bytes into an int32 
    uint32_u ret;
    byte buffer[3];
    f.read(buffer,3);
    ret.bytes[0] = buffer[2];
    ret.bytes[1] = buffer[1];
    ret.bytes[2] = buffer[0];
    ret.bytes[3] = 0;
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
