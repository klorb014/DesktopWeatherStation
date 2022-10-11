#include "arduino_secrets.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_EPD.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SdFat.h>
#include <Adafruit_GFX.h>
#include <TimeLib.h>
#include <Adafruit_ImageReader_EPD.h>
#include <Fonts/FreeSans9pt7b.h>


// ESP32 settings
#define USE_SD_CARD
#define VBATPIN A13

#define SD_CS 14
#define SRAM_CS 32
#define EPD_CS 15
#define EPD_DC 33
#define LEDPIN 13
#define LEDPINON HIGH
#define LEDPINOFF LOW

#define EPD_RESET -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1  // can set to -1 to not use a pin (will wait a fixed delay)

volatile int interruptCounter;
int totalInterruptCounter;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

char files[9][15] = {"/01d.bmp", "/02d.bmp", "/03d.bmp", "/04d.bmp", "/09d.bmp", "/10d.bmp", "/11d.bmp", "/13d.bmp", "/50d.bmp"};

Adafruit_SSD1675 display(250, 122, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#define COLOR EPD_BLACK

#if defined(USE_SD_CARD)
SdFat SD;                            // SD card filesystem
Adafruit_ImageReader_EPD reader(SD); // Image-reader object, pass in SD filesys
#endif
Adafruit_Image_EPD img; // An image loaded into RAM
int32_t width = 0,      // BMP image dimensions
    height = 0;

const char *ssid = SECRET_SSID;
const char *password = SECRET_PASSWORD;

// Get an OpenWeatherMap api token by creating an account
// Append the token after &appid=
const String token = SECRET_TOKEN;

const String URL = "http://api.openweathermap.org/data/2.5/weather?q=";

String location = "";

const char *APssid = "DesktopWeatherStation";
const char *APpassword = "123456789";

WebServer httpServer(80);
DynamicJsonDocument response(50000);

const int MAX_ANALOG_VAL = 4095;
const float MAX_BATTERY_VOLTAGE = 4.2;
const float PROTECTION_VOLTAGE = 3.2;

#define uS_TO_MIN_FACTOR 60000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10          /* Time ESP32 will go to sleep (in minutes) */

RTC_DATA_ATTR int bootCount = 0;

enum Mode
{
  LOW_POWER,
  ACCESS_POINT,
  HTTP_SERVER
};

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000); // Take some time to open up the Serial Monitor

  int rawValue = analogRead(VBATPIN);
  float voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3; // calculate voltage level
  float batteryFraction = voltageLevel / MAX_BATTERY_VOLTAGE;

  Serial.println((String) "Raw:" + rawValue + " Voltage:" + voltageLevel + "V Percent: " + (batteryFraction * 100) + "%");

  display.begin();
  Serial.print(F("Initializing filesystem..."));
#if defined(USE_SD_CARD)
  // SD card is pretty straightforward, a single call...
  if (!SD.begin(SD_CS, SD_SCK_MHZ(10)))
  { // Breakouts require 10 MHz limit due to longer wires
    Serial.println(F("SD begin() failed"));
    for (;;)
      ; // Fatal error, do not continue
  }
#endif
  Serial.println(F("OK!"));

  if (availableConfiguration())
  {
    loadConfiguration();
  }
  refreshDisplay();

  Mode mode = LOW_POWER;

  switch (mode)
  {
  case (mode == Mode::LOW_POWER):
    lowPowerMode();
    break;
  case (mode == Mode::ACCESS_POINT):
    startAccessPoint();
    break;

  default:
    break;
  }

  
  
}

void lowPowerMode(){
// Print the wakeup reason for ESP32
  print_wakeup_reason();
  /*
  set ESP32 to wake up every 10 Minutes
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_MIN_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Minutes");
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
}

void accessPointMode(){

}

// Retrieve page response from given URL
int getWeatherResponse(String location)
{
  String url = URL + location + token;
  HTTPClient http;
  int httpCode = 0;
  Serial.println("getting url: " + url);
  if (http.begin(url))
  {
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been sent and Server response header has been handled
      Serial.println("[HTTP] GET... code: " + String(httpCode));

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        DeserializationError error = deserializeJson(response, http.getString());
        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
        }
      }
    }
    else
    {
      Serial.println("[HTTP] GET... failed, error: " + http.errorToString(httpCode));
    }
    http.end();
  }
  else
  {
    Serial.println("[HTTP] Unable to connect");
  }
  return httpCode;
}

/*
void setup(void) {

  Serial.begin(115200);
  pinMode(13, OUTPUT);
  Serial.println();
  Serial.println("Booting Sketch...");
  startAccessPoint();

  if (!MDNS.begin("esp32")) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
  Serial.println("mDNS responder started");

  httpServer.on("/", handleRoot);
  httpServer.on("/inline", []() {
    httpServer.send(200, "text/plain", "this works as well");
  });
  httpServer.on("/submit", handleWifiConnect);
  httpServer.on("/location", setLocation);
  httpServer.begin();
  Serial.println("HTTP server started");

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

  if (availableConfiguration()){
    loadConfiguration();
  }
  refreshDisplay();

}*/

void loop(void)
{
  // httpServer.handleClient();

  if (interruptCounter > 0)
  {
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);

    refreshDisplay();
  }
}

void refreshDisplay()
{
  display.clearBuffer();

  if (WiFi.status() != WL_CONNECTED)
  {
    if (availableConfiguration())
    {
      loadConfiguration();
    }
    else
    {
      Serial.println("Displaying Welcome Screen");
      displayWelcome(COLOR);
      display.display();
      return;
    }
  }
  if (location == "")
  {
    Serial.println("Displaying IP.......");
    displayIP(COLOR);
  }
  else
  {
    Serial.println("Fetching weather data.......");
    getWeatherResponse(location);
    displayWeatherData(response, COLOR);
    storeConfiguration();
  }

  display.display();
}

void setLocation()
{

  String city = "";
  String country = "";

  for (uint8_t i = 0; i < httpServer.args(); i++)
  {
    if (httpServer.argName(i) == "city")
    {
      city = httpServer.arg(i);
    }
    else if (httpServer.argName(i) == "country")
    {
      country = httpServer.arg(i);
    }
  }
  String requestedLocation = city + ", " + country;

  int httpCode = getWeatherResponse(requestedLocation);
  if (httpCode != 200)
  {
    httpServer.send(httpCode, "text/html");
  }
  else
  {
    location = requestedLocation;
    Serial.println("User selected new location: ");
    Serial.println(location);
    httpServer.send(200, "text/html");
    refreshDisplay();
  }
}

String getTime(int unix)
{
  int hour_int = (hour(unix) - 4);
  bool am = true;
  if (hour_int >= 12)
  {
    am = false;
  }
  else if (hour_int < 0)
  {
    am = false;
    hour_int = hour_int + 12;
  }
  hour_int = hour_int % 12;
  // Serial.println(hour_int);
  // Serial.println(hour_int%12);
  if (hour_int == 0)
  {
    hour_int = 12;
  }
  String minute_str;
  if (minute(unix) < 10)
  {
    minute_str = "0" + String(minute(unix));
  }
  else
  {
    minute_str = String(minute(unix));
  }
  if (am)
  {
    minute_str = minute_str + "am";
  }
  else
  {
    minute_str = minute_str + "pm";
  }

  return String(hour_int) + ":" + minute_str + "\n";
}

char *getWeatherIcon(String icon)
{

  if (icon == "01d")
  {
    return files[0];
  }
  if (icon == "02d")
  {
    return files[1];
  }
  if (icon == "03d")
  {
    return files[2];
  }
  if (icon == "04d")
  {
    return files[3];
  }
  if (icon == "09d")
  {
    return files[4];
  }
  if (icon == "10d")
  {
    return files[5];
  }
  if (icon == "11d")
  {
    return files[6];
  }
  if (icon == "13d")
  {
    return files[7];
  }
  if (icon == "50d")
  {
    return files[8];
  }

  if (icon == "01n")
  {
    return files[0];
  }
  if (icon == "02n")
  {
    return files[1];
  }
  if (icon == "03n")
  {
    return files[2];
  }
  if (icon == "04n")
  {
    return files[3];
  }
  if (icon == "09n")
  {
    return files[4];
  }
  if (icon == "10n")
  {
    return files[5];
  }
  if (icon == "11n")
  {
    return files[6];
  }
  if (icon == "13n")
  {
    return files[7];
  }
  if (icon == "50n")
  {
    return files[8];
  }
  return "missing";
}

void displayIP(uint16_t color)
{
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextSize(2);
  display.setTextWrap(true);
  display.print(WiFi.localIP());
}

void displayWelcome(uint16_t color)
{
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextSize(2);
  display.setTextWrap(true);
  display.print("Desktop Weather \nStation\n\nSetup Wifi At:\n192.168.4.1/");
}

void displayWeatherData(DynamicJsonDocument response, uint16_t color)
{
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextSize(2);
  display.setTextWrap(true);

  display.print(location + "\n");
  int resp_int = response["dt"];
  display.print(getTime(resp_int));
  display.setTextSize(4);
  String resp = response["main"]["temp"];
  resp = String((int)round(resp.toFloat() - 273.15));
  display.setCursor(55, 45);
  display.print(resp + "C\n");

  resp = response["weather"][0]["icon"].as<String>();
  ImageReturnCode stat; // Status from image-reading functions
  stat = reader.drawBMP(getWeatherIcon(resp), display, 0, 35);
  reader.printStatus(stat); // How'd we do?

  resp = response["main"]["feels_like"].as<String>();
  resp = String((int)round(resp.toFloat() - 273.15));

  display.setCursor(0, 85);
  display.setTextSize(2);
  display.print("feels like " + resp + "C\n");
  // display.setCursor(0, 85);
  display.setTextSize(3);
  resp = response["weather"][0]["icon"].as<String>();
  // display.print(resp+"\n");
  Serial.println(resp);
  resp = response["weather"][0]["main"].as<String>();
  Serial.println(resp);
  display.print(resp + "\n");

  display.setCursor(220, 0);
  display.setTextSize(1);

  int rawValue = analogRead(VBATPIN);
  float voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3; // calculate voltage level
  int batteryFraction = (int)round(((voltageLevel - PROTECTION_VOLTAGE) / (MAX_BATTERY_VOLTAGE - PROTECTION_VOLTAGE)) * 100);
  display.print((String) "(" + batteryFraction + "%)");

  display.setCursor(200, 10);
  display.setTextSize(1);
  display.print((String) "(" + voltageLevel + ")");
}

bool wifiConnect(const char *ssid, const char *pass)
{
  // We start by connecting to a WiFi network
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Try Connecting to ");
    Serial.println(ssid);
    Serial.println(pass);

    // Connect to your wi-fi modem
    WiFi.begin(ssid, pass);

    int retries = 0;
    const int maxRetries = 30;

    // Check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED)
    {
      if (retries < maxRetries)
      {
        retries += 1;
        delay(1000);
        Serial.print(".");
      }
      else
      {
        break;
      }
    }
  }
  return true;
}

void handleRoot()
{
  char temp[1000];
  snprintf(temp, 1000,

           "<html>\
  <head>\
    <title>ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Desktop Weather Station: Wifi Setup</h1>\
    <form action=\"/submit\" method=\"post\">\
    <label for=\"fname\">Network:</label><br>\
    <input type=\"text\" id=\"ssid\" name=\"ssid\"><br>\
    <label for=\"password\">Password:</label><br>\
    <input type=\"password\" id=\"password\" name=\"password\"><br><br>\
    <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>");
  httpServer.send(200, "text/html", temp);
}

bool handleWifiConnect()
{
  if (httpServer.method() == HTTP_POST)
  {
    int bufsize = 40;
    char tmpSsid[bufsize];
    char tmpPassword[bufsize];

    for (uint8_t i = 0; i < httpServer.args(); i++)
    {
      if (httpServer.argName(i) == "ssid")
      {
        httpServer.arg(i).toCharArray(tmpSsid, bufsize);
      }
      else if (httpServer.argName(i) == "password")
      {
        httpServer.arg(i).toCharArray(tmpPassword, bufsize);
      }
    }

    // Try to connect to network
    WiFi.softAPdisconnect(true);

    wifiConnect(tmpSsid, tmpPassword);

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi connected successfully");
      Serial.print("Got IP: ");
      Serial.println(WiFi.localIP());

      refreshDisplay();

      return true;
    }
    else
    {
      startAccessPoint();
      return false;
    }
  }
  httpServer.send(400, "text/html", "Bad Request");
  return false;
}

void startAccessPoint()
{
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(APssid, APpassword);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

bool storeConfiguration()
{
  File confFile = SD.open("weather-station.conf", FILE_WRITE);
  // if the file opened okay, write to it:
  if (confFile)
  {
    Serial.print("Writing to weather-station.conf...");
    confFile.println(ssid);
    confFile.println(password);
    confFile.println(location);
    // close the file:
    confFile.close();
    Serial.println("done.");
    return true;
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
    return false;
  }
}

bool availableConfiguration()
{
  File confFile = SD.open("weather-station.conf", FILE_READ);
  // if the file opened okay, write to it:
  if (confFile)
  {
    Serial.println("Checking if stored configuration");
    if (confFile.peek() != -1)
    {
      Serial.println("Configuration Available");
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening weather-station.conf");
    return false;
  }
}

bool loadConfiguration()
{
  File confFile = SD.open("weather-station.conf", FILE_READ);
  // if the file opened okay, write to it:
  if (confFile)
  {
    Serial.println("Reading configuration from weather-station.conf...");
    std::string contents = "";
    std::string next;
    int line = 0;
    std::string tmpSsid;
    std::string tmpPassword;
    while (confFile.available())
    {
      next = confFile.read();
      if (next == "\n")
      {
        switch (line % 3)
        {
        case 0:
          tmpSsid = trim(contents);
          break;
        case 1:
          tmpPassword = trim(contents);
          break;
        default:
          location = trim(contents).c_str();
        }
        contents = "";
        line += 1;
      }
      else
      {
        contents += next;
      }
    }
    // close the file:
    confFile.close();
    Serial.println("done.");

    wifiConnect(tmpSsid.c_str(), tmpPassword.c_str());

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi connected successfully");
      Serial.print("Got IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    else
    {
      Serial.println("WiFi Failed to Connect");
      return false;
    }
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening weather-station.conf");
    return false;
  }
}

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string &s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s)
{
  return rtrim(ltrim(s));
}
