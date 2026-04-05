#include "esp_camera.h"
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "FS.h"
#include "SD_MMC.h"

TFT_eSPI tft = TFT_eSPI();

#define BTN_CAPTURE 2
#define BTN_SAVE    4

bool frameCaptured = false;
camera_fb_t *savedFrame = NULL;
int photoCount = 0;

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void setupCamera() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_QQVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    while (1);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_CAPTURE, INPUT_PULLUP);
  pinMode(BTN_SAVE, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  TJpg_Decoder.setCallback(tft_output);

  setupCamera();

  SD_MMC.begin();
}

void loop() {
  if (digitalRead(BTN_CAPTURE) == LOW && !frameCaptured) {
    savedFrame = esp_camera_fb_get();
    frameCaptured = true;
    delay(500);
  }

  if (digitalRead(BTN_SAVE) == LOW && frameCaptured) {
    String path = "/photo_" + String(photoCount++) + ".jpg";

    File file = SD_MMC.open(path, FILE_WRITE);

    if (file) {
      file.write(savedFrame->buf, savedFrame->len);
      file.close();
    }

    esp_camera_fb_return(savedFrame);
    frameCaptured = false;

    delay(500);
  }

  if (!frameCaptured) {
    camera_fb_t * fb = esp_camera_fb_get();

    if (fb) {
      TJpg_Decoder.drawJpg(0, 0, fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }

  if (frameCaptured && savedFrame) {
    TJpg_Decoder.drawJpg(0, 0, savedFrame->buf, savedFrame->len);
  }
}
