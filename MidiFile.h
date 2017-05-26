#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include <SD.h>

#define CHUNK_SIZE 50

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

  // BlockPointers pointers [];

  Note notes[10];
  // Note tempNotes[2 * CHUNK_SIZE];
  File midi;
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
    
    for ( int i = 0; i < 10; i++) {
      Serial.print(notes[i].pos); Serial.print(' ');
      Serial.print(notes[i].len); Serial.print(' ');
      Serial.print(notes[i].key); Serial.println();
    }
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
    midi = SD.open(filename);
    Serial.println("Opened midi");
    
    byte buff[4];
    uint32_u length;
    midi.read(buff, 4);
    Serial.println((char*)buff);
    
    midi.read(buff,4);
    length.bytes[0] = buff[3];
    length.bytes[1] = buff[2];
    length.bytes[2] = buff[1];
    length.bytes[3] = buff[0];
    Serial.println(length.data);
    
    midi.seek(midi.position() + length.data);
    
    Serial.println(midi.position());
    
    midi.read(buff, 4);
    Serial.println((char*)buff);
    
    midi.read(buff,4);
    length.bytes[0] = buff[3];
    length.bytes[1] = buff[2];
    length.bytes[2] = buff[1];
    length.bytes[3] = buff[0];
    Serial.println(length.data);
    // length = (int) buff;
    // Serial.println(length);
    // Opens the midi file and reads all the important information about it.

    // Loop through chunks (first pass)

      // Scan chunk for tempo information

      // Count chunks with actual music

    // Loop through chunks (second pass)

      // Ignore chunks without music

      // Only record key down 0x09 and key up 0x08 events

      // Every CHUNK_SIZE notes, record the pointer and the start time

    // pointers
  }

  int getNextVarInt( File f ) {
    // Do integer wrangling here and progress the file
    return 0;
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
