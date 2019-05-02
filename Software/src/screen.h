#pragma once

#include "ATM90E32.h"
#include <time.h>

class screen
{
  public:
    screen();
    virtual ~screen();

    void setup(void);
    void handle(void);
    void clear(void);
    void connecting_to_wifi(void);
    void wifi_done(void);
    void display_menu(time_t *now);
    void display_metering(metering data, String str);
    void display_relay_status(bool enable);

  protected:
    void commit(void);
    int8_t getWifiQuality();
    void drawWifiQuality();

  private:
    uint8_t _state;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern screen Screen;
#endif