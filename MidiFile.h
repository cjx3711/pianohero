#ifndef _MIDIFILE_H
#define _MIDIFILE_H_

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


struct BlockPointers {
  int currentBlock;
  int maxBlocks;
  // All the block pointers for a given midi channel;
  // Stores an array
  int * position; // Array of positions
  int * time; // Array of times
}

class MidiFile {
  // Metadata
  int microseconds; // Microseconds per quarter note
  
  BlockPointers pointers [];
  
  Note tempNotes[2 * CHUNK_SIZE];
  void openFile(char * filename) {
    // Opens the midi file and reads all the important information about it.
  
    // Loop through chunks (first pass)
    
      // Scan chunk for tempo information
      
      // Count chunks with actual music
    
    // Loop through chunks (second pass)
    
      // Ignore chunks without music
      
      // Only record key down 0x09 and key up 0x08 events
      
      // Every CHUNK_SIZE notes, record the pointer and the start time
      
    pointers 
  }
  
  int getNextVarInt( File f ) {
    // Do integer wrangling here and progress the file
  }
  
  
}



#endif