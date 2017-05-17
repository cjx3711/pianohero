#include <Adafruit_NeoPixel.h>

#define KEYS 6
#define PIXELS_PER_KEY 10
#define PIXELS KEYS * PIXELS_PER_KEY
#define STRIP_PIN 4

Adafruit_NeoPixel strip = Adafruit_NeoPixel(KEYS * PIXELS_PER_KEY, STRIP_PIN, NEO_RGB + NEO_KHZ800);



void setKBPixel(int key, int pos, int rP, int gP, int bP) {
  // Odd keys are inverted
  if ( key > KEYS ) key = KEYS;
  if ( key < 0 ) key = 0;
  if ( pos > PIXELS_PER_KEY ) pos = PIXELS_PER_KEY;
  if ( pos < 0 ) pos = 0;
  
  int block = PIXELS_PER_KEY * key;
  if ( key % 2 == 0 ) {
    block += pos;
  } else {
    block += PIXELS_PER_KEY - pos - 1;
  }
  strip.setPixelColor(block, rP, gP, bP);
}


void setup() {
  // put your setup code here, to run once:
  pinMode(6, OUTPUT); 
  digitalWrite(6, HIGH);
  delay(1000);
  
  Serial.begin(9600);
  
  strip.begin();
  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 20, 20, 40);
  }
  strip.show();

  digitalWrite(6, LOW);
}

int counter = 0;
void loop() {
  // put your main code here, to run repeatedly:
  for ( int i = 0 ; i < KEYS * PIXELS_PER_KEY; i++ ) {
     strip.setPixelColor(i, 0, 0, 0);
  }

  counter--;
  if ( counter < 0 ) {
    counter = PIXELS_PER_KEY - 1;
  }

  for ( int i = 0 ; i < KEYS; i++ ) {
     setKBPixel(i, counter, 15, 15, 15);
  }
  
  strip.show();
  delay(100);
}



