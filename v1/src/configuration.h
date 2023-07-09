#pragma once
#include <Arduino.h>
#include <SdFat.h>
#include <WiFi.h>
#include <map>
#include <enum.h>

class WiFiWrapper
{
public:
    bool connected();
    static bool wifiConnect(String network, String password);
    IPAddress getIP(Mode);
    String getAPSSID();
};

class Configuration
{
private:
    WiFiWrapper network;
    String networkID, password, AP_password, location;
    // static std::atomic<bool> change_;
    static bool change_;

public:
    Configuration();
    Configuration(String network, String pass);
    void setNetworkID(String ID);
    void setNetworkPass(String pass);
    void setAPPass(String pass);
    void setLocation(String loc);
    bool connect();
    bool connected();
    IPAddress getIP(Mode);
    String getLocation();
    String getNetworkID();
    String getNetworkPass();
    String getAPSSID();
    String getAPPass();

    static SdFat SD;
    static bool initializeSD();
    static bool available(Mode *state);
    static bool load(Mode *state, Configuration *config);
    static bool store(Configuration *config);
    static bool storeLocation(String location);
    static bool storeWiFiCredentials(String ssid, String password);
    static bool changeDetected();
    static bool peek();
};