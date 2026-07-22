/*
 * main.cpp  --  ProductivityOpen (P1AM) teaching demo, Wokwi edition.
 *
 * This sketch is written EXACTLY as it would be on real P1AM hardware: it uses
 * the P1AM API (P1.init / P1.writeDiscrete) for the P1000 discrete-output
 * module, and plain Arduino GPIO for the GPIO shield. The only difference is
 * the #include below (P1AM_Sim.h instead of P1AM.h) and that it runs on an
 * RP2040 stand-in inside Wokwi.
 *
 * Two subsystems, connected two different ways -- this is the whole point:
 *
 *   1. DISCRETE OUTPUT MODULE (slot 1, a P1-08TRS relay module)
 *      Reached over SPI through the base controller. Pressing the shield
 *      button advances a "chase" that lights one relay at a time. You watch
 *      the green relay indicators (and a real relay part) energize -- never a
 *      raw SPI byte.
 *
 *   2. GPIO SHIELD (direct Arduino pins, no base controller)
 *      A button, a digital-output LED that mirrors the button, and a PWM LED
 *      whose brightness follows the potentiometer.
 */
#include <Arduino.h>
#include "P1AM_Sim.h"
#include "ShieldLogic.h"

// ---- GPIO shield pin map (Pico stand-in; see wokwi/diagram.json) ----
static const int PIN_BTN    = 15;   // pushbutton to GND  (INPUT_PULLUP)
static const int PIN_DOLED  = 14;   // digital-output demo LED
static const int PIN_PWMLED = 13;   // PWM demo LED (analogWrite)
static const int PIN_POT    = 26;   // potentiometer wiper -> ADC0

// ---- P1000 module layout ----
// Slot 1 holds a P1-08TRS: an 8-channel relay output module (6x Form A SPST +
// 2x Form C SPDT). [ref: docs/references/facts-docs/P1-08TRS.md;
// ID/byte-count: docs/references/p1am-library.md#module-ids]
static const uint8_t RELAY_SLOT = 1;   // P1-08TRS in slot 1
static const uint8_t RELAY_CH   = 8;   // 8 relay channels

static uint8_t chaseStep = 0;
static bool    lastBtn   = false;

void setup() {
    Serial.begin(115200);

    // GPIO shield: plain Arduino I/O -- nothing to do with the base controller.
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_DOLED, OUTPUT);
    pinMode(PIN_PWMLED, OUTPUT);

    // Discrete module: bring up the base controller over SPI and enumerate.
    uint8_t n = 0;
    while ((n = P1.init()) == 0) {
        Serial.println("Waiting for base controller...");
        delay(200);
    }
    Serial.print("Base online. Modules found: ");
    Serial.println(n);
    Serial.print("Slot 1: ");
    Serial.println(P1.slotName(RELAY_SLOT));

    P1.writeDiscrete(0x00, RELAY_SLOT);   // all relays off to start
    Serial.println("Ready");
}

void loop() {
    // --- GPIO shield: potentiometer -> PWM LED brightness ---
    int adc = analogRead(PIN_POT);
    analogWrite(PIN_PWMLED, shield::potToPwm((uint16_t)adc, 1023));

    // --- GPIO shield: digital-out LED mirrors the button (active-low) ---
    bool pressed = (digitalRead(PIN_BTN) == LOW);
    digitalWrite(PIN_DOLED, pressed ? HIGH : LOW);

    // --- Discrete module: on each button press, step the relay chase ---
    if (pressed && !lastBtn) {
        uint32_t mask = shield::chaseMask(chaseStep, RELAY_CH);
        P1.writeDiscrete(mask, RELAY_SLOT);           // channel 0 => bitmap
        Serial.print("Relay step ");
        Serial.print(chaseStep);
        Serial.print(" mask=0x");
        Serial.println(mask, HEX);
        chaseStep = (uint8_t)((chaseStep + 1) % RELAY_CH);
    }
    lastBtn = pressed;

    delay(20);   // crude debounce / loop pacing
}
