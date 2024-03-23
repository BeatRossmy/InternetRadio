// code from: https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/I2Saudio/I2Saudio.ino

#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"

#include <elapsedMillis.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>

#include "button.h"

// PCM5102 I2S
#define I2S_DOUT      2  // -> DIN
#define I2S_BCLK      4  // -> BCK
#define I2S_LRC       15 // -> LCK
                         // GND -> SCK

// NEOPIXEL
#define NEO_PIN       21

// ENCODER
#define ENC_IN1       19
#define ENC_IN2       18
#define ENC_BTN       5

// 
String stations[] = {
  "http://streams.br.de/bayern1_2.m3u",     // br 1
  "http://streams.br.de/b5aktuell_2.m3u",   // br 24 aktuell
  "http://streams.br.de/bayern2_2.m3u",     // br 2
  "https://st01.sslstream.dlf.de/dlf/01/128/mp3/stream.mp3?aggregator=web", // deutschlandfunk
  "https://st02.sslstream.dlf.de/dlf/02/128/mp3/stream.mp3?aggregator=web", // deutschlandfunk kultur
  "http://streams.br.de/br-klassik_2.m3u",  // klassikradio
  "http://streams.br.de/puls_2.m3u",        // puls
};

Audio audio;
WiFiMulti wifiMulti;
Adafruit_NeoPixel pixels(16, NEO_PIN);
Button button = {ENC_BTN,false};
RotaryEncoder encoder(ENC_IN1, ENC_IN2, RotaryEncoder::LatchMode::FOUR3);

int volume = 17; // 0-21
int station = 0; // 0-6

int _volume = 12;
int _station = 0;

elapsedMillis ledTimer, uiTimer;

int pos;

int background [] = {5,0,5,0,5,0,5,0,5,0,5,0,5,0,0,0};
int led_mapping [] = {14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15};

TaskHandle_t Task0;

bool station_flag = false;
bool volume_flag = false;
bool change;

bool DEBUG = true;

void setup() {
  Serial.begin(115200);

  // PIXEL SETUP
  pixels.setPixelColor(0, pixels.Color(50,0,0));
  pixels.show();

  // WIFI CONNECT AND REPEAT
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("wifi_name","password");
  wifiMulti.addAP("wifi_name","password");
  wifiMulti.addAP("wifi_name","password");
  try_connection();

  // AUDIO SETUP
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume);
  audio.connecttohost(stations[station].c_str());

  // DEFINE THREAD FOR SECOND CORE
  xTaskCreatePinnedToCore(
    AudioTask,   /* Task function. */
    "AudioTask / Core0",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task0,      /* Task handle to keep track of created task */
    0); 
}

void try_connection () {
  // WIFI AUTO CONNECT TO ONE OF THE STORED WIFIS
  if (DEBUG) {Serial.print("WiFi.status() => "); Serial.println(WiFi.status());}
  if (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect(true);
    wifiMulti.run();
  }
  if (DEBUG) {Serial.print("WiFi.status() => "); Serial.println(WiFi.status());}
  if (WiFi.status() != WL_CONNECTED){
    delay(2000);
    try_connection (); 
  }
  else if (WiFi.status() == WL_CONNECTED) {
    if (DEBUG) Serial.println("successful");
  }
}

//Audio Task on second core
void AudioTask( void * pvParameters ){
  Serial.print("Task0 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    if (station_flag) {
      _station = station;
      audio.connecttohost(stations[_station].c_str());
      station_flag = false;
    }
    if (volume_flag) { // *** new
      _volume = volume;
      audio.setVolume(_volume);
      volume_flag = false;
    }
    audio.loop();
  } 
}

void loop() {
  // UI
  button.read();
  if (button.hasChanged()) {
    ledTimer = 1000; // => led update
    Serial.print("button: ");
    Serial.println(button.state);
  }

  encoder.tick();
  int newPos = encoder.getPosition();
  if (pos != newPos) {
    pos = newPos;
    int d = (int)(encoder.getDirection())*-1;
    if (!button.state) {
      station = constrain(station+d,0,6);
    } else {
      volume = constrain(volume+d,0,21);  
    }
    uiTimer = 0;
    change = true;
    ledTimer = 1000;
  }

  // UI CHANGE 
  if (change && uiTimer>500) {
    station_flag = _station != station;
    volume_flag = _volume != volume;
    change = false;
  }

  // LEDS
  if (ledTimer>500) {
    ledTimer = 0;
    if (!button.state) {
      for (int i=0; i<16; i++) {
        int c = background[i];
        c = (i==2*station)?pixels.Color(50,40,5):pixels.Color(c,c,c);
        pixels.setPixelColor(led_mapping[i], c);
      }
    }
    else {
      for (int i=0; i<16; i++) {
        int c = pixels.Color(2,2,2);
        if (i>12) c = pixels.Color(0,0,0);
        else if (i<map(volume,0,21,0,12)) c = pixels.Color(25,25,25);
        pixels.setPixelColor(led_mapping[i], c);
      }
    }
    if (change || station_flag) {
      pixels.setPixelColor(led_mapping[14], pixels.Color(50,40,5));
    } else {
      pixels.setPixelColor(led_mapping[14], pixels.Color(5,50,5));
    }
    pixels.show();
  }
}
