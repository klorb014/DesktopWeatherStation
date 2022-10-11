#pragma once
#include <Arduino.h>
#include <SdFat.h>
#include <logger.h>
#include <WiFi.h>
#include <map>
#include <enum.h>

class WiFiWrapper{
  public:
    bool connected();
    bool wifiConnect(String network, String password);
    IPAddress getIP();
};

class Configuration
{
private:
    WiFiWrapper network;
    String networkID, password, location;
    //static std::atomic<String> networkID, password, location;
    bool change; 
public:
    Configuration();
    Configuration(String network, String pass);
    void setNetworkID(String ID);
    void setNetworkPass(String pass);
    void setLocation(String loc);
    bool connect();
    bool connected();
    IPAddress getIP();
    String getLocation();
    String getNetworkID();
    String getNetworkPass();

    static SdFat SD;
    static bool initializeSD();
    static bool available(Mode *state);
    static bool load(Mode *state, Configuration *config);
    static bool store(Configuration* config);
};