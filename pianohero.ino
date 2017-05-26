#include <Adafruit_NeoPixel.h>

#include "MidiFile.h"

#define KEYS 6
#define PIXELS_PER_KEY 10
#define PIXELS KEYS * PIXELS_PER_KEY
#define STRIP_PIN 4

#define CONTROL_1 25
#define CONTROL_2 26

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

MidiFile midi;
void setup() {
  // put your setup code here, to run once:
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  
  strip.begin();
  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 15, 5, 5);
  }
  strip.show();
  
  delay(1000);
  
  pinMode(CONTROL_1, INPUT);
  pinMode(CONTROL_2, INPUT);
  
  Serial.begin(9600);
  Serial.println("Start");
  
  midi.init();
  
  if ( SD.begin() ) {
    Serial.println("SD connected");
  } else {
    Serial.println("SD fail");
  }
  File root = SD.open("/");
  printDirectory(root, 0);
  
  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
  
  digitalWrite(6, LOW);
}

void loop() {

  uint8_t c1 = digitalRead(CONTROL_1);
  uint8_t c2 = digitalRead(CONTROL_2);

  long curMillis = millis();
  float delta = (float)(curMillis - lastMillis) / 1000.0f;
  if ( vel > 0.02f ) vel = 0.02f;
  if ( vel < -0.02f ) vel = -0.02f;
//  vel *= 1.0f / (1.0f + (delta * 0.1f));
  pos += vel * delta;

  // Hack to get the stupid thing working.
  if ( curMillis - lastMillis > 100 ) {
    dir = 0;
  }
  if ( c1 != pc1 ) {
    pc1 = c1;
    if ( c1 == 1 ) {
      if ( dir == 0 ) {
        dir = -1;
        lastMillis = millis();
      } else {
        dir = 0;
        pos += 0.03;
        Serial.println(pos);
      }
    }
  }
  if ( c2 != pc2 ) {
    pc2 = c2;
    if ( c2 == 1 ) {
      if ( dir == 0 ) {
        dir = 1;
        lastMillis = millis();
      } else {
        dir = 0;
        pos -= 0.03;
        Serial.println(pos);
      }
    }
  }

  if ( pos < 0 ) pos = 0;
  if ( pos > 1 ) pos = 1;

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
        setKBPixelInv(midi.getNote(i).key, pos, 0, 16, 14);
      } else {
        setKBPixelInv(midi.getNote(i).key, pos, 4, 8, 16);
      }
      for ( int l = 1; l < midi.getNote(i).len; l++) {
        if ( i % 2 ) {
          setKBPixelInv(midi.getNote(i).key, pos - l, 0, 7, 4);
        } else {
          setKBPixelInv(midi.getNote(i).key, pos - l, 1, 2, 6);
        }

      }
    }
  }
}
