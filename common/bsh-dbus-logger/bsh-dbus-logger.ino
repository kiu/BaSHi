/*
   bsh-dbus-logger.ino

   B/S/H/ D-Bus data logger

   (C) 2024 Hajo Noerenberg

   http://www.noerenberg.de/
   https://github.com/hn/bsh-home-appliances

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3.0 as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.txt>.
*/

/*
   Slightly adapted for use with https://github.com/kiu/BaSHi
*/

// Arduino IDE
// Library manager: Install "CRC" by Rob Tillaart https://github.com/RobTillaart/CRC
// Library manager: Install "FastLED" by Daniel Garcia https://github.com/FastLED/FastLED
// Boards manager: Install "esp32" https://github.com/espressif/arduino-esp32
// Config: Set "USB CDC on Boot" to "Enabled"

// Compatibility BaSHi-SM-1
// ESP32-C3-SUPER-MINI (Black)
// ESP32-C3-SUPER-MINI-PLUS (Red) [Unverified / LED needs to be configured below]
// ESP32-C6-SUPER-MINI (Black)
// ESP32-S3-SUPER-MINI (Black)

// Compatibility BaSHi-ZERO-1
// ESP32-C3-ZERO (Blue)
// ESP32-C6-ZERO (Blue)
// ESP32-S3-ZERO (Blue)

// RGB LED
// blue   = RX byte received
// green  = RX CRC ok
// red    = RX CRC fail
// purple = TX bye

// ---------------------------------------------------------------------------

#ifdef ARDUINO_ESP32C3_DEV
#define RX_PIN 10
#define TX_PIN 9
#define LED_TYPE 0  // Blue LED
#define LED_PIN 8
#define LED_ON LOW
#define LED_OFF HIGH
//#define LED_TYPE 1  // RGB LED SUPER_MINI_PLUS ONLY
//#define LED_PIN 8 // SUPER_MINI_PLUS ONLY
#endif

#ifdef ARDUINO_ESP32C6_DEV
#define RX_PIN 3
#define TX_PIN 2
//#define LED_TYPE 0 // Blue LED
//#define LED_PIN 15
//#define LED_ON HIGH
//#define LED_OFF LOW
#define LED_TYPE 1  // RGB LED
#define LED_PIN 8
#define LED_COLOR_ORDER GRB
#endif

#ifdef ARDUINO_ESP32S3_DEV
#define RX_PIN 4
#define TX_PIN 3
#define LED_TYPE 1  // RGB LED
#define LED_PIN 48
#define LED_COLOR_ORDER GRB
#endif

#ifdef ARDUINO_WAVESHARE_ESP32_C3_ZERO
#define RX_PIN 0
#define TX_PIN 1
#define LED_TYPE 1  // RGB LED
#define LED_PIN 10
#define LED_COLOR_ORDER GRB
#endif

#ifdef ARDUINO_WAVESHARE_ESP32_C6_ZERO
#define RX_PIN 0
#define TX_PIN 1
#define LED_TYPE 1  // RGB LED
#define LED_PIN 8
#define LED_COLOR_ORDER RGB
#endif

#ifdef ARDUINO_WAVESHARE_ESP32_S3_ZERO
#define RX_PIN 1
#define TX_PIN 2
#define LED_TYPE 1  // RGB LED
#define LED_PIN 21
#define LED_COLOR_ORDER GRB
#endif

// ---------------------------------------------------------------------------

#include "CRC16.h"
#include "CRC.h"

#if LED_TYPE == 1
#include <FastLED.h>
CRGB leds[1];
#endif

#define FRAMEBUFLEN 64
#define USERBUFLEN 32

#define serial_usb Serial
#define serial_dbus Serial0

// ---------------------------------------------------------------------------

CRC16 crc(CRC16_XMODEM_POLYNOME,
          CRC16_XMODEM_INITIAL,
          CRC16_XMODEM_XOR_OUT,
          CRC16_XMODEM_REV_IN,
          CRC16_XMODEM_REV_OUT);

byte framebuf[FRAMEBUFLEN];
unsigned int framepos = 0;
unsigned int framelen = 0;
unsigned long readlast = 0;
char userbuf[USERBUFLEN];
unsigned int userlen = 0;

// ---------------------------------------------------------------------------

void led_init() {
#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
#if LED_TYPE == 1
  FastLED.addLeds<WS2812, LED_PIN, LED_COLOR_ORDER>(leds, 1);
#endif
  led_set(0, 0);
#endif
}

void led_set(uint8_t color, uint8_t ms) {
#ifdef LED_PIN

#if LED_TYPE == 0
  if (color == 0) {
    digitalWrite(LED_PIN, LED_OFF);
  } else {  // Blue
    digitalWrite(LED_PIN, LED_ON);
  }
#endif

#if LED_TYPE == 1
  if (color == 0 || color > 4) {
    leds[0] = CRGB::Black;
  }
  if (color == 1) {
    leds[0] = CRGB::Green;
  }
  if (color == 2) {
    leds[0] = CRGB::Red;
  }
  if (color == 3) {
    leds[0] = CRGB::Blue;
  }
  if (color == 4) {
    leds[0] = CRGB::Purple;
  }
  FastLED.show();
#endif

  delay(ms);

  if (color > 0) {
    led_set(0,0);
  }

#endif
}

// ---------------------------------------------------------------------------

void setup() {
  delay(500);

  led_init();

  serial_usb.begin(115200);
  serial_usb.println("\n\nB/S/H/ D-Bus logger started.");

  serial_dbus.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  while (serial_dbus.available()) {
    unsigned long readnow = millis();
    byte rawbyte = serial_dbus.read();

    if ((readnow - readlast) > 40) {
      if ((framelen > 0) && (framepos == framelen)) {
        serial_usb.printf("(ack=none)\n");
      } else if (framepos > 0) {
        serial_usb.printf("(read=timout)\n");
      }
      framepos = framelen = 0;
      crc.restart();
    } else if ((framelen > 0) && (framepos == framelen)) {
      framepos = framelen = 0;
      crc.restart();
      // Strictly speaking, this should not be necessary here
      // if the timing is precise, so it is a kind of workaround:
      if (rawbyte == ((framebuf[1] & 0xf0) | 0x0a)) {
        serial_usb.printf("%02x (ack=ok)\n", rawbyte);
        continue;
      } else {
        serial_usb.printf("(ack=err, re-evaluate byte)\n");
      }
    }

    readlast = readnow;
    crc.add(rawbyte);
    serial_usb.printf("%02x", rawbyte);
    led_set(3, 15);
    if (framepos < FRAMEBUFLEN) {
      framebuf[framepos++] = rawbyte;
    }

    if (framepos == 1) {
      framelen = 2 + 2 + (unsigned int)rawbyte;
      serial_usb.printf(" | ");
    } else if (framepos == 2) {
      serial_usb.printf(".");
    } else if (framepos == 3) {
      serial_usb.printf("-");
    } else if (framepos == 4) {
      serial_usb.printf(" | ");
    } else if (framepos == (framelen - 2)) {
      for (unsigned int p = framelen; p < 10; p++) {
        serial_usb.printf("   ");
      }
      serial_usb.printf(" | ");
    } else if (framepos == framelen) {
      if (crc.calc()) {
        serial_usb.printf(" (crc=err, len=%d)\n", framelen);
        led_set(2, 80);
        framepos = framelen = 0;
        crc.restart();
      } else {
        serial_usb.printf(" (crc=ok) | ");
        led_set(1, 80);
        // We only process a small sample set of data here:
        switch (framebuf[1] << 16 | framebuf[2] << 8 | framebuf[3]) {
          case 0x141004:
            serial_usb.printf("(Temperature=%d) ", framebuf[4]);
            break;
          case 0x141005:
            serial_usb.printf("(Washing program=%d) ", framebuf[6]);
            break;
          case 0x141006:
            serial_usb.printf("(Spin speed=%d) ", framebuf[4]);
            break;
          case 0x2a1600:
            serial_usb.printf("(Remaining time=%d) ", framebuf[4]);
            break;
        }
      }
    } else {
      serial_usb.printf(" ");
    }
  }

  // Dirty way to write to the D-Bus: No silence / collision / anything detection, just fire and forget
  // Input via terminal: DS.CC-CC MM MM (no length prefix, no crc)

  while (serial_usb.available()) {
    byte c = serial_usb.read();
    if (((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) && userlen < USERBUFLEN) {
      userbuf[userlen++] = c;
    }
    if (c != '\r' && c != '\n') continue;
    if (userlen == 0) break;
    if (userlen < 6 || userlen % 2) {
      serial_usb.println("Error: Invalid input length");
      userlen = 0;
      continue;
    }
    CRC16 outcrc(CRC16_XMODEM_POLYNOME, CRC16_XMODEM_INITIAL, CRC16_XMODEM_XOR_OUT, CRC16_XMODEM_REV_IN, CRC16_XMODEM_REV_OUT);
    byte outbyte = userlen / 2 - 1;    
    outcrc.add(outbyte);
    led_set(4, 80);
    serial_dbus.write(outbyte);
    for (unsigned int i = 0; i < userlen; i += 2) {
      outbyte = (userbuf[i] % 32 + 9) % 25 * 16 + (userbuf[i + 1] % 32 + 9) % 25;
      outcrc.add(outbyte);
      serial_dbus.write(outbyte);
    }
    uint16_t outcc = outcrc.calc();
    serial_dbus.write(outcc >> 8);
    serial_dbus.write(outcc);
    userlen = 0;
  }
}
