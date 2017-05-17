#include <Adafruit_NeoPixel.h>

#define PIN 4
#define PIXELS 60

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS+1, PIN, NEO_RGB + NEO_KHZ800);

void setup() {
  pinMode(6, OUTPUT); 
  digitalWrite(6, HIGH);
  delay(2000);
  Serial.begin(9600);
  strip.begin();
  for ( int i = 0; i < PIXELS; i++ ) {
    strip.setPixelColor(i, 20, 20, 40);
  }
  strip.show();

  digitalWrite(6, LOW);
}

int p = 0;
float ledFloat = 0;
void loop() {
  // put your main code here, to run repeatedly:
  int val = analogRead(0);    // read the value from the sensor


  for ( int i = 0; i <= PIXELS; i++ ) {
    strip.setPixelColor(i, 0, 0, 0);
  }

  float pos = (float)val / 1024;



  float currentLed = pos * PIXELS;
  ledFloat = ledFloat * 0.97 + currentLed * 0.03;
  int led = (int)ledFloat;
  float fractionalPart = ledFloat - led;

  Serial.print(ledFloat);
  Serial.print(' ');
  Serial.print(led);
  Serial.print(' ');
  Serial.println(fractionalPart);

  int maxBrightness = 50;
  int led1 = maxBrightness * (1-fractionalPart);
  int led2 = maxBrightness * (fractionalPart);
  strip.setPixelColor(led,led1,led1,led1);
  strip.setPixelColor(led+1,led2,led2,led2);
//  strip.setPixelColor(led,maxBrightness,maxBrightness,maxBrightness);

//  strip.setPixelColor(p,0,maxBrightness,maxBrightness);
//  p++;
//  if ( p >= 10 ) { 
//    p = 0;
//  }
  strip.show();
}
