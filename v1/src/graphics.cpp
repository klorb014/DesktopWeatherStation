#include <Arduino.h>
#include <Adafruit_EPD.h>
#include <Adafruit_GFX.h>
#include <SdFat.h>
#include <Adafruit_ImageReader_EPD.h>
#include <Fonts/FreeSans9pt7b.h>
#include <graphics.h>
#include <logger.h>

// Battery Settings
#define VBATPIN A13
const int MAX_ANALOG_VAL = 4095;
const float MAX_BATTERY_VOLTAGE = 4.2;
const float PROTECTION_VOLTAGE = 3.2;
#define COLOR EPD_BLACK

Graphics::Graphics(Adafruit_SSD1675 *d, Adafruit_ImageReader_EPD *r)
{
    display = d;
    display->begin();
    reader = r;
}

Graphics::ImageMap Graphics::ImageMap_ = {
    {"01d", "/01d.bmp"},
    {"02d", "/02d.bmp"},
    {"03d", "/03d.bmp"},
    {"04d", "/04d.bmp"},
    {"09d", "/09d.bmp"},
    {"10d", "/10d.bmp"},
    {"11d", "/11d.bmp"},
    {"13d", "/13d.bmp"},
    {"50d", "/50d.bmp"},
    {"01n", "/01d.bmp"},
    {"02n", "/02d.bmp"},
    {"03n", "/03d.bmp"},
    {"04n", "/04d.bmp"},
    {"09n", "/09d.bmp"},
    {"10n", "/10d.bmp"},
    {"11n", "/11d.bmp"},
    {"13n", "/13d.bmp"},
    {"50n", "/50d.bmp"},
};

void Graphics::displayIP(String ip)
{
    display->clearBuffer();
    display->setCursor(0, 0);
    display->setTextColor(COLOR);
    display->setTextSize(2);
    display->setTextWrap(true);
    display->print(ip);
    display->display();
}

void Graphics::displayAccessPoint(String ssid, String password, String ip)
{
    display->clearBuffer();
    display->setCursor(0, 0);
    display->setTextColor(COLOR);
    display->setTextSize(2);
    display->setTextWrap(true);
    String message = "Weather Station\n\n";
    display->print(message);
    display->setTextSize(1);
    message = "Configure System At:\n\nssid: " + ssid + "\n\npassword: " + password + "\n\nip: " + ip + "/";
    display->print(message);
    display->display();
}

void Graphics::displayWeatherData(String location,
                                  String time,
                                  String temperature,
                                  String icon,
                                  String feelsLike,
                                  String weather)
{
    LOG_DEBUG("[" + location + "]" + "[" + time + "]" + "[" + temperature + "]" + "[" + icon + "]" + "[" + feelsLike + "]" + "[" + weather + "]");
    display->clearBuffer();
    display->setCursor(0, 0);
    display->setTextColor(COLOR);
    display->setTextSize(2);
    display->setTextWrap(true);
    display->print(location + "\n");
    display->print(time);
    display->setTextSize(4);
    display->setCursor(55, 45);
    display->print(temperature + "C\n");
    ImageReturnCode stat; // Status from image-reading functions
    ImageMap::iterator it = ImageMap_.find(icon);
    if (it == ImageMap_.end())
    {
        LOG_ERROR("Icon does not exist");
        return;
    }

    char *imgFile = it->second;
    stat = reader->drawBMP(imgFile, *display, 0, 35);
    if (stat == IMAGE_SUCCESS)
    {
        LOG_INFO("BMP Drawn Successfully");
    }
    else if (stat == IMAGE_ERR_FILE_NOT_FOUND)
    {
        LOG_ERROR("File not found.");
    }
    else if (stat == IMAGE_ERR_FORMAT)
    {
        LOG_ERROR("Not a supported BMP variant.");
    }
    else if (stat == IMAGE_ERR_MALLOC)
    {
        LOG_ERROR("Malloc failed (insufficient RAM).");
    }
    display->setCursor(0, 85);
    display->setTextSize(2);
    display->print("feels like " + feelsLike + "C\n");
    // display->setCursor(0, 85);
    display->setTextSize(3);
    display->print(weather + "\n");
    // displayBatteryLife();
    display->display();
}

void Graphics::displayBatteryLife()
{
    display->setCursor(220, 0);
    display->setTextSize(1);

    int rawValue = analogRead(VBATPIN);
    float voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3; // calculate voltage level
    int batteryFraction = (int)round(((voltageLevel - PROTECTION_VOLTAGE) / (MAX_BATTERY_VOLTAGE - PROTECTION_VOLTAGE)) * 100);
    display->print((String) "(" + batteryFraction + "%)");

    display->setCursor(200, 10);
    display->setTextSize(1);
    display->print((String) "(" + voltageLevel + ")");
    display->display();
}