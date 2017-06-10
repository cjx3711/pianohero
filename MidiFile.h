#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include <SD.h>

#define BLOCK_SIZE 50
#define MAX_BLOCKS 200

// Notes that can be displayed on screen
#define KEYS 5
#define PIXELS_PER_KEY 24
#define PIXELS KEYS * PIXELS_PER_KEY

// min (8 * 4 * x) + (8 * (10000/x))

// The number representations of the ascii code for
// 'MThd' and 'MTrk'
#define MTHD 1297377380
#define MTRK 1297379947

#define MAX32 4294967295
#define MAX16 65535

// https://github.com/5shekel/midi.player/blob/master/libs/MD_MIDIFile/MD_MIDITrack.cpp

// #define MIDI_DEBUG

struct Note {
  uint32_t pos;
  uint16_t len;
  uint8_t key;
  Note() {
    pos = 0;
    len = 2;
    key = 0;
  }
  Note(uint32_t _pos, uint16_t _len, char _key) {
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

struct FilePos {
  uint32_t position; // Position in file
  uint32_t time;
};

struct BlockPointers {
  uint16_t currentBlock;
  uint16_t maxBlocks;
  uint32_t maxFilePos;
  uint32_t noteCount;
  // All the block pointers for a given midi channel;
  // Stores an array
  FilePos filePos[MAX_BLOCKS];
};

struct Track {
  uint16_t currentlyLoaded = 0;
  Note notes[BLOCK_SIZE * 2];
};

class MidiFile {
public:
  uint8_t clocksPerTick;
  uint8_t notesPer24Clocks;
  uint32_t tempo; // Microseconds per quarter note
  uint8_t qNoteScreenSize = 4; // The height of a single quarter note
  uint32_t screenTop = 0;
  uint32_t screenBottom = 0;
  // Used for key counting
  uint16_t noteSequence[88]; // Stores the sequence number of each note
  uint32_t noteTimes[88]; // So we know which key has been pressed
  // Metadata
  // int microseconds; // Microseconds per quarter note
  uint32_t trackPosition = 0;
  uint32_t trackLength;
  uint16_t format, tracks, division;
  // Pointer to the 
  BlockPointers trackBlocks[2];
  Track trackBuffers[2];
  uint8_t trackCount;
  
  Note notes[10];
  // Note tempNotes[2 * BLOCK_SIZE];
  File midiFile;
  
  // The notes will be stored in two buffers.
  // When we reach the end of the first buffer, we will
  // fill the second buffer with the following notes.
  // When we reach the start of the first buffer, we will
  // make it the second buffer and fill the first buffer with the initial notes.
  bool bufferIndex = false;
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
  
  
  /**************************************
  *   FILE ACCESSING FUNCTIONS
  **************************************/
  Note getNote(int i) {
    // TODO: Do some hot swapping stuff
    return notes[i];
  }
  
  Note * getNoteInScreen(bool which, uint16_t i) {
    if ( !inScreen(which, i) ) {
      return NULL;
    }
    return getNote(which, i);
  }
  
  Note * getNote(bool which, uint16_t i) {
    if ( i < trackBuffers[which].currentlyLoaded * BLOCK_SIZE ) {
      return NULL; // Out of the buffer range
    } else if ( i >= getNoteCount(which) ) {
      return NULL;
    }
    i -= trackBuffers[which].currentlyLoaded * BLOCK_SIZE;
    return &trackBuffers[which].notes[i];
  }
  
  uint16_t getFirstInScreen(bool which) {
    for ( uint16_t i = 0 ; i < BLOCK_SIZE * 2; i++ ) {
      uint16_t I = i + trackBuffers[which].currentlyLoaded * BLOCK_SIZE;
      if ( inScreen(which, I) ) return I;
    }
    return MAX16;
  }
  
  uint32_t getNoteCount(bool which) {
    return trackBlocks[which].noteCount;
  }
  
  bool inScreen(bool which, uint16_t noteIndex) {
    Note * note = getNote(which, noteIndex);
    if ( !note ) return false; // Out of the buffer range
    
    if ( note->pos <= screenTop &&
         note->pos + note->len >= screenBottom ) {
           return true;
         }
    return false;
  }
  
  void calculateScreenBoundaries() {
    screenBottom = trackPosition;
    screenTop = trackPosition + ((float)PIXELS_PER_KEY / (float)qNoteScreenSize) * division;
    Serial.print("Screen Top: "); Serial.println(screenTop);
    Serial.print("Screen Btm: "); Serial.println(screenBottom);
  }
  
  void setPosition(float perc) {
    trackPosition = perc * trackLength;
    calculateScreenBoundaries();
    loadBlockLogic(0);
    loadBlockLogic(1);
    Serial.println();
    Serial.print("Pos: "); Serial.print(trackPosition); Serial.print('/'); Serial.println(trackLength);
    
    Serial.println("Track 1: ");
    Serial.print(getBlock(0) + 1); Serial.print('/'); Serial.println(trackBlocks[0].maxBlocks);
    Serial.println(getBlockFloat(0) + 1);
    Serial.println(trackBuffers[0].currentlyLoaded);
    Serial.println("Track 2: ");
    Serial.print(getBlock(1) + 1); Serial.print('/'); Serial.println(trackBlocks[1].maxBlocks);
    Serial.println(getBlockFloat(1) + 1);
    Serial.println(trackBuffers[1].currentlyLoaded);
    
    printBuffers(0);
    printBuffers(1);
  }
  bool allKeysReleased(uint32_t * keys) {
    for ( uint8_t i = 0; i < 88; i++ )
    if (keys[i] != MAX32)
    return false;
    return true;
  }
  
  void setLoadedBlocks(bool which, uint16_t block) {
    // Do actual block loading logic here
    if ( trackBuffers[which].currentlyLoaded < block ) { // Moving up
      if ( block - trackBuffers[which].currentlyLoaded == 1 ) { // Moving up one
        // Copy second half into first half
        memcpy(&trackBuffers[which].notes[0], &trackBuffers[which].notes[BLOCK_SIZE], sizeof(Note) * BLOCK_SIZE);
        
        // Load stuff for second half
        loadBlock(which, block + 1, &trackBuffers[which].notes[BLOCK_SIZE]);
      }
    } else if ( trackBuffers[which].currentlyLoaded > block ) { // Moving down
      if ( trackBuffers[which].currentlyLoaded - block == 1 ) { // Moving down one
        // Copy first half into second half
        memcpy(&trackBuffers[which].notes[BLOCK_SIZE], &trackBuffers[which].notes[0], sizeof(Note) * BLOCK_SIZE);
        
        // Load stuff for first half
        loadBlock(which, block, &trackBuffers[which].notes[0]);
      }
    }
    trackBuffers[which].currentlyLoaded = block;
  }
  
  void printBuffers(bool which) {
    Serial.print("Buffer "); Serial.print((int)which); Serial.println(" :");
    for ( uint16_t i = 0; i < BLOCK_SIZE * 2; i++ ) {
      if ( trackBuffers[which].currentlyLoaded * BLOCK_SIZE + i >= trackBlocks[which].noteCount ) break;
      Serial.print(i); Serial.print(' '); Serial.print(trackBuffers[which].currentlyLoaded * BLOCK_SIZE + i);
      Serial.print(' '); Serial.print(trackBuffers[which].notes[i].pos); Serial.print(' '); Serial.println(trackBuffers[which].notes[i].len);
    }
    Serial.println();
  }
  
  void loadBlock(bool which, uint16_t block, Note * dest) {
    uint16_t noteCount = 0;
    uint8_t size = 0; // Used for run on messages
    for ( uint8_t i = 0; i < 88; i++ ) noteTimes[i] = MAX32;
    
    Serial.print("Loading block "); Serial.print((int)which); Serial.print(' '); Serial.println(block);
    midiFile.seek(trackBlocks[which].filePos[block].position);
    uint32_t currentTime = trackBlocks[which].filePos[block].time;
    
    while((noteCount < BLOCK_SIZE || !allKeysReleased(&noteTimes[0])) && midiFile.position() < trackBlocks[which].maxFilePos) {
      uint32_t pos = midiFile.position();
      uint32_t delta = readIntMidi(midiFile); //
      uint8_t type = readInt8(midiFile);
      switch(type) {
        case 0xFF: // Meta event
        skipMeta(midiFile); break; // Skip meta track
        case 0xF0: // System Exclusive event
        case 0xF7: 
        skipSysex(midiFile); break; // Ignore sysex events, we're not dealing with it here.
        case 0xc0 ... 0xdf: // MIDI message with 1 parameter
        size = 2; readInt8(midiFile); break;
        case 0x00 ... 0x7f: // MIDI run on message
        for (uint8_t i = 2; i < size; i++) readInt8(midiFile); break;// Read next byte and dispose
        case 0x80 ... 0xBf: // MIDI message with 2 parameters
        case 0xe0 ... 0xef:
        size = 3;          
        uint8_t key = readInt8(midiFile) - 9; readInt8(midiFile); // Velocity, ignore
        uint8_t midiType = type >> 4;
        if ( midiType == 9 ) {
          currentTime += !noteCount ? 0 : delta; // Ignore the delta for the first note
          noteTimes[key] = currentTime;
          noteSequence[key] = noteCount;
          Serial.print("Down Pos: "); Serial.print(pos);
          Serial.print(" Sequence: "); Serial.print(noteSequence[key]);
          Serial.print(" Key: "); Serial.print(key); 
          Serial.print(" Time: "); Serial.println(currentTime);
          noteCount++; // This is a key press event. We only need to count keypresses.
        } else if ( midiType == 8 ) {
          currentTime += delta;
          Serial.print("Up   Pos: "); Serial.print(pos);
          Serial.print(" Sequence: "); Serial.print(noteSequence[key]);
          Serial.print(" Key: "); Serial.print(key);
          Serial.print(" Time: "); Serial.println(currentTime);
          
          // Add the note to the buffer
          if ( noteTimes[key] != MAX32 ) {
            dest[noteSequence[key]].pos = noteTimes[key];
            dest[noteSequence[key]].len = (uint16_t)(currentTime - noteTimes[key]);
            dest[noteSequence[key]].key = key + 9;
            noteTimes[key] = MAX32;
          }
        }
        break;
      }
    }
  }
  
  /**
  * Decides which blocks should be loaded into the buffer
  * e.g. 0 means 0 and 1 should be loaded, since the buffer has space for 2 blocks
  * Return range, 0 to maxBlocks - 2
  */
  void loadBlockLogic(bool which) {
    float loaded = trackBuffers[which].currentlyLoaded;
    float current = getBlockFloat(which);
    float relativePos = current - loaded;
    if ( relativePos >= 1.5 && loaded < trackBlocks[which].maxBlocks - 2) {
      setLoadedBlocks(which, trackBuffers[which].currentlyLoaded + 1 );
    } else if ( relativePos <= 0.5 && loaded > 0 ) {
      setLoadedBlocks(which, trackBuffers[which].currentlyLoaded - 1 );
    }
  }
  /**
  * Gets the block number with percentage to next block
  */
  float getBlockFloat(bool which) {
    uint16_t block = getBlock(which);
    uint32_t time1 = getTimeFromBlock(which, block);
    uint32_t time2 = getTimeFromBlock(which, block + 1);
    float timePart = trackPosition - time1;
    float timeDiff = time2 - time1;
    return (float)block + (timePart / timeDiff);
  }
  
  /**
  * Gets the time associated with each block
  * If the block is == maxBlocks, it returns the max time
  */
  uint32_t getTimeFromBlock(bool which, uint16_t block) {
    if ( block < trackBlocks[which].maxBlocks ) { // Less than max
      return trackBlocks[which].filePos[block].time;
    } else { // Max block
      return trackLength;
    }
  }
  
  /**
  * Gets the block number that should contain the data for the current trackPosition
  */
  uint16_t getBlock(bool which) {
    uint16_t whichBlock = 0;
    for ( uint16_t i = 0; i < trackBlocks[which].maxBlocks; i++ ) {
      uint16_t I = trackBlocks[which].maxBlocks - i - 1;
      // Serial.print(trackPosition); Serial.print('/'); Serial.println(trackBlocks[which].filePos[I].time);
      if ( trackPosition > trackBlocks[which].filePos[I].time ) {
        // Previous block
        whichBlock = I;
        break;
      }
    }
    return whichBlock;
  }
  
  /**************************************
  *   FILE OPENING FUNCTIONS
  **************************************/
  /**
  * Opens the midi file and loads the important information
  * as well as the block information
  */
  void openFile(char * filename) {
    Serial.println("-------");
    if ( !SD.exists(filename) ) {
      Serial.println("Missing midi");
      return;
    }
    midiFile = SD.open(filename);
    Serial.println("Opened midi");
    
    trackLength = 0;
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
    
    if ( trackCount > 2 ) {
      trackCount = 2;
      Serial.println("Too many tracks");
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
      trackBlocks[i].noteCount = noteCounts[i];
      Serial.print("   "); Serial.print(noteCounts[i]); Serial.print('\t'); Serial.println(trackLocations[i]); Serial.println();
    }
    
    // Creating the block arrays
    for ( uint8_t i = 0; i < trackCount; i++ ) {
      trackBlocks[i].currentBlock = 0;
      trackBlocks[i].maxBlocks = (noteCounts[i] / BLOCK_SIZE) + (noteCounts[i] % BLOCK_SIZE ? 1 : 0);
      Serial.print("Max: "); Serial.println(trackBlocks[i].maxBlocks);
      
      // Initalise the arrays
      for ( uint16_t j = 0; j < MAX_BLOCKS; j++ ) {
        trackBlocks[i].filePos[j].position = 0;
        trackBlocks[i].filePos[j].time = 0;
      }
    }
    midiFile.seek(0);
    
    // Loop through chunks with music (third pass)
    for ( uint8_t i = 0; i < trackCount; i++ ) {
      midiFile.seek(trackLocations[i]);
      Serial.print("Seeking: "); Serial.println(midiFile.position());
      getBlockPositionsInTrack(midiFile, i);
      
      Serial.print("Track "); Serial.println(i);
      
      for ( uint16_t j = 0; j < trackBlocks[i].maxBlocks; j++ ) {
        Serial.print("  File: ");
        Serial.print(trackBlocks[i].filePos[j].position);
        Serial.print(" Time: ");
        Serial.println(trackBlocks[i].filePos[j].time);
      }
      Serial.println();
    }
    
    for ( uint8_t i = 0; i < trackCount; i++ ) {
      if ( trackBlocks[i].maxBlocks >= 1 ) loadBlock(!!i, 0, &trackBuffers[i].notes[0]);
      if ( trackBlocks[i].maxBlocks >= 2 ) loadBlock(!!i, 1, &trackBuffers[i].notes[BLOCK_SIZE]);
    }
    
    
    Serial.println("Track opened");
    Serial.print("Tracks: "); Serial.println(trackCount);
    Serial.print("Length: "); Serial.println(trackLength);
    
    for ( uint8_t i = 0; i < trackCount; i++ ) {
      printBuffers(i);
    }
  }
  
  void readHeader(File &f) {
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    format = readInt16(f);
    tracks = readInt16(f);
    division = readInt16(f);
    Serial.print("Fmt: "); Serial.println(format);
    Serial.print("Trk: "); Serial.println(tracks);
    Serial.print("Div: "); Serial.println(division);
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
    uint32_t length = readInt32(f);
    uint32_t end = f.position() + length;
    uint8_t size = 0; // Size used for midi run on messages
    uint32_t noteCount = 0;
    while(f.position() < end) {
      readIntMidi(f); // uint32_t delta = 
      uint8_t type = readInt8(f);
      switch(type) {
        case 0xFF: // Meta event
        skipMeta(f); break; // Skip meta track
        case 0xF0: // System Exclusive event
        case 0xF7: 
        skipSysex(f); break; // Ignore sysex events, we're not dealing with it here.
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
      readIntMidi(f); // uint32_t delta =
      uint8_t type = readInt8(f);
      switch(type) {
        case 0xFF: // Meta event
        readMeta(f); break;
        case 0xF0: // System Exclusive event
        case 0xF7: 
        skipSysex(f); break; // Ignore sysex events, we're not dealing with it here.
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
    uint32_t currentTime = 0;
    
    // Do track reading stuff here
    while(f.position() < end) {
      uint32_t pos = f.position();
      uint32_t delta = readIntMidi(f);
      uint8_t type = readInt8(f);
      // Serial.print("POS: "); Serial.print(pos);  Serial.print(" D: "); Serial.print(delta); Serial.print(" type:"); Serial.println(type);
      switch(type) {
        case 0xFF: // Meta event
        readMeta(f); break;
        case 0xF0: // System Exclusive event
        case 0xF7:
        skipSysex(f); break;
        case 0x80 ... 0xBf:	// MIDI message with 2 parameters
        case 0xe0 ... 0xef: {
          size = 3;
          /* uint8_t key = */ readInt8(f) /*- 9*/; readInt8(f);
          uint8_t midiType = type >> 4;
          if ( midiType == 9 ) { // When it detects a keydown, increment the count
            currentTime += delta;
            // Serial.print("Pos: "); Serial.print(pos);  Serial.print(" Time: "); Serial.println(currentTime);
            if ( currentCount == 0 ) {
              // Serial.print("Add "); 
              uint16_t i = trackBlocks[blockNumber].currentBlock;
              // Serial.print("Block: "); Serial.print(i);
              // Add position and time to the block
              
              trackBlocks[blockNumber].filePos[i].position = pos;
              trackBlocks[blockNumber].filePos[i].time = currentTime;
              trackBlocks[blockNumber].currentBlock++;
              // Serial.print(" Pos: "); Serial.print(trackBlocks[blockNumber].filePos[i].position);  Serial.print(" Time: "); Serial.println(  trackBlocks[blockNumber].filePos[i].time );
              
            }
            currentCount++;
            if ( currentCount >= BLOCK_SIZE ) {
              currentCount = 0;
            }
          } else if ( midiType == 8 ) {
            // Serial.print("Release time: "); Serial.println(currentTime);
            currentTime += delta;
          }
          if ( midiType == 8 || midiType == 9 ) {
            if ( trackLength < currentTime ) trackLength = currentTime;
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
    trackBlocks[blockNumber].maxFilePos = end;
    f.seek(end);
  }
  
  void skipMeta(File &f) {
    readInt8(f);
    uint32_t length = readIntMidi(f);
    uint32_t end = f.position() + length;
    f.seek(end);
  }
  void readMeta(File &f) {
    #ifdef MIDI_DEBUG
    Serial.println(" - Meta");
    #endif
    uint8_t metaType = readInt8(f);
    uint32_t length = readIntMidi(f);
    uint32_t end = f.position() + length;
    
    switch ( metaType ) {
      case 0x51: { // Tempo event
        Serial.println("Tempo event");
        tempo = readInt24(f); // uint32_t tempo = 
        Serial.print("Tempo: "); Serial.println(tempo);
        
        break;
      }
      
      case 0x58: { // Time Signature event
        Serial.println("Time Signature");
        readInt8(f); // uint32_t timeSNum = 
        readInt8(f); // uint32_t timeSDen = 
        clocksPerTick = readInt8(f); // uint32_t clocksPerTick = 
        notesPer24Clocks = readInt8(f); // uint32_t notesPer24Clocks = 
        Serial.print("Clocks per tick: "); Serial.println(clocksPerTick);
        Serial.print("Notes / 24 clocks: "); Serial.println(notesPer24Clocks);
        
        break;
      }
      
      case 0x03: // Text event
      Serial.print("Text event. len: "); Serial.println(length);
      char * text = (char*)malloc(length+1);
      f.read(text, length);
      text[length] = 0;
      Serial.print('\t');
      Serial.println(text);
      break;
    }
    f.seek(end);
  }
  
  void skipSysex(File &f) {
    // Ignore sysex events, we're not dealing with it here.
    uint32_t length = readIntMidi(f);
    uint32_t end = f.position() + length;
    f.seek(end);
  }
  
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
};



#endif
