#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <configuration.h>
#include <SdFat.h>


class HttpServer{
  private:
    static WebServer server;
    Configuration* config;
  public:
    HttpServer(Configuration* Configuration);    
    static void handleWifiPage();
    bool serverStart();
    void handleClient();
    static void handleLocationSet();
};

class Fetcher
{
private:
    String time, temperature, icon, feelsLike, weather;
    static const String token;
    static const String URL;
    DynamicJsonDocument response = DynamicJsonDocument(50000);
    
public:
    Fetcher();
    static int validateLocation(String);
    int fetchResponse(String);
    void refreshWeatherData(String);
    String convertTime(int);
    String getTime();
    String getTemperature();
    String getIcon();
    String getFeelsLike();
    String getWeather();
};