#include <Adafruit_NeoPixel.h>

#include "MidiFile.h"

#define KEYS 6
#define PIXELS_PER_KEY 10
#define PIXELS KEYS * PIXELS_PER_KEY
#define STRIP_PIN 24

#define ROTARY_1 15
#define ROTARY_2 16
#define ROTARY_B 17
// Notes that can be displayed on screen
// Todo: This will eventually use the note length
#define SCREEN_HEIGHT 10

Adafruit_NeoPixel strip = Adafruit_NeoPixel(KEYS * PIXELS_PER_KEY, STRIP_PIN, NEO_RGB + NEO_KHZ800);


float ledFloat;

uint8_t pc1 = 1;
uint8_t pc2 = 1;
uint8_t dir = 0;
long lastMillis = 0;
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
    strip.setPixelColor(i, 15, 5, 5);
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
    
    midi.openFile((char*)"test-two.mid");
  } else {
    Serial.println("SD fail");
  }
  
  
  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
  
  digitalWrite(6, LOW);
  
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
    if (c1 != lastRotState && !c1) {
      if (c2 != c1) { 
         // Serial.println("Up");
         pos -= 0.03;
         updatePos();
       } else {
         // Serial.println("Down");
         pos += 0.03;
         updatePos();
       }
    }
    if ( b != lastBtnState && !b ) {
      // Serial.println("Btn");
      changeBrightness();
    }
    
    lastRotState = c1;
    lastBtnState = b;
  }

  // long curMillis = millis();
  // float delta = (float)(curMillis - lastMillis) / 1000.0f;

  for ( int i = 0 ; i < KEYS * PIXELS_PER_KEY; i++ ) {
     strip.setPixelColor(i, 0, 0, 0);
  }

  // float currentLed = pos * (PIXELS_PER_KEY - 1);
  // ledFloat = ledFloat * 0.97 + currentLed * 0.03;
  // int led = (int)ledFloat;
  // float fractionalPart = ledFloat - led;

  // int maxBrightness = 30;
  // int led1 = maxBrightness * (1-fractionalPart);
  // int led2 = maxBrightness * (fractionalPart);
  // 
  setScreenState(pos);
  // for ( int i = 0 ; i < KEYS; i++ ) {
  //   setKBPixel(i,led,led1,led1,led1);
  //   setKBPixel(i,led+1,led2,led2,led2);
  // }

  //  for ( int i = 0 ; i < KEYS; i++ ) {
  //    setKBPixel(i,led,maxBrightness,maxBrightness,maxBrightness);
  //  }
  strip.show();
}

void updatePos() {
  if ( pos < 0 ) pos = 0;
  if ( pos > 1 ) pos = 1;
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


void setKBPixel(int key, int pos, int rP, int gP, int bP) {
  // Odd keys are inverted
  if ( key > KEYS - 1 ) key = KEYS - 1;
  if ( key < 0 ) key = 0;
  if ( pos > PIXELS_PER_KEY - 1 ) pos = PIXELS_PER_KEY - 1;
  if ( pos < 0 ) pos = 0;

  int block = PIXELS_PER_KEY * key;
  if ( key % 2 == 0 ) {
    block += pos;
  } else {
    block += PIXELS_PER_KEY - pos - 1;
  }
  strip.setPixelColor(block, rP, gP, bP);
}

void setKBPixelInv(int key, int pos, int rP, int gP, int bP) {
  setKBPixel(key, PIXELS_PER_KEY - pos, rP, gP, bP);
}

bool inScreen(int notePos, int noteIndex) {
  if ( notePos >= midi.getNote(noteIndex).pos + midi.getNote(noteIndex).len ) {
    return false; // Past it's playtime
  }
  if ( notePos + SCREEN_HEIGHT < midi.getNote(noteIndex).pos ) {
    return false; // Not yet in view
  }
  return true;
}
void setScreenState(float pos) {
  int maxNote = 22;
  int currentNote = ((maxNote + SCREEN_HEIGHT + 2) * pos) - SCREEN_HEIGHT - 2;
  for ( int i = 0; i < 10; i++ ) {
    if ( inScreen(currentNote, i) ) {
      // Calculate position on screen
      int pos = currentNote - midi.getNote(i).pos + SCREEN_HEIGHT;

      if ( i % 2 ) {
        setKBPixelInv(midi.getNote(i).key, pos, 0, maxBrightness, 0.875 * maxBrightness);
      } else {
        setKBPixelInv(midi.getNote(i).key, pos, 0.25 * maxBrightness, 0.5 * maxBrightness, maxBrightness);
      }
      for ( int l = 1; l < midi.getNote(i).len; l++) {
        if ( i % 2 ) {
          setKBPixelInv(midi.getNote(i).key, pos - l, 0, 0.4375 * maxBrightness, 0.25 * maxBrightness);
        } else {
          setKBPixelInv(midi.getNote(i).key, pos - l, 0.0625 * maxBrightness, 0.125 * maxBrightness, 0.375 * maxBrightness);
        }

      }
    }
  }
}
