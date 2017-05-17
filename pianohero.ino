#include <Adafruit_NeoPixel.h>

#define KEYS 6
#define PIXELS_PER_KEY 10
#define PIXELS KEYS * PIXELS_PER_KEY
#define STRIP_PIN 4

// Notes that can be displayed on screen
// Todo: This will eventually use the note length
#define SCREEN_HEIGHT 10

Adafruit_NeoPixel strip = Adafruit_NeoPixel(KEYS * PIXELS_PER_KEY, STRIP_PIN, NEO_RGB + NEO_KHZ800);


struct Note {
  int pos;
  uint8_t len;
  uint8_t key;
  Note() {
    pos = 0;
    len = 1;
    key = 0;
  }
  Note(int _pos, char _len, char _key) {
    pos = _pos;
    len = _len;
    key = _key;
  }
};

Note notes[10];

void setKBPixel(int key, int pos, int rP, int gP, int bP) {
  // Odd keys are inverted
  if ( key > KEYS ) key = KEYS;
  if ( key < 0 ) key = 0;
  if ( pos >= PIXELS_PER_KEY ) pos = PIXELS_PER_KEY;
  if ( pos < 0 ) pos = 0;

  int block = PIXELS_PER_KEY * key;
  if ( key % 2 == 0 ) {
    block += pos;
  } else {
    block += PIXELS_PER_KEY - pos - 1;
  }
  strip.setPixelColor(block, rP, gP, bP);
}

bool inScreen(int noteIndex) {
  
}
void setScreenState(float pos) {
  int maxNote = 10;
  int currentNote = maxNote * pos;
  // Get first
  Serial.println(currentNote);
}

void setup() {
  notes[0].key = 4;
  notes[1].key = 2;
  notes[2].key = 0;
  notes[3].key = 2;
  notes[4].key = 4;
  notes[5].key = 4;
  notes[6].key = 4;
  for ( int i = 0; i < 10; i++) {
    notes[i].pos = i;
    }

  // put your setup code here, to run once:
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  delay(1000);

  Serial.begin(9600);
  Serial.println("Start");
  for ( int i = 0; i < 10; i++) {
    Serial.print(notes[i].pos); Serial.print(' ');
    Serial.print(notes[i].len); Serial.print(' ');
    Serial.print(notes[i].key); Serial.println();
  }

  strip.begin();

  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 20, 20, 40);
  }
  strip.show();

  digitalWrite(6, LOW);
}

float ledFloat;

void loop() {
  // put your main code here, to run repeatedly:
  int val = analogRead(0);    // read the value from the sensor

  for ( int i = 0 ; i < KEYS * PIXELS_PER_KEY; i++ ) {
     strip.setPixelColor(i, 0, 0, 0);
  }

  float pos = (float)val / 1024;

  float currentLed = pos * (PIXELS_PER_KEY - 1);
  ledFloat = ledFloat * 0.97 + currentLed * 0.03;
  int led = (int)ledFloat;
  float fractionalPart = ledFloat - led;

  int maxBrightness = 30;
  int led1 = maxBrightness * (1-fractionalPart);
  int led2 = maxBrightness * (fractionalPart);

  setScreenState(pos);
  // for ( int i = 0 ; i < KEYS; i++ ) {
  //   setKBPixel(i,led,led1,led1,led1);
  //   setKBPixel(i,led+1,led2,led2,led2);
  // }

//  for ( int i = 0 ; i < KEYS; i++ ) {
//    setKBPixel(i,led,maxBrightness,maxBrightness,maxBrightness);
//  }

  strip.show();
//  delay(100);
}
