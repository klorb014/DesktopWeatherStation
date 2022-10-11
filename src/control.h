#pragma once
#include <Arduino.h>
#include <SdFat.h>
#include <network.h>
#include <logger.h>
#include <graphics.h>
#include <map>
#include <configuration.h>
#include <enum.h>

class Controller
{
private:
    Configuration *config;
    Mode state;
    HttpServer *server;
    Graphics *graphics;
    bool lowPowerMode();
    bool handleStateChange(Mode state);
public:
    Controller(HttpServer*, Configuration*, Graphics*);
    void stateChange(Mode newState);
    void refreshDisplay();
    Mode getState();
    Fetcher fetcher;
    typedef std::map<Mode, String> StateMap;
    static StateMap StateMap_;
};