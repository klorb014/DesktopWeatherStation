#include <Arduino.h>
#include <network_secrets.h>
#include <logger.h>
#include <SdFat.h>
#include <Adafruit_EPD.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_ImageReader_EPD.h>
#include <string>
#include <sstream>

#define SRAM_CS 32
#define EPD_CS 15
#define EPD_DC 33
#define LEDPIN 13
#define LEDPINON HIGH
#define LEDPINOFF LOW
#define COLOR EPD_BLACK

#define EPD_RESET -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1  // can set to -1 to not use a pin (will wait a fixed delay)

#define SD_CS 14
SdFat SD;
Adafruit_ImageReader_EPD reader(SD); // Image-reader object, pass in SD filesys
Adafruit_SSD1675 display(250, 122, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

HTTPClient http;

String SERVER_URL = "http://192.168.0.29:5000/api/key";
// String SERVER_URL = "http://192.168.0.29:5000/";
String TMP_FILE = "tmp.bmp";

bool wifi_connect()
{
    String ssid = SECRET_SSID;
    String pass = SECRET_PASSWORD;
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Connecting to ");
        Serial.println(ssid);

        WiFi.begin(ssid.c_str(), pass.c_str());
        int timeout = 30;

        while (WiFi.status() != WL_CONNECTED)
        {
            if (timeout > 0)
            {
                digitalWrite(13, HIGH);
                delay(1000);
                digitalWrite(13, LOW);
                Serial.print(".");
                delay(2000);
                timeout = timeout - 3;
            }
            else
            {
                WiFi.begin(ssid.c_str(), pass.c_str());
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

void setup()
{
    delay(10000);
    Serial.begin(115200);
    while (!Serial && !Serial.available())
    {
    }
    Serial.print(F("Initializing filesystem..."));
    pinMode(13, OUTPUT);

    // SD card is pretty straightforward, a single call...
    if (!SD.begin(SD_CS, SD_SCK_MHZ(10)))
    { // Breakouts require 10 MHz limit due to longer wires
        Serial.println(F("SD begin() failed"));
        for (;;)
            ; // Fatal error, do not continue
    }

    display.begin();

    Serial.println(F("OK!"));

    wifi_connect();
}



bool fetch_display_data(std::string &error_msg)
{
    std::stringstream ss;
    bool rc = false;
    Serial.println(SERVER_URL);
    SD.remove(TMP_FILE.c_str());
    File f = SD.open(TMP_FILE, FILE_WRITE);
    if (f)
    {
        http.begin(SERVER_URL);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK)
            {
                http.writeToStream(&f);
                rc = true;
            }
        }
        else
        {
            ss << "[HTTP] GET error: " << http.errorToString(httpCode).c_str();
            error_msg = ss.str();
            Serial.println(error_msg.c_str());
        }
        f.close();
    }
    else
    {
        Serial.printf("Failed to open file\n");
    }
    http.end();
    return rc;
}

bool refresh_display()
{
    bool rc = false;
    display.clearBuffer();

    ImageReturnCode stat = reader.drawBMP(strdup(TMP_FILE.c_str()), display, 0, 0);
    if (stat == IMAGE_SUCCESS)
    {
        rc = true;
        Serial.println("BMP Drawn Successfully");
    }
    else if (stat == IMAGE_ERR_FILE_NOT_FOUND)
    {
        Serial.println("File not found.");
    }
    else if (stat == IMAGE_ERR_FORMAT)
    {
        Serial.println("Not a supported BMP variant.");
    }
    else if (stat == IMAGE_ERR_MALLOC)
    {
        Serial.println("Malloc failed (insufficient RAM).");
    }

    display.display();
    return rc;
}

void display_msg(std::string msg)
{
    display.clearBuffer();
    display.setCursor(0, 0);
    display.setTextColor(COLOR);
    display.setTextSize(2);
    display.setTextWrap(true);
    display.print(msg.c_str());
    display.display();
}

void loop()
{
    std::string error_msg;
    if (fetch_display_data(error_msg))
    {
        refresh_display();
    }
    else
    {
        display_msg(error_msg);
    }

    sleep(10 * 60);
}