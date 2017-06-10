#include <Adafruit_NeoPixel.h>

#include "MidiFile.h"


#define STRIP_PIN 24

#define ROTARY_1 15
#define ROTARY_2 16
#define ROTARY_B 17



Adafruit_NeoPixel strip = Adafruit_NeoPixel(KEYS * PIXELS_PER_KEY, STRIP_PIN, NEO_RGB + NEO_KHZ800);

float ledFloat;

uint8_t pc1 = 1;
uint8_t pc2 = 1;
uint8_t dir = 0;

long lastMillis = 0;
long curMillis = 0;
float pos = 0;
float vel = 0;

bool lastRotState;  
bool lastBtnState;

uint8_t maxBrightness = 16;

MidiFile midi;

void setup() {
  Serial.begin(9600);
  Serial.println("Start");
  // put your setup code here, to run once:
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  
  pinMode (ROTARY_1,INPUT);
  pinMode (ROTARY_2,INPUT);
  pinMode (ROTARY_B,INPUT);
  digitalWrite(ROTARY_B, HIGH);
  
  strip.begin();
  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 10, 5, 3);
  }
  strip.show();
  
  delay(500);

  // Serial.println("Sizes (Bytes):");
  // Serial.print("uint8_t:"); Serial.println(sizeof(uint8_t));
  // Serial.print("uint16_t:"); Serial.println(sizeof(uint16_t));
  // Serial.print("uint32_t:"); Serial.println(sizeof(uint32_t));
  // Serial.print("char:"); Serial.println(sizeof(char));
  // Serial.print("uint8_t*:"); Serial.println(sizeof(uint8_t*));
  // Serial.print("uint16_t*:"); Serial.println(sizeof(uint16_t*));
  // Serial.print("uint32_t*:"); Serial.println(sizeof(uint32_t*));
  // Serial.print("char*:"); Serial.println(sizeof(char*));
  // Serial.print("BlockPointers:"); Serial.println(sizeof(BlockPointers));
  // Serial.print("BlockPointers*:"); Serial.println(sizeof(BlockPointers*));
  // Serial.print("FilePos:"); Serial.println(sizeof(FilePos));
  // Serial.print("FilePos*:"); Serial.println(sizeof(FilePos*));
  midi.init();
  
  if ( SD.begin() ) {
    Serial.println("SD connected");
    
    File root = SD.open("/");
    printDirectory(root, 0);
    
    midi.openFile((char*)"test-one.mid");
  } else {
    Serial.println("SD fail");
  }
  
  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
  
  digitalWrite(6, LOW);
  
  updatePos();
  
  lastRotState = digitalRead(ROTARY_1);
  lastBtnState = digitalRead(ROTARY_B);
}

void changeBrightness() {
  switch ( maxBrightness ) {
    case 8:
      maxBrightness = 16;
    break;
    case 16:
      maxBrightness = 32;
    break;
    case 32:
      maxBrightness = 64;
    break;
    case 64:
      maxBrightness = 128;
    break;
    case 128:
      maxBrightness = 255;
    break;
    case 255:
      maxBrightness = 8;
    break;
  }
}

void loop() {
  {
    bool b = digitalRead(ROTARY_B);
    bool c1 = digitalRead(ROTARY_1);
    bool c2 = digitalRead(ROTARY_2);
    // Serial.print(c1);
    // Serial.print('\t');
    // Serial.println(c2);
    curMillis = millis();
    if (c1 != lastRotState && !c1) {
      if (c2 != c1) { 
         Serial.println("Up");
         pos -= 1 / (float)(((float)midi.trackLength / (float)midi.division) * (float)midi.qNoteScreenSize);
         updatePos();
         Serial.println(curMillis - lastMillis);
       } else {
         Serial.println("Down");
         pos += 1 / (float)(((float)midi.trackLength / (float)midi.division) * (float)midi.qNoteScreenSize);
         updatePos();
         Serial.println(curMillis - lastMillis);
         
       }
    }
    if ( b != lastBtnState && !b ) {
      // Serial.println("Btn");
      changeBrightness();
    }
    
    lastRotState = c1;
    lastBtnState = b;
  }
  


  setScreenState();
  
  strip.show();
  lastMillis = curMillis;
}

void updatePos() {
  if ( pos < 0 ) pos = 0;
  if ( pos > 1 ) pos = 1;
  Serial.print("POS: "); Serial.println(pos);

  midi.setPosition(pos);
}

void printDirectory(File dir, int numTabs) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.print("/");
       Serial.println("\t\tDIR");
       // printDirectory(entry, numTabs+1);
     } else {
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
   }
}


void setKBPixel(uint8_t key, uint8_t pos, float rP, float gP, float bP) {
  uint8_t R = rP * maxBrightness;
  uint8_t G = gP * maxBrightness;
  uint8_t B = bP * maxBrightness;
  // Odd keys are inverted
  if ( key > KEYS - 1 ) key = KEYS - 1;
  if ( key < 0 ) key = 0;
  if ( pos > PIXELS_PER_KEY - 1 ) pos = PIXELS_PER_KEY - 1;
  if ( pos < 0 ) pos = 0;

  int block = PIXELS_PER_KEY * key;
  if ( key % 2 ) {
    block += pos;
  } else {
    block += PIXELS_PER_KEY - pos - 1;
  }
  strip.setPixelColor(block, G, R, B);
}

void drawNote(uint8_t key, uint8_t pos, uint8_t len, uint8_t col) {
  // Serial.print(pos); Serial.print(' '); Serial.print(len); Serial.print(' '); Serial.println(key);
  for ( uint8_t i = 0; i < len; i++ ) {
    uint8_t dispPos = pos + i;
    if ( dispPos < PIXELS_PER_KEY ) { // Out of screen
      if ( i == 0 ) {
        if ( col ) {
          setKBPixel(key, dispPos, 0, 1, 0.475);
        } else {
          setKBPixel(key, dispPos, 1, 0.5, 0.25);
        }
        
      } else {
        if ( col ) {
          setKBPixel(key, dispPos, 0, 0.38 , 0.12);
        } else {
          setKBPixel(key, dispPos, 0.375, 0.125, 0.0625);
        }
      }
      
    }
  }
}

void setScreenState() {
  for ( uint32_t i = 0 ; i < KEYS * PIXELS_PER_KEY; i++ ) {
     strip.setPixelColor(i, 0, 0, 0);
  }
  for ( uint8_t which = 0; which < midi.trackCount; which++ ) {
    uint16_t i = midi.getFirstInScreen(which);
    
    uint32_t noteCount = midi.getNoteCount(which);
    // Serial.print("First: ");
    // Serial.print(i); Serial.print(" max: "); Serial.println(noteCount); 
    // Serial.println("Notes in screen:");
    while ( i < noteCount ) {
      Note * note = midi.getNoteInScreen(which, i);
      if ( !note ) break;
      // Serial.print(i); Serial.print(' ');
      uint8_t pos = note->pos / (midi.division / midi.qNoteScreenSize) - midi.trackPosition  / (midi.division / midi.qNoteScreenSize);
      uint8_t len = note->len / (midi.division / midi.qNoteScreenSize);
      
      drawNote(note->key - 72, pos, len, i % 2 );
      i++;
    }
  }

  strip.show();
}
