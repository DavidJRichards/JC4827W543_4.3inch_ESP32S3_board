/*******************************************************************************
Created by profi-max (Oleg Linnik) 2024
https://profimaxblog.ru
https://github.com/profi-max

*******************************************************************************/
#include "lv_demo_widgets.h"
#include <Arduino_GFX_Library.h>
#include "touch.h"

// FOR ARDUINO Uncomment the line below if you wish debug messages
//#define CORE_DEBUG_LEVEL 4

// FOR VSCODE see platformio.ini file -D CORE_DEBUG_LEVEL=1

/***************************************************************************************************
 * Arduino_GFX Setup for JC4827W543C
 ***************************************************************************************************/
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define SCR_BUF_LEN 32

// LCD backlight PWM 
#define LCD_BL 1           // lcd BL pin
#define LEDC_CHANNEL_0     0 // use first channel of 16 channels (started from zero)
#define LEDC_TIMER_12_BIT  12 // use 12 bit precission for LEDC timer
#define LEDC_BASE_FREQ     5000 // use 5000 Hz as a LEDC base frequency


Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
Arduino_NV3041A *panel = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
Arduino_GFX *gfx = new Arduino_Canvas(SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */, panel);
//------------------------------- End TFT+TOUCH setup ------------------------------

static const char* logTAG = "main.cpp";

/***************************************************************************************************
 * LVGL functions
 ***************************************************************************************************/

//=====================================================================================================================
void IRAM_ATTR my_disp_flush_(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  panel->startWrite();
  panel->setAddrWindow(area->x1, area->y1, w, h); 
  panel->writePixels((uint16_t *)&color_p->full, w * h);
  panel->endWrite();
  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}
//=====================================================================================================================
void IRAM_ATTR my_touchpad_read_(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}
//=====================================================================================================================
void IRAM_ATTR my_touchpad_feedback_(lv_indev_drv_t *indev_driver, uint8_t data)
{
  // Beep sound here
}
//=====================================================================================================================
// value has to be between 0 and 255
void setBrightness(uint8_t value)
{
  uint32_t duty = 4095 * value / 255;
  ledcWrite(LEDC_CHANNEL_0, duty);
}
//=====================================================================================================================

/***************************************************************************************************
 * LVGL Setup
 ***************************************************************************************************/

//=====================================================================================================================
void lvgl_init()
{
  static lv_disp_draw_buf_t draw_buf;
  static lv_disp_drv_t disp_drv;
  static lv_color_t disp_draw_buf[SCREEN_WIDTH * SCR_BUF_LEN];
  //static lv_color_t disp_draw_buf2[SCREEN_WIDTH * SCR_BUF_LEN];

  // Setting up the LEDC and configuring the Back light pin
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(LCD_BL, LEDC_CHANNEL_0);
  setBrightness(250);

   // Init Display
  if (!gfx->begin())
  {
    ESP_LOGI(logTAG, "gfx->begin() failed!\n");  
  }
  gfx->fillScreen(BLACK);

 
  touch_init(gfx->width(), gfx->height(), gfx->getRotation());
  lv_init(); 

  if (!disp_draw_buf)  ESP_LOGE(logTAG, "LVGL disp_draw_buf allocate failed!"); 
  else 
  {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, SCREEN_WIDTH * SCR_BUF_LEN);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush_;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read_;
    indev_drv.feedback_cb = my_touchpad_feedback_;
    lv_indev_drv_register(&indev_drv);
  }

   ESP_LOGI(logTAG, "Lvgl v%d.%d.%d initialized\n", lv_version_major(), lv_version_minor(), lv_version_patch());  
}
//=====================================================================================================================

void setup()
{
  Serial.begin(115200);
  lvgl_init();
  lv_demo_widgets();
  ESP_LOGI(logTAG, "Setup done");
}

//=====================================================================================================================
void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}

//=====================================================================================================================