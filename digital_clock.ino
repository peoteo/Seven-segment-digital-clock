#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>          //server web
#include <ESP8266HTTPUpdateServer.h>   //aggiornamenti firmware
#include <time.h>
#include "LittleFS.h"                  //filesystem
#include "secret.h"
#define NUM_LEDS 58
#define DATA_PIN D4

const char* ssid     = WIFI_NAME;
const char* password = WIFI_PASSWORD;
const char* tz = "CET-1CEST,M3.5.0/2,M10.5.0/3"; //wi-fi e fuso orario

CRGB leds[NUM_LEDS];
ESP8266WebServer server(80);               //webserver sulla porta 80
ESP8266HTTPUpdateServer httpUpdateServer;

CRGB alternateColor = CRGB::Black;
byte r_val = 0;
byte g_val = 200;
byte b_val = 255;
byte brightness = 50;
byte hourFormat = 24; 
unsigned long prevTime = 0;
bool dotsOn = true;

long digit01[] = {
  0b11111100111111,  // [0] 0
  0b00001100000011,  // [1] 1
  0b11110011001111,  // [2] 2
  0b00111111001111,  // [3] 3
  0b00001111110011,  // [4] 4
  0b00111111111100,  // [5] 5
  0b11111111111100,  // [6] 6
  0b00001100001111,  // [7] 7
  0b11111111111111,  // [8] 8
  0b00111111111111,  // [9] 9
  0b00000000000000,  // [10] off
};

long digit23[] = {
  0b11111100111111,  // [0] 0
  0b00001100000011,  // [1] 1
  0b00111111111100,  // [2] 2
  0b00111111001111,  // [3] 3
  0b11001111000011,  // [4] 4
  0b11110011001111,  // [5] 5
  0b11110011111111,  // [6] 6
  0b00111100000011,  // [7] 7
  0b11111111111111,  // [8] 8
  0b11111111001111,  // [9] 9
  0b00000000000000,  // [10] off
};



void setup() { 
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); 
  
  Serial.begin(115200);
  Serial.print("wi-fi connection");
  delay(200);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  delay(200);
   
  byte count = 0;                            //connessione wi-fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    // Stop if cannot connect
    if (count >= 60) {
      Serial.println("Could not connect to local WiFi.");      
      return;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  IPAddress ip = WiFi.localIP();
  Serial.println(ip[3]);
  
  configTime(tz, "pool.ntp.org", "time.nist.gov");   //configurazione e connessione NTP
  Serial.println("Waiting for time sync");
  time_t now = time(nullptr);
  int retry = 0;
  const int maxRetries = 20;
  while (now < 8 * 3600 * 2 && retry < maxRetries) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retry++;
  }

  httpUpdateServer.setup(&server);

  server.on("/color", HTTP_POST, []() {    
    r_val = server.arg("r").toInt();
    g_val = server.arg("g").toInt();
    b_val = server.arg("b").toInt();
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/brightness", HTTP_POST, []() {    
    brightness = server.arg("brightness").toInt();    
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/hourformat", HTTP_POST, []() {   
    hourFormat = server.arg("hourformat").toInt();  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.serveStatic("/", LittleFS, "/", "max-age=86400");
  server.begin();     

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  Serial.println("LittleFS contents:");

  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
  }
  Serial.println();
}


void loop() {
  server.handleClient();
  
  unsigned long currentMillis = millis();  
  if (currentMillis - prevTime >= 1000) {
    prevTime = currentMillis;
    FastLED.setBrightness(brightness);
    CRGB color = CRGB(r_val, g_val, b_val);
    updateClock(); 
    FastLED.show();
  }

}

void displayNumber(byte number, byte segment, CRGB color) {
  /*
   * 
      __  __          __ __            __ __          __ __  
    __       __    __       __      __       __    __       __
    __       __    __       __  _0  __       15    __       _1
      __  __          __ __            __ __          __ __  
    __       44    __       30  29  __       __    __       __
    __       __    __       __      __       __    __       __
      __  __          __ __            __ __          __ __   

   */
 
  // segmenti da sinistra a destra: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 1;
      break;
    case 1:
      startindex = 15;
      break;
    case 2:
      startindex = 30;
      break;
    case 3:
      startindex = 44;
      break;    
  }

  if (segment <= 1) {
      for (byte i=0; i<14; i++){
      yield();
      leds[i + startindex] = ((digit01[number] & 1 << i) == 1 << i) ? color : alternateColor;
    } 
  }
  else {
      for (byte i=0; i<14; i++){
      yield();
      leds[i + startindex] = ((digit23[number] & 1 << i) == 1 << i) ? color : alternateColor;
    } 
  }
}

void displayDots(CRGB color) {   
  if (dotsOn) {
    leds[0] = color;
    leds[29] = color;
  } else {
    leds[0] = CRGB::Black;
    leds[29] = CRGB::Black;
  }

  dotsOn = !dotsOn; 
}

void updateClock() {
  time_t rawTime = time(nullptr);
  struct tm timeinfo;
  localtime_r(&rawTime, &timeinfo);     //scompone la data

  int ora = timeinfo.tm_hour;
  int minuti = timeinfo.tm_min;  

  if (hourFormat == 12 && ora > 12)
    ora = ora - 12;       

  int h2 = ora / 10;
  int h1 = ora % 10;
  int m2 = minuti / 10;
  int m1 = minuti % 10;          //estrae le cifre di ore e minuti

  CRGB color = CRGB(r_val, g_val, b_val);

  displayNumber(h2,3,color);
  displayNumber(h1,2,color);
  displayNumber(m2,1,color);
  displayNumber(m1,0,color); 

  displayDots(color);  
}
