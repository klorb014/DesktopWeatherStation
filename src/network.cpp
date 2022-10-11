#include <Arduino.h>
#include <network.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <HTTPClient.h>
#include <network_secrets.h>
#include <configuration.h>
#include <logger.h>


HttpServer::HttpServer(Configuration* Configuration){
  config = Configuration;
}

WebServer HttpServer::server(80);

bool HttpServer::serverStart(){
    server.on("/", HttpServer::handleWifiPage);
    //server.on("/submit", handleWifiConnect);
    server.on("/location", handleLocationSet);
    server.begin();
    LOG_INFO("HTTP server started");
    return true;
}

 void HttpServer::handleWifiPage()
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
  server.send(200, "text/html", temp);
}

void HttpServer::handleClient(){
  server.handleClient();
}

void HttpServer::handleLocationSet()
{

  String city = "";
  String country = "";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "city")
    {
      city = server.arg(i);
    }
    else if (server.argName(i) == "country")
    {
      country = server.arg(i);
    }
  }
  String requestedLocation = city + ", " + country;

  int httpCode = Fetcher::validateLocation(requestedLocation);
  if (httpCode != 200)
  {
    server.send(httpCode, "text/html");
  }
  else
  {
    LOG_INFO("User selected new location: " + requestedLocation);
    File file = Configuration::SD.open("location.conf", FILE_WRITE);
    // if the file opened okay, write to it:
    if (!file)
    {
        // if the file didn't open, print an error:
        LOG_ERROR("error opening location.conf");
    }
    file.println(requestedLocation);
    // close the file:
    file.close();
    
    server.send(200, "text/html");
  }
}


Fetcher::Fetcher()
{
  //response = DynamicJsonDocument(50000);
};

const String Fetcher::token = API_TOKEN;
const String Fetcher::URL = "http://api.openweathermap.org/data/2.5/weather?q=";

int Fetcher::validateLocation(String location)
{
  String url = URL + location + token;
  HTTPClient http;
  int httpCode = 0;
  LOG_INFO("validating location: " + location);
  if (http.begin(url))
  {
    // start connection and send HTTP header
    httpCode = http.GET();
    http.end();
  }
  return httpCode;
}

// Retrieve page response from given URL
int Fetcher::fetchResponse(String location)
{
  String url = URL + location + token;
  HTTPClient http;
  int httpCode = 0;
  LOG_INFO("getting url: " + url);
  if (http.begin(url))
  {
    // start connection and send HTTP header
    httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been sent and Server response header has been handled
      LOG_INFO("[HTTP] GET... code: " + String(httpCode));

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        DeserializationError error = deserializeJson(response, http.getString());
        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          LOG_ERROR(error.f_str());
        }
      }
    }
    else
    {
      LOG_ERROR("[HTTP] GET... failed, error: " + http.errorToString(httpCode));
    }
    http.end();
  }
  else
  {
    LOG_INFO("[HTTP] Unable to connect");
  }
  return httpCode;
}

void Fetcher::refreshWeatherData(String location){
  fetchResponse(location);

  time = convertTime(response["dt"]);

  String tmp = response["main"]["temp"];
  temperature = String((int)round(tmp.toFloat() - 273.15));

  icon = response["weather"][0]["icon"].as<String>();

  tmp = response["main"]["feels_like"].as<String>();
  feelsLike = String((int)round(tmp.toFloat() - 273.15));

  weather = response["weather"][0]["main"].as<String>();
}

String Fetcher::convertTime(int unixTime)
{
  int hour_int = (hour(unixTime) - 4);
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
  if (hour_int == 0)
  {
    hour_int = 12;
  }
  String minute_str;
  if (minute(unixTime) < 10)
  {
    minute_str = "0" + String(minute(unixTime));
  }
  else
  {
    minute_str = String(minute(unixTime));
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

String Fetcher::getTime(){return time;}
String Fetcher::getTemperature(){return temperature;}
String Fetcher::getIcon(){return icon;}
String Fetcher::getFeelsLike(){return feelsLike;}
String Fetcher::getWeather(){return weather;}