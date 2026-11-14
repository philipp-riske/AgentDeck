#pragma once

// reTerminal E1001 — Seeed Studio 7.5" monochrome e-paper terminal
// ESP32-S3 (8MB OPI PSRAM) + the same 800×480 1-bit panel as TRMNL 7.5"
// (Good Display GDEY075T7, UC8179 controller), so it shares the TRMNL 7.5"
// render tree. [env:reterminal_e1001] therefore defines BOARD_TRMNL_75 (the
// 7.5" paper family) as well as BOARD_RETERMINAL_E1001 (identity + pins);
// every identity switch tests BOARD_RETERMINAL_E1001 first.
//
// Pin map: Seeed wiki "reTerminal E10xx with Arduino" (GxEPD2 example) and
// Seeed_GFX User_Setups/Setup520_Seeed_reTerminal_E1001.h agree on the e-paper
// pins; buttons/LED/buzzer/battery from the Seeed peripheral cookbook.
//
// USB is a CH340K USB-UART bridge on UART0 (GPIO43/44), NOT the S3 native USB:
// measured 2026-11-14 as 1a86:7523. The env therefore builds with
// ARDUINO_USB_CDC_ON_BOOT=0 so `Serial` is UART0 and the daemon's
// device_info/provisioning path reaches the board through the CH340. The
// CH340 drives EN/IO0, so download mode is entered automatically by the
// flasher — no BOOT press needed once AgentDeck runs.

#define BOARD_NAME "reTerminal E1001 e-ink"

// E-paper SPI (GxEPD2_750_GDEY075T7). No MISO; SCK/MOSI are shared with the
// microSD slot (SD_CS=GPIO14), which AgentDeck does not power (SD_EN=GPIO16).
#define BOARD_PIN_EPD_SCK   7
#define BOARD_PIN_EPD_MOSI  9
#define BOARD_PIN_EPD_CS    10
#define BOARD_PIN_EPD_DC    11
#define BOARD_PIN_EPD_RST   12
#define BOARD_PIN_EPD_BUSY  13

// Front keys (active low, external pull-ups): KEY0=GPIO3 (green),
// KEY1=GPIO4, KEY2=GPIO5. The paper renderer reads two keys: primary = green.
#define BOARD_PIN_KEY1      3
#define BOARD_PIN_KEY2      4
#define BOARD_PIN_KEY3      5

// Peripherals not driven by AgentDeck yet (documented for the next port step)
#define BOARD_PIN_STATUS_LED 6   // active LOW
#define BOARD_PIN_BUZZER_PWM 45
#define BOARD_PIN_VBAT_EN   21   // HIGH ~5 ms before reading, divider ×2
#define BOARD_PIN_VBAT_ADC  1
#define BOARD_PIN_I2C_SDA   19   // SHT4x @0x44, PCF8563 RTC @0x51
#define BOARD_PIN_I2C_SCL   20
