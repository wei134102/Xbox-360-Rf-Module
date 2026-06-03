#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "xbox360_icon.h"

// ========== 引脚定义 / Pin Definitions ==========
#define RF_DATA_PIN 3
#define RF_CLOCK_PIN 2
#define RF_SYNC_PIN 7
#define OLED_SDA 4
#define OLED_SCL 5
#define BUTTON_PIN 6

// 微雪 RP2040-Zero 板载 WS2812，接 GP16（勿外接其它设备到此脚）
#define RGB_PIN 16

#define CLOCK_WAIT_US 50000UL

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

int SYNC[11] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1};
int LED_INIT[10] = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0};
int BOOT_ANIM[10] = {0, 0, 1, 0, 0, 0, 0, 1, 0, 1};

int paired_count = 0;
bool rf_bus_ok = false;
bool last_pair_pressed = false;

enum RgbState : uint8_t {
  RGB_BOOT,
  RGB_READY,
  RGB_PAIRING,
  RGB_PAIRED,
  RGB_ERROR,
};

RgbState rgb_state = RGB_BOOT;
uint32_t rgb_anim_ms = 0;
uint8_t rgb_r = 0, rgb_g = 0, rgb_b = 0;

// ---- 板载 WS2812 bit-bang（GRB，单颗）----
static inline void wsDelayCycles(volatile uint32_t n) {
  while (n--) {
    __asm volatile("nop");
  }
}

static void ws2812Write(uint8_t r, uint8_t g, uint8_t b) {
  const uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
  pinMode(RGB_PIN, OUTPUT);
  digitalWrite(RGB_PIN, LOW);
  noInterrupts();
  for (int bit = 23; bit >= 0; bit--) {
    digitalWrite(RGB_PIN, HIGH);
    if (grb & (1UL << bit)) {
      wsDelayCycles(6);
      digitalWrite(RGB_PIN, LOW);
      wsDelayCycles(6);
    } else {
      wsDelayCycles(3);
      digitalWrite(RGB_PIN, LOW);
      wsDelayCycles(9);
    }
  }
  interrupts();
  delayMicroseconds(80);
}

static void rgbSet(uint8_t r, uint8_t g, uint8_t b) {
  rgb_r = r;
  rgb_g = g;
  rgb_b = b;
  ws2812Write(r, g, b);
}

static void rgbInit() {
  rgbSet(0, 0, 0);
}

static void rgbSetState(RgbState state) {
  rgb_state = state;
  rgb_anim_ms = millis();
  switch (state) {
    case RGB_BOOT:
      rgbSet(0, 0, 40);
      break;
    case RGB_READY:
      rgbSet(0, 24, 0);
      break;
    case RGB_PAIRING:
      rgbSet(0, 0, 60);
      break;
    case RGB_PAIRED:
      rgbSet(0, 80, 0);
      break;
    case RGB_ERROR:
      rgbSet(80, 0, 0);
      break;
  }
}

static void rgbTick() {
  const uint32_t now = millis();
  switch (rgb_state) {
    case RGB_BOOT:
      if ((now - rgb_anim_ms) > 400) {
        rgb_anim_ms = now;
        static bool boot_on = false;
        boot_on = !boot_on;
        rgbSet(0, 0, boot_on ? 50 : 8);
      }
      break;
    case RGB_PAIRING:
      if ((now - rgb_anim_ms) > 120) {
        rgb_anim_ms = now;
        static bool pair_on = false;
        pair_on = !pair_on;
        rgbSet(0, 0, pair_on ? 90 : 0);
      }
      break;
    case RGB_PAIRED:
      if ((now - rgb_anim_ms) > 1500) {
        rgbSetState(rf_bus_ok ? RGB_READY : RGB_ERROR);
      }
      break;
    default:
      break;
  }
}

static void rfBusInit() {
  pinMode(RF_DATA_PIN, INPUT);
  pinMode(RF_CLOCK_PIN, INPUT_PULLUP);
  pinMode(RF_SYNC_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

static bool waitClockEdge(int previous, uint32_t timeout_us) {
  uint32_t start = micros();
  while (digitalRead(RF_CLOCK_PIN) == previous) {
    if ((micros() - start) > timeout_us) {
      return false;
    }
  }
  return true;
}

template <size_t N>
bool sendData(int (&command)[N], int delay_ms) {
  pinMode(RF_DATA_PIN, OUTPUT);
  digitalWrite(RF_DATA_PIN, LOW);

  int previous_clock = digitalRead(RF_CLOCK_PIN);
  for (size_t i = 0; i < N; i++) {
    if (!waitClockEdge(previous_clock, CLOCK_WAIT_US)) {
      Serial.println(F("RF: clock timeout (check GP2->Pin7, 3.3V, pull-up)"));
      digitalWrite(RF_DATA_PIN, HIGH);
      pinMode(RF_DATA_PIN, INPUT);
      return false;
    }
    previous_clock = digitalRead(RF_CLOCK_PIN);
    digitalWrite(RF_DATA_PIN, command[i]);
    if (!waitClockEdge(previous_clock, CLOCK_WAIT_US)) {
      Serial.println(F("RF: clock timeout on bit"));
      digitalWrite(RF_DATA_PIN, HIGH);
      pinMode(RF_DATA_PIN, INPUT);
      return false;
    }
    previous_clock = digitalRead(RF_CLOCK_PIN);
  }

  digitalWrite(RF_DATA_PIN, HIGH);
  pinMode(RF_DATA_PIN, INPUT);
  delay(delay_ms);
  return true;
}

static bool pairButtonPressed() {
  return (digitalRead(RF_SYNC_PIN) == LOW) || (digitalRead(BUTTON_PIN) == LOW);
}

static void drawControllerIcon() {
  u8g2.drawVLine(XBOX360_ICON_X - 3, XBOX360_ICON_Y, XBOX360_ICON_H);
  u8g2.drawXBMP(XBOX360_ICON_X, XBOX360_ICON_Y, XBOX360_ICON_W, XBOX360_ICON_H, xbox360_icon);
}

void showStatus(const char* msg) {
  u8g2.clearBuffer();

  u8g2.setClipWindow(0, 0, XBOX360_ICON_X - 5, 63);
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, "Xbox360 RF");
  u8g2.drawStr(0, 28, msg);
  char buf[24];
  snprintf(buf, sizeof(buf), "Paired: %d", paired_count);
  u8g2.drawStr(0, 44, buf);
  if (!rf_bus_ok) {
    u8g2.drawStr(0, 56, "RF bus ERR");
  }
  u8g2.setMaxClipWindow();

  drawControllerIcon();
  u8g2.sendBuffer();
}

static void doPair() {
  Serial.println(F("Pair: sending SYNC"));
  rgbSetState(RGB_PAIRING);
  showStatus("Pairing...");
  if (sendData(SYNC, 50)) {
    paired_count++;
    rgbSetState(RGB_PAIRED);
    showStatus("Paired!");
    Serial.println(F("Pair: SYNC sent OK"));
  } else {
    rgbSetState(RGB_ERROR);
    showStatus("RF Error!");
    Serial.println(F("Pair: SYNC failed"));
  }
  delay(800);
  while (pairButtonPressed()) {
    delay(10);
  }
  delay(200);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("Xbox360 RF RP2040 boot"));

  rgbInit();
  rgbSetState(RGB_BOOT);

  rfBusInit();
  delay(3000);

  u8g2.begin();
  showStatus("Init RF...");

  rf_bus_ok = sendData(LED_INIT, 50);
  delay(50);
  if (rf_bus_ok) {
    rf_bus_ok = sendData(LED_INIT, 7000);
  }
  if (rf_bus_ok) {
    sendData(BOOT_ANIM, 50);
    Serial.println(F("RF: LED init OK (center LED on is normal)"));
    showStatus("Ready GP7=Pair");
    rgbSetState(RGB_READY);
  } else {
    Serial.println(F("RF: init failed - see wiring"));
    showStatus("Clock/Data ERR");
    rgbSetState(RGB_ERROR);
  }
}

void loop() {
  rgbTick();

  bool pressed = pairButtonPressed();
  if (pressed && !last_pair_pressed) {
    doPair();
  }
  last_pair_pressed = pressed;

  static uint32_t last_ui = 0;
  if (millis() - last_ui > 500) {
    last_ui = millis();
    showStatus(rf_bus_ok ? "GP7: Pair" : "Fix GP2/3");
  }
}
