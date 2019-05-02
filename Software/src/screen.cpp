#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "screen.h"
#include "Adafruit_ST7735.h"
// #include "Adafruit_ST7789.h"
#include "JsonConfiguration.h"
#include "settings.h"

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 tftBuf = GFXcanvas16(ST7735_TFTWIDTH_128, ST7735_TFTHEIGHT_128);

#define SIZE_MENU_TOP 15    // in pixels
#define SIZE_MENU_BOTTOM 10 // in pixels

// Pick color from: https://ee-programming-notepad.blogspot.com/2016/10/16-bit-color-generator-picker.html
#define COLOR_GREY 0xD69A

enum EStateDisplay
{
  st_DisplayPhaseA = 0,
  st_DisplayPhaseB,
  st_DisplayPhaseC
};

/*********************************/
/********* PUBLIC METHOD *********/
/*********************************/

screen::screen()
{
}

screen::~screen()
{
}

void screen::setup(void)
{
  tft.initR(INITR_144GREENTAB); // initialize ST7735S chip, green tab
  // tft.init(240, 240);
  tft.setTextWrap(true);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.drawRect(0, 35, tft.width(), 45, ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(45, 40);
  tft.print("ESP");
  tft.setCursor(5, 60);
  tft.print("Monitoring");
  tft.setTextSize(1);
  tft.setCursor(0, 120);
  tft.println("V1.0    by Yannick R.");
  delay(1000);
  _state = st_DisplayPhaseA;
}

void screen::handle()
{
  static unsigned long oldMillis;

  if ((millis() - oldMillis) >= (unsigned long)(Configuration._timeUpdateScreen * 1000))
  {
    oldMillis = millis();

    /* Draw wifi quality */
    drawWifiQuality();

    if (Configuration._mode == MODE_MONO)
    {

      /* Update data from ATM90E32 */
      Monitoring.handle();

      switch (_state)
      {
      /* Display Phase A */
      case st_DisplayPhaseA:
        display_metering(Monitoring.getLineA(), Configuration._namePhaseA);
        _state = st_DisplayPhaseB;
        break;

      /* Display Phase B */
      case st_DisplayPhaseB:
        display_metering(Monitoring.getLineB(), Configuration._namePhaseB);
        _state = st_DisplayPhaseC;
        break;

      /* Display Phase C */
      case st_DisplayPhaseC:
        display_metering(Monitoring.getLineC(), Configuration._namePhaseC);
        _state = st_DisplayPhaseA;
        break;

      default:
        _state = st_DisplayPhaseA;
        break;
      }
    }
    else if (Configuration._mode == MODE_TRI)
    {
      /* Update data from ATM90E32 */
      Monitoring.handle();

      /* Display data */
      display_metering(Monitoring.getLineA(), Configuration._namePhaseA);
    }
    
    /* Apply to screen */
    commit();
  }
}

void screen::clear(void)
{
  tft.fillScreen(ST77XX_BLACK);
}

void screen::connecting_to_wifi(void)
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(4, 40);
  tft.print("Connecting  to Wifi");
}

void screen::wifi_done(void)
{
  tft.setTextSize(1);
  tft.setCursor(0, 100);
  tft.println(WiFi.SSID());
  tft.println(WiFi.localIP().toString());
}

void screen::display_menu(time_t *now)
{
  // Get current date time
  static struct tm oldTm;
  char txt[30];
  struct tm *timeinfo = localtime(now);

  /* Update time every minutes */
  if (oldTm.tm_min == timeinfo->tm_min) return;
  
  oldTm = *timeinfo;
  Serial.println("Update Time on screen");

  /* Clear menu */
  tftBuf.fillRect(0, 0, tft.width()-10, SIZE_MENU_TOP, ST77XX_BLACK);

  /* Write Date */
  tftBuf.setTextColor(COLOR_GREY);
  tftBuf.setTextSize(1);
  tftBuf.setCursor(0, 1);
  sprintf(txt, "%02d/%02d/%02d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year - 100);
  tftBuf.print(txt);

  /* Write Time */
  tftBuf.setTextColor(COLOR_GREY);
  tftBuf.setTextSize(1);
  tftBuf.setCursor(68, 1);
  sprintf(txt, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  tftBuf.print(txt);

  tftBuf.drawFastHLine(0, 10, tft.width(), COLOR_GREY);

  commit();
}

void screen::display_metering(metering data, String str)
{
  char txt[10];

  /* Clear screen */
  tftBuf.fillRect(0, SIZE_MENU_TOP, tft.width(), tft.height() - SIZE_MENU_TOP - SIZE_MENU_BOTTOM, ST77XX_BLACK);

  /* Write str center */
  tftBuf.setTextColor(ST77XX_WHITE);
  tftBuf.setCursor(20, SIZE_MENU_TOP + 3);
  tftBuf.setTextSize(2);
  tftBuf.println(str);

  /* Write Voltage */
  tftBuf.setTextColor(ST77XX_BLUE);
  tftBuf.setCursor(6, SIZE_MENU_TOP + 25);
  tftBuf.setTextSize(1);
  tftBuf.print("Tension");
  if (data.voltage >= 100)
    tftBuf.setCursor(6, SIZE_MENU_TOP + 35);
  else if (data.voltage >= 10)
    tftBuf.setCursor(11, SIZE_MENU_TOP + 35);
  else
    tftBuf.setCursor(16, SIZE_MENU_TOP + 35);
  tftBuf.setTextSize(2);
  sprintf(txt, "%.0fV", data.voltage);
  tftBuf.print(txt);

  /* Write Current */
  tftBuf.setTextColor(ST77XX_RED);
  tftBuf.setCursor(72, SIZE_MENU_TOP + 25);
  tftBuf.setTextSize(1);
  tftBuf.print("Courant");
  tftBuf.setTextSize(2);
  if (data.current >= 100)
  {
    tftBuf.setCursor(72, SIZE_MENU_TOP + 35);
    sprintf(txt, "%.0fA", data.current);
  }
  else if (data.current >= 10)
  {
    tftBuf.setCursor(65, SIZE_MENU_TOP + 35);
    sprintf(txt, "%.1fA", data.current);
  }
  else
  {
    tftBuf.setCursor(65, SIZE_MENU_TOP + 35);
    sprintf(txt, "%.2fA", data.current);
  }
  tftBuf.print(txt);

  /* Write Power */
  tftBuf.setTextColor(ST77XX_GREEN);
  tftBuf.setCursor(35, SIZE_MENU_TOP + 60);
  tftBuf.setTextSize(1);
  tftBuf.print("Puissance");
  if (data.power >= 10000)
    tftBuf.setCursor(10, SIZE_MENU_TOP + 70);
  else if (data.power >= 1000)
    tftBuf.setCursor(20, SIZE_MENU_TOP + 70);
  else if (data.power >= 100)
    tftBuf.setCursor(30, SIZE_MENU_TOP + 70);
  else if (data.power >= 10)
    tftBuf.setCursor(40, SIZE_MENU_TOP + 70);
  else
    tftBuf.setCursor(50, SIZE_MENU_TOP + 70);
  tftBuf.setTextSize(3);
  sprintf(txt, "%.0fW", data.power);
  tftBuf.print(txt);
}

void screen::display_relay_status(bool enable)
{
  /* Clear menu */
  tftBuf.fillRect(0, tftBuf.height() - SIZE_MENU_BOTTOM, tftBuf.width(), tftBuf.height(), ST77XX_BLACK);

  /* Write Text */
  tftBuf.setTextColor(COLOR_GREY);
  tftBuf.setTextSize(1);
  tftBuf.setCursor(0, tft.height() - SIZE_MENU_BOTTOM);
  tftBuf.print("Relais: ");

  /* Write Status */
  if (enable)
  {
    tftBuf.setTextColor(ST77XX_GREEN);
    tftBuf.print("ON");
  }
  else
  {
    tftBuf.setTextColor(ST77XX_RED);
    tftBuf.print("OFF");
  }

  commit();
}

/**********************************/
/********* PRIVATE METHOD *********/
/**********************************/
void screen::commit(void)
{
  tft.drawRGBBitmap(0, 0, tftBuf.getBuffer(), tftBuf.width(), tftBuf.height());
}

// converts the dBm to a range between 0 and 100%
int8_t screen::getWifiQuality()
{
  static int32_t dbm = 0;
  dbm = (dbm + WiFi.RSSI()) / 2;
  if (dbm <= -100)
  {
    return 0;
  }
  else if (dbm >= -50)
  {
    return 100;
  }
  else
  {
    return 2 * (dbm + 100);
  }
}

void screen::drawWifiQuality()
{
  int8_t quality = getWifiQuality();

  /* Clear old wifi quality */
  tftBuf.fillRect(tftBuf.width() - 10, 0, 10, 10, ST77XX_BLACK);

  //   tft.print(String(quality) + "%");
  for (int8_t i = 0; i < 4; i++)
  {
    for (int8_t j = 0; j < 2 * (i + 1); j++)
    {
      if (quality >= i * 25 || j == 0)
      {
        tftBuf.drawPixel(tftBuf.width() - 8 + 2 * i, 7 - j, COLOR_GREY);
      }
    }
  }
}

#if !defined(NO_GLOBAL_INSTANCES)
screen Screen;
#endif
