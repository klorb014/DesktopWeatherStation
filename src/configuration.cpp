#include <Arduino.h>
#include <SdFat.h>
#include <logger.h>
#include <enum.h>
#include <configuration.h>


// SD CARD PINS
#define SD_CS 14
#define SRAM_CS 32


bool WiFiWrapper::connected(){
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiWrapper::wifiConnect(String network, String password)
{
  // We start by connecting to a WiFi network
  if (WiFi.status() == WL_CONNECTED)
  {
    LOG_INFO("Connected to network: " + String(WiFi.getHostname()));
    return true;
  }
  // Connect to your wi-fi modem
  WiFi.begin(network.c_str(), password.c_str());
  LOG_INFO("Attempting to connect to network: " + String(network));

  int retries = 0;
  const int maxRetries = 30;

  // Check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    if (retries < maxRetries)
    {
      delay(1000);
      Serial.print(".");
      retries++;
    }
    else
    {
      Serial.println();
      LOG_ERROR("Failed to connect to network: " + String(network));
      return false;
    }
  }
  Serial.println();
  LOG_INFO("Connected to network: " + String(network));
  Serial.println("WiFi connected successfully");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

IPAddress WiFiWrapper::getIP(){
    return WiFi.localIP();
}

//std::atomic<String> networkID{""};
//std::atomic<String> password{""}; 
//std::atomic<String> location{""};

SdFat Configuration::SD;

Configuration::Configuration(){};

Configuration::Configuration(String ssid, String pass)
{
    networkID = ssid;
    password = pass;
    network = WiFiWrapper();
    change = false;
}

void Configuration::setNetworkID(String ID)
{
    networkID = ID;
    change = true;
}
void Configuration::setNetworkPass(String pass)
{
    password = pass;
    change = true;
}
void Configuration::setLocation(String loc)
{
    location = loc;
    change = true;
}
String Configuration::getNetworkID(){return networkID;}
String Configuration::getNetworkPass(){return password;}
bool Configuration::connect(){return network.wifiConnect(networkID,password);}
bool Configuration::connected(){return network.connected();}
IPAddress Configuration::getIP(){return network.getIP();}
String Configuration::getLocation(){return location;}

bool Configuration::initializeSD()
{
    if (!Configuration::SD.begin(SD_CS, SD_SCK_MHZ(10)))
    { // Breakouts require 10 MHz limit due to longer wires
        LOG_ERROR("Fatal Error Initializing SD Card");
        return false;
    }
    LOG_INFO("SD Card Initialized");
    return true;
}

bool Configuration::available(Mode *state)
{
    File file = Configuration::SD.open("weather-station.conf", FILE_READ);
    if (!file)
    {
        // if the file didn't open, print an error:
        LOG_ERROR("error opening weather-station.conf");
        *state = Mode::ERROR;
        return false;
    }
    LOG_INFO("Checking if stored configuration");
    if (file.peek() == -1)
    {
        *state = Mode::ACCESS_POINT;
        return false;
    }
    LOG_INFO("Configuration Available");
    *state = Mode::LOADING;
    return true;
}

bool Configuration::load(Mode *state, Configuration *config)
{
    File file = Configuration::SD.open("weather-station.conf", FILE_READ);
    if (!file)
    {
        // if the file didn't open, print an error:
        LOG_ERROR("error opening weather-station.conf");
        return false;
    }

    LOG_INFO("Reading configuration from weather-station.conf...");
    String contents = "";
    char next;
    int line = 0;
    while (file.available()) // && line < 3
    {
      next = file.read();
      if (next == '\n')
      {
          //TODO: Read encrypted info
          contents.trim();
          switch (line)
          {
          case 0:
              config->setNetworkID(contents);
              break;
          case 1:
              config->setNetworkPass(contents);
              break;
          case 2:
              config->setLocation(contents);
              break;
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
    file.close(); 
    *state = Mode::LOADING;
    return true;
}

bool Configuration::store(Configuration* config)
{
    File file = Configuration::SD.open("weather-station.conf", FILE_WRITE);
    // if the file opened okay, write to it:
    if (!file)
    {
        // if the file didn't open, print an error:
        LOG_ERROR("error opening test.txt");
        return false;
    }
    //TODO: store encypted info
    LOG_INFO("Writing to weather-station.conf...");
    String network = config->getNetworkID();
    String password = config->getNetworkPass();
    String location = config->getLocation();
    file.println(network);
    file.println(password);
    file.println(location);
    // close the file:
    file.close();
    LOG_INFO("Finished writing to weather-station.conf");
    return true;
}