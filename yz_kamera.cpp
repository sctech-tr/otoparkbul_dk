#include <WiFi.h>
#include <HTTPClient.h>
// edge impulse kütüphanesi aşağıya yazılacak
#include <EI_KUTUPHANE_BURAYA_SAKIN_UNUTMAAAAAAAAAAAAAAAAAAAAAAAA.h> 
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"

// Deneyap Kart (OV2640) Kamera Pin Tanımları
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     32
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     15
#define Y9_GPIO_NUM       39
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       23
#define Y6_GPIO_NUM       18
#define Y5_GPIO_NUM       14
#define Y4_GPIO_NUM       12
#define Y3_GPIO_NUM       13
#define Y2_GPIO_NUM       21
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     19

// led pini
const int LED_PIN = A4; 

// Kamera ayarları ve bellek tamponu
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3
uint8_t *snapshot_buf;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

bool kameraBaslat() {
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Kamera başlatılamadı! Hata: 0x%x\n", err);
        return false;
    }
    return true;
}

static int goruntuVerisiAl(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + 
                              (snapshot_buf[pixel_ix + 1] << 8) + 
                              snapshot_buf[pixel_ix];
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    return 0;
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    if (!kameraBaslat()) {
        Serial.println("Kamera hatası, durduruluyor.");
        while (1);
    }
    Serial.println("Kamera ve Yapay Zeka Hazır!");
}

void loop() {
    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    if (snapshot_buf == nullptr) {
        Serial.println("Bellek hatasi!");
        return;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        free(snapshot_buf);
        return;
    }

    bool donusum = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    esp_camera_fb_return(fb);
    if (!donusum) {
        free(snapshot_buf);
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &goruntuVerisiAl;

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);

    if (err == EI_IMPULSE_OK) {
        bool arabaGoruldu = false;

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            // Edge Impulse'da verdiğimiz etiket adını kontrol eder
            if (String(result.classification[ix].label) == "araba" && result.classification[ix].value > 0.80) {
                arabaGoruldu = true;
                Serial.print("OYUNCAK ARABA TANINDI! Güven: %");
                Serial.println(result.classification[ix].value * 100);
            }
        }

        if (arabaGoruldu) {
            digitalWrite(LED_PIN, HIGH); // LED'i yak
        } else {
            digitalWrite(LED_PIN, LOW);  // LED'i söndür
        }
    }

    free(snapshot_buf);
    delay(100);
}
