#include "SimpleRelay.h"
#include "JsonConfiguration.h"
#include "Logger.h"
#include "Mqtt.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
SimpleRelay::SimpleRelay(uint8_t pinRelay, const char* name)
{
    _pinRelay = pinRelay;
    strncpy(_name, name, sizeof(_name));
    pinMode(_pinRelay, OUTPUT);
    digitalWrite(_pinRelay, 0);
    _timeout = 0;
}

SimpleRelay::~SimpleRelay()
{
}

void SimpleRelay::setTimeout(float timeInSeconds)
{
    _timeout = timeInSeconds;
}

void SimpleRelay::setState(uint8_t state)
{
    digitalWrite(_pinRelay, state);

    Log.println("set " + String(_name) + " to " + String(state));
    MqttClient.publish(String(_name), String(state));

    _tickTimeout.detach();

    if (state) {
        if (_timeout != 0)
            _tickTimeout.once(_timeout, +[](SimpleRelay *inst) { inst->setState(0); }, this);
    }
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/