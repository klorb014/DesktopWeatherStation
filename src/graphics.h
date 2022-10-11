#pragma once
#include <Arduino.h>
#include <Adafruit_EPD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ImageReader_EPD.h>
#include <map>

class Graphics{
    private:
        Adafruit_SSD1675* display;
        Adafruit_ImageReader_EPD* reader;
        Adafruit_Image_EPD img; // An image loaded into RAM
        int32_t width = 0;      // BMP image dimensions
        int32_t height = 0;
    public:
        Graphics(Adafruit_SSD1675*, Adafruit_ImageReader_EPD*);
        void displayWeatherData(String, String, String, String, String, String);
        void displayIP(String);
        void displayWelcome();
        void displayBatteryLife();
        typedef std::map<String, char*> ImageMap;
        static ImageMap ImageMap_;
};