#include "arduino_secrets.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_EPD.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SdFat.h> 
#include <Adafruit_GFX.h>
#include <TimeLib.h>
#include <Adafruit_ImageReader_EPD.h>
#include <Fonts/FreeSans9pt7b.h>


#define USE_SD_CARD

// ESP32 settings
#define SD_CS       14
#define SRAM_CS     32
#define EPD_CS      15
#define EPD_DC      33 
#define LEDPIN      13
#define LEDPINON    HIGH
#define LEDPINOFF   LOW 
 
#define EPD_RESET   -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    -1 // can set to -1 to not use a pin (will wait a fixed delay)

volatile int interruptCounter;
int totalInterruptCounter;
 
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
 
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
 
} 

char files[9][15] = {"/01d.bmp", "/02d.bmp", "/03d.bmp", "/04d.bmp", "/09d.bmp", "/10d.bmp", "/11d.bmp", "/13d.bmp", "/50d.bmp"};

Adafruit_SSD1675 display(250, 122, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#define COLOR EPD_BLACK

#if defined(USE_SD_CARD)
  SdFat                    SD;         // SD card filesystem
  Adafruit_ImageReader_EPD reader(SD); // Image-reader object, pass in SD filesys
#endif
Adafruit_Image_EPD   img;        // An image loaded into RAM
int32_t              width  = 0, // BMP image dimensions
                     height = 0;

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASSWORD;

//Get an OpenWeatherMap api token by creating an account
//Append the token after &appid=
const String token = SECRET_TOKEN;

const String URL = "http://api.openweathermap.org/data/2.5/weather?q=";
const String locations[3] = {"Ottawa, CA", "Montreal, CA", "Toronto, CA"};

String location = "";

WebServer httpServer(80);
DynamicJsonDocument response(50000);

// Retrieve page response from given URL
int getWeatherResponse(String location)
{
  String url = URL+location+token;
  HTTPClient http;
  int httpCode = 0;
  Serial.println("getting url: " + url);
  if(http.begin(url))
  {
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    httpCode = http.GET();
 
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been sent and Server response header has been handled
      Serial.println("[HTTP] GET... code: " + String(httpCode));
 
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      DeserializationError error = deserializeJson(response, http.getString());
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      }
    }
    } else {
      Serial.println("[HTTP] GET... failed, error: " + http.errorToString(httpCode));
    }
    http.end();
  }
  else {
    Serial.println("[HTTP] Unable to connect");
  }
  return httpCode;
}


void setup(void) {

  Serial.begin(115200);
  pinMode(13, OUTPUT);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  httpServer.on("/location", setLocation);
  httpServer.begin();

  display.begin();
  Serial.print(F("Initializing filesystem..."));
  #if defined(USE_SD_CARD)
  // SD card is pretty straightforward, a single call...
  if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
    Serial.println(F("SD begin() failed"));
    for(;;); // Fatal error, do not continue
  }
  #endif
    Serial.println(F("OK!"));

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 300*1000000, true);
  timerAlarmEnable(timer);

  wifiConnect();
  display.clearBuffer();
  
  if (location == ""){
    Serial.println("Displaying IP.......");
    displayIP(COLOR);
  }else{
    Serial.println("Fetching weather data.......");
    getWeatherResponse(location);
    displayWeatherData(response, COLOR);
  }
  display.display();
}

void loop(void) {
  httpServer.handleClient();
  if (interruptCounter > 0) {
    
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
    
    display.clearBuffer();
    
    if (location == ""){
      Serial.println("Displaying IP.......");
      displayIP(COLOR);
    }else{
      Serial.println("Fetching weather data.......");
      getWeatherResponse(location);
      displayWeatherData(response, COLOR);
    }
    display.display();
         
    }
  
}

void setLocation() {

  String city = "";
  String country = "";

  for (uint8_t i = 0; i < httpServer.args(); i++) {
    if (httpServer.argName(i) == "city"){
      city = httpServer.arg(i);
    }else if (httpServer.argName(i) == "country"){
      country = httpServer.arg(i);
    }
  }
  String requestedLocation = city + ", " + country ;

  int httpCode = getWeatherResponse(requestedLocation);
  if (httpCode != 200){
    httpServer.send(httpCode, "text/html");
  }else{
    location = requestedLocation;
    Serial.println("User selected new location: ");
    Serial.println(location);
    httpServer.send(200, "text/html"); 
  }
}

String getTime(int unix){
  int hour_int = (hour(unix)-5);
  bool am = true;
  if(hour_int>=12){am = false;}
  else if(hour_int<0){
    am = false;
    hour_int = hour_int+12;
    }
  hour_int = hour_int % 12;
  Serial.println(hour_int);
  Serial.println(hour_int%12);
  if(hour_int == 0){hour_int = 12;}
  String minute_str;
  if(minute(unix)<10){
    minute_str = "0"+String(minute(unix));
    }
   else{
    minute_str = String(minute(unix));
    }
   if (am){minute_str = minute_str + "am";}
   else{minute_str = minute_str + "pm";}
  
    
  return String(hour_int)+":"+minute_str+"\n";
  
  }

char* getWeatherIcon(String icon){

  if(icon =="01d"){return files[0];}
  if(icon =="02d"){return files[1];}
  if(icon =="03d"){return files[2];}
  if(icon =="04d"){return files[3];}
  if(icon =="09d"){return files[4];}
  if(icon =="10d"){return files[5];}
  if(icon =="11d"){return files[6];}
  if(icon =="13d"){return files[7];}
  if(icon =="50d"){return files[8];}
  
  if(icon =="01n"){return files[0];}
  if(icon =="02n"){return files[1];}
  if(icon =="03n"){return files[2];}
  if(icon =="04n"){return files[3];}
  if(icon =="09n"){return files[4];}
  if(icon =="10n"){return files[5];}
  if(icon =="11n"){return files[6];}
  if(icon =="13n"){return files[7];}
  if(icon =="50n"){return files[8];}
  return "missing";
  }

void displayIP(uint16_t color){
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextSize(2);
  display.setTextWrap(true);
  display.print(WiFi.localIP());
  }

void displayWeatherData(DynamicJsonDocument response, uint16_t color) {
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextSize(2);
  display.setTextWrap(true);
  
  display.print(location +"\n");
  int resp_int = response["dt"];
  display.print(getTime(resp_int));
  display.setTextSize(4);
  String resp = response["main"]["temp"];
  resp = String((int)round(resp.toFloat() - 273.15));
  display.setCursor(55, 45);
  display.print(resp+"C\n");


  resp = response["weather"][0]["icon"].as<String>();
  ImageReturnCode stat; // Status from image-reading functions
  stat = reader.drawBMP(getWeatherIcon(resp), display, 0, 35);
  reader.printStatus(stat); // How'd we do?
  
  resp = response["main"]["feels_like"].as<String>();
  resp = String((int)round(resp.toFloat() - 273.15)); 

  display.setCursor(0, 85);
  display.setTextSize(2);
  display.print("feels like "+resp+"C\n");
  //display.setCursor(0, 85);
  display.setTextSize(3);
  resp = response["weather"][0]["icon"].as<String>();
  //display.print(resp+"\n");
  Serial.println(resp);
  resp = response["weather"][0]["main"].as<String>();
  Serial.println(resp);
  display.print(resp+"\n");
  
  
}

bool wifiConnect(){
  // We start by connecting to a WiFi network
  if (WiFi.status() != WL_CONNECTED){
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    int timeout = 30;

    while (WiFi.status() != WL_CONNECTED) {
      if (timeout > 0){
        digitalWrite(13, HIGH);
        delay(1000);
        digitalWrite(13, LOW);
        Serial.print(".");
        delay(2000);
        timeout = timeout - 3;
      }
      else{
        WiFi.begin(ssid, password);
        timeout = 30;
        Serial.print("\n");
      }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  return true;
}
