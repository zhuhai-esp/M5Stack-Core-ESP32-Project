#define LGFX_AUTODETECT

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>

static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static const int buf_size = screenWidth * 10;
static lv_color_t disp_draw_buf[buf_size];
static lv_color_t disp_draw_buf2[buf_size];

long check1s = 0;
long check5ms = 0;
LGFX tft;
char buf[128] = {0};

lv_obj_t *label_date;
lv_obj_t *label_ip;
lv_obj_t *label_time;
lv_obj_t *img_second;
lv_obj_t *img_minute;
lv_obj_t *img_hour;

LV_IMG_DECLARE(watch_bg);
LV_IMG_DECLARE(hour);
LV_IMG_DECLARE(minute);
LV_IMG_DECLARE(second);

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p) {
  tft.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1,
                   area->y2 - area->y1 + 1, (lgfx::rgb565_t *)&color_p->full);
  lv_disp_flush_ready(disp);
}

void ui_init(void) {
  auto *parent = lv_scr_act();

  label_date = lv_label_create(parent);
  lv_obj_align(label_date, LV_ALIGN_TOP_RIGHT, -4, 4);
  lv_label_set_text(label_date, "2000-01-01");

  label_ip = lv_label_create(parent);
  lv_obj_align(label_ip, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
  lv_obj_set_style_text_font(label_ip, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_label_set_text(label_ip, "IP: 0.0.0.0");

  label_time = lv_label_create(parent);
  lv_obj_align(label_time, LV_ALIGN_BOTTOM_LEFT, 4, -4);
  lv_label_set_text(label_time, "12:33:45");

  auto *panel = lv_img_create(parent);
  lv_img_set_src(panel, &watch_bg);
  lv_obj_set_align(panel, LV_ALIGN_CENTER);

  img_hour = lv_img_create(parent);
  lv_img_set_src(img_hour, &hour);
  lv_obj_set_align(img_hour, LV_ALIGN_CENTER);

  img_minute = lv_img_create(parent);
  lv_img_set_src(img_minute, &minute);
  lv_obj_set_align(img_minute, LV_ALIGN_CENTER);

  img_second = lv_img_create(parent);
  lv_img_set_src(img_second, &second);
  lv_obj_set_align(img_second, LV_ALIGN_CENTER);
}

inline void showClientIP() {
  sprintf(buf, "IP: %s", WiFi.localIP().toString().c_str());
  lv_label_set_text(label_ip, buf);
}

inline void showCurrentTime() {
  struct tm info;
  getLocalTime(&info);
  strftime(buf, 32, "%F", &info);
  lv_label_set_text(label_date, buf);
  strftime(buf, 32, "%T", &info);
  lv_label_set_text(label_time, buf);

  lv_img_set_angle(img_hour, info.tm_hour * 300 + info.tm_min / 12 % 12 * 60);
  lv_img_set_angle(img_minute, info.tm_min * 6 * 10);
  lv_img_set_angle(img_second, info.tm_sec * 6 * 10);
}

void inline startConfigTime() {
  const int timeZone = 8 * 3600;
  configTime(timeZone, 0, "ntp6.aliyun.com", "cn.ntp.org.cn", "ntp.ntsc.ac.cn");
  while (time(nullptr) < 8 * 3600 * 2) {
    delay(500);
  }
  sprintf(buf, "%s", WiFi.localIP().toString().c_str());
  tft.println(buf);
  struct tm info;
  getLocalTime(&info);
  strftime(buf, 64, "%c", &info);
  tft.println(buf);
  sleep(1);
}

inline void show_center_msg(const char *msg) {
  tft.drawCenterString(msg, screenWidth / 2, screenHeight / 2,
                       &fonts::AsciiFont8x16);
}

void inline autoConfigWifi() {
  tft.println("Start WiFi Connect!");
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin();
  for (int i = 0; WiFi.status() != WL_CONNECTED && i < 100; i++) {
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.beginSmartConfig();
    tft.println("Config WiFi with ESPTouch App!");
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
    }
    WiFi.stopSmartConfig();
    WiFi.mode(WIFI_MODE_STA);
  }
  tft.println("WiFi Connected, Please Wait...");
}

void inline setupOTAConfig() {
  ArduinoOTA.onStart([] { show_center_msg("OTA Update"); });
  ArduinoOTA.onProgress([](u32_t pro, u32_t total) {
    sprintf(buf, "OTA Updating: %d / %d", pro, total);
    show_center_msg(buf);
  });
  ArduinoOTA.onEnd([] { show_center_msg("OTA Sccess, Restarting..."); });
  ArduinoOTA.onError([](ota_error_t err) {
    sprintf(buf, "OTA Error [%d]!!", err);
    show_center_msg(buf);
  });
  ArduinoOTA.begin();
}

void inline initDisplay() {
  tft.init();
  tft.initDMA();
  tft.startWrite();
  tft.setBrightness(50);
  tft.setFont(&fonts::AsciiFont8x16);

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, disp_draw_buf2, buf_size);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
}

void setup() {
  Serial.begin(115200);
  initDisplay();
  autoConfigWifi();
  startConfigTime();
  setupOTAConfig();
  ui_init();
  showClientIP();
}

void loop() {
  auto ms = millis();
  if (ms - check1s > 1000) {
    check1s = ms;
    ArduinoOTA.handle();
    showCurrentTime();
  }
  if (ms - check5ms > 5) {
    check5ms = ms;
    lv_timer_handler();
  }
}