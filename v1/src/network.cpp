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

HttpServer::HttpServer(Configuration *Configuration)
{
  config = Configuration;
}

WebServer HttpServer::server(80);
const String HttpServer::AP_ssid = "DesktopWeatherStation";
String HttpServer::AP_pass = "123456789";

bool HttpServer::serverStart()
{
  server.on("/", HttpServer::handleWifiPage);
  server.on("/submit", handleWifiConnect);
  server.on("/submit_location", handleLocationSet);
  server.begin();
  LOG_INFO("HTTP server started");
  return true;
}

bool HttpServer::startAccessPoint(String *AP_password, bool generate)
{
  // Connect to Wi-Fi network with SSID and password
  LOG_DEBUG("Setting AP (Access Point)…");

  // Generate random password
  if (generate){
    *AP_password = String(esp_random());
  }

  HttpServer::AP_pass = *AP_password;

  // Remove the password parameter, if you want the AP (Access Point) to be open
  if (!WiFi.softAP(AP_ssid.c_str(), HttpServer::AP_pass.c_str()))
  {
    LOG_ERROR("Failed to start access point")
    return false;
  }
  return true;
}

void HttpServer::handleWifiPage()
{
  char temp[1000];
  if (WiFi.status() != WL_CONNECTED)
  {
    snprintf(temp, 1000,
             "<html>\
    <head>\
      <title>Desktop Weather Station</title>\
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
  }
  else
  {
    snprintf(temp, 1000,
             "<html>\
  <head>\
    <title>Desktop Weather Station</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Desktop Weather Station: Location Setup</h1>\
    <form action=\"/submit_location\" method=\"post\">\
    <label for=\"fname\">City:</label><br>\
    <input type=\"text\" id=\"city\" name=\"city\"><br>\
    <label for=\"country\">Country:</label><br>\
    <input type=\"country\" id=\"country\" name=\"country\"><br><br>\
    <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>");
  }
  server.send(200, "text/html", temp);
}

void HttpServer::handleClient()
{
  server.handleClient();
}

void HttpServer::handleWifiConnect()
{
  if (server.method() != HTTP_POST)
  {
    server.send(405, "text/html");
    return;
  }

  String tmp_ssid = "";
  String tmp_password = "";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "ssid")
    {
      tmp_ssid = server.arg(i);
    }
    else if (server.argName(i) == "password")
    {
      tmp_password = server.arg(i);
    }
  }

    // Try to connect to network
    WiFi.softAPdisconnect(true);

    if (!WiFiWrapper::wifiConnect(tmp_ssid, tmp_password))
    {
      HttpServer::startAccessPoint(&HttpServer::AP_pass, false);
      server.send(400, "text/html", "Bad Credentials");
      return;
    }

    Serial.println("");
      Serial.println("WiFi connected successfully");
      Serial.print("Got IP: ");
      Serial.println(WiFi.localIP());

      bool rc = Configuration::storeWiFiCredentials(tmp_ssid, tmp_password);
      server.send(200, "text/html");

      return;
}

void HttpServer::handleLocationSet()
{

  if (server.method() != HTTP_POST)
  {
    server.send(405, "text/html");
    return;
  }

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
  LOG_ERROR(requestedLocation);
  int httpCode = Fetcher::validateLocation(requestedLocation);
  LOG_ERROR(httpCode);
  if (httpCode != 200)
  {
    server.send(httpCode, "text/html");
  }
  else
  {
    LOG_INFO("User selected new location: " + requestedLocation);
    bool rc = Configuration::storeLocation(requestedLocation);
    server.send(200, "text/html");
  }
}

Fetcher::Fetcher(){
    // response = DynamicJsonDocument(50000);
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

void Fetcher::refreshWeatherData(String location)
{
  if (!fetchResponse(location) == 200)
  {
    weather = "ERROR OCCURRED";
    return;
  }

  time = convertTime(response["dt"]);

  String tmp = response["main"]["temp"];
  temperature = String((int)round(tmp.toFloat() - 273.15));

  icon = response["weather"][0]["icon"].as<String>();

  tmp = response["main"]["feels_like"].as<String>();
  feelsLike = String((int)round(tmp.toFloat() - 273.15));

  weather = response["weather"][0]["main"].as<String>();
  return;
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
  return String(hour_int) + ":" + minute_str;
}

String Fetcher::getTime() { return time; }
String Fetcher::getTemperature() { return temperature; }
String Fetcher::getIcon() { return icon; }
String Fetcher::getFeelsLike() { return feelsLike; }
String Fetcher::getWeather() { return weather; }