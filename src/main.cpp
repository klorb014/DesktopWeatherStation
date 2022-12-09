#include <network_secrets.h>
#include <Arduino.h>
#include <network.h>
#include <logger.h>
#include <control.h>
#include <graphics.h>
#include <SdFat.h>
#include <Adafruit_EPD.h>
#include <Adafruit_GFX.h>
#include <WebServer.h>
#include <Adafruit_ImageReader_EPD.h>
#include <enum.h>

#define SRAM_CS 32
#define EPD_CS 15
#define EPD_DC 33
#define LEDPIN 13
#define LEDPINON HIGH
#define LEDPINOFF LOW
#define COLOR EPD_BLACK

#define EPD_RESET -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1  // can set to -1 to not use a pin (will wait a fixed delay)

#define REFRESH_TIMER 300 //5 minutes in seconds

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

#define SD_CS 14
//SdFat SD;
Adafruit_ImageReader_EPD reader(Configuration::SD); // Image-reader object, pass in SD filesys 
Adafruit_SSD1675 display(250, 122, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

HttpServer *httpServer;
Configuration *config;
Controller *controller;
Graphics* graphics;
String location = "Ottawa, Canada"; 


void setup() {
  delay(10000);
  Serial.begin(115200);
  while(!Serial && !Serial.available()){}
  
  config = new Configuration();
  graphics = new Graphics(&display, &reader);
  httpServer = new HttpServer(config);
  controller = new Controller(httpServer, config, graphics);


  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, REFRESH_TIMER*1000000, true);
  timerAlarmEnable(timer);
  

  Mode newState;
  if (!Configuration::initializeSD()){
      newState = Mode::ERROR;
      controller->stateChange(newState);
  }
  else if (Configuration::available(&newState) && Configuration::load(&newState, config))
  {
      config->connect() ? controller->stateChange(Mode::HTTP_SERVER) : controller->stateChange(Mode::ACCESS_POINT);
  }
  else
  {
      controller->stateChange(newState);
  }
  // put your main code here, to run repeatedly:
  if (controller->getState() == Mode::HTTP_SERVER){
    controller->refreshDisplay();
  }
  LOG_INFO("Setup Complete");
}

bool handleTimer()
{
    if (interruptCounter > 0)
    {
      portENTER_CRITICAL(&timerMux);
      interruptCounter--;
      portEXIT_CRITICAL(&timerMux);
      LOG_DEBUG("Timer Finished");
      return true;
    }
    return false;
}

void loop() {
  if (controller->getState() == Mode::HTTP_SERVER)
  {
    httpServer->handleClient();
    if (handleTimer())
    {
        controller->refreshDisplay();
    }
    delay(2);
  }
}