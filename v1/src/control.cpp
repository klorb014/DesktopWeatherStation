#include <Arduino.h>
#include <control.h>
#include <SdFat.h>
#include <network.h>
#include <logger.h>
#include <graphics.h>
#include <map>

Controller::Controller(HttpServer *httpServer, Configuration *configuration, Graphics *g)
{
    state = Mode::BOOT;
    server = httpServer;
    config = configuration;
    graphics = g;
    fetcher = Fetcher();
}

Controller::StateMap Controller::StateMap_ = {
    {Mode::BOOT, "BOOT"},
    {Mode::LOW_POWER, "LOW_POWER"},
    {Mode::ACCESS_POINT, "ACCESS_POINT"},
    {Mode::HTTP_SERVER, "HTTP_SERVER"},
    {Mode::LOADING, "LOADING"},
    {Mode::ERROR, "ERROR"}};

Mode Controller::getState() { return state; }

bool Controller::lowPowerMode()
{
    // Print the wakeup reason for ESP32
    // print_wakeup_reason();

    // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_MIN_FACTOR);
    // LOG_INFO("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +" Minutes");
    LOG_INFO("Going to sleep now");
    delay(1000);
    Serial.flush();
    esp_deep_sleep_start();
}

bool Controller::handleStateChange(Mode state)
{
    bool rc = false;
    String AP_pass;
    switch (state)
    {
    case (Mode::LOW_POWER):
        LOG_INFO("Entering low power mode");
        rc = lowPowerMode();
        break;
    case (Mode::HTTP_SERVER):
        LOG_INFO("Starting HTTP Server");
        rc = server->serverStart();
        break;
    case (Mode::ACCESS_POINT):
        LOG_ERROR("Starting Access Point");
        rc = HttpServer::startAccessPoint(&AP_pass);
        config->setAPPass(AP_pass);
        break;
    default:
        rc = false;
        break;
    }
    return rc;
}

void Controller::stateChange(Mode newState)
{
    String next = StateMap_.find(newState)->second;
    String prev = StateMap_.find(state)->second;
    if (newState == Mode::ERROR)
    {
        LOG_ERROR("State: {" + prev + "} ==> {" + next + "}");
        state = newState;
        return;
    }

    if (handleStateChange(newState))
    {
        LOG_INFO("State: {" + prev + "} ==> {" + next + "}");
        state = newState;
        return;
    }

    LOG_ERROR("Failed to handle state change: {" + prev + "} ==> {" + next + "}");
}

void Controller::refreshDisplay()
{
    if (state == Mode::ACCESS_POINT)
    {
        LOG_INFO("Displaying Welcome Screen");
        String ssid = config->getAPSSID();
        String password = config->getAPPass();
        String ip = getIP().toString();
        graphics->displayAccessPoint(ssid, password, ip);
        return;
    }
    String location = config->getLocation();
    if (location.isEmpty())
    {
        LOG_INFO("Displaying IP");
        String ip = String(getIP());
        graphics->displayIP(ip);
    }
    else
    {
        LOG_INFO("Fetching weather data");
        fetcher.refreshWeatherData(location);
        graphics->displayWeatherData(location,
                                     fetcher.getTime(),
                                     fetcher.getTemperature(),
                                     fetcher.getIcon(),
                                     fetcher.getFeelsLike(),
                                     fetcher.getWeather());
    }
}

IPAddress Controller::getIP()
{
    return config->getIP(state);
}