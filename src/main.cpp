/*
 * main.cpp  --  ProductivityOpen (P1AM) teaching demo, Wokwi edition.
 *
 * Written exactly as on real P1AM hardware: the P1AM API for the P1000 modules
 * (behind the SPI base controller) and plain Arduino GPIO for the GPIO shield.
 * Only the #include changes (P1AM_Sim.h) and it runs on an RP2040 stand-in.
 *
 * Base lineup (see wokwi/chips/p1-base-controller): slot 1 P1-15TD1 (digital
 * out), slot 2 P1-08TRS (relay out), slot 3 P1-04AD-2 (analog in), slot 4
 * P1-04THM (thermocouple). Module IDs/kinds:
 * [ref: docs/references/p1am-library.md#module-ids;
 *  docs/references/facts-docs/<part>.md]
 *
 * Two connection paths, on purpose:
 *   - P1000 modules -> SPI base controller (P1.writeDiscrete/readAnalog/...)
 *   - GPIO shield    -> direct Arduino pins (digitalWrite/analogRead/...)
 */
#include <Arduino.h>
#include "P1AM_Sim.h"
#include "ShieldLogic.h"

// ---- GPIO shield pin map (Pico stand-in; see diagram.json) ----
static const int PIN_BTN    = 15;   // pushbutton to GND  (INPUT_PULLUP)
static const int PIN_DOLED  = 14;   // digital-output demo LED
static const int PIN_PWMLED = 13;   // PWM demo LED (analogWrite)
static const int PIN_POT    = 26;   // potentiometer wiper -> ADC0

// ---- P1000 slot layout ----
static const uint8_t DOUT_SLOT  = 1; // P1-15TD1, 15-ch sinking DC output
static const uint8_t DOUT_CH    = 15;
static const uint8_t RELAY_SLOT = 2; // P1-08TRS, 8-ch relay output
static const uint8_t AIN_SLOT   = 3; // P1-04AD-2, 4-ch analog input
static const uint8_t TEMP_SLOT  = 4; // P1-04THM, 4-ch thermocouple

static uint8_t  chaseStep = 0;
static bool     lastBtn   = false;
static uint16_t tick      = 0;

void setup() {
    Serial.begin(115200);

    // GPIO shield: plain Arduino I/O -- no base controller involved.
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_DOLED, OUTPUT);
    pinMode(PIN_PWMLED, OUTPUT);

    // P1000 modules: bring up the base controller over SPI and enumerate.
    uint8_t n = 0;
    while ((n = P1.init()) == 0) {
        Serial.println("Waiting for base controller...");
        delay(200);
    }
    Serial.print("Base online. Modules found: ");
    Serial.println(n);
    for (uint8_t s = 1; s <= n; s++) {
        Serial.print("  slot ");
        Serial.print(s);
        Serial.print(": ");
        Serial.println(P1.slotName(s));
    }
    P1.writeDiscrete(0x0000, DOUT_SLOT);   // all digital outputs off
    P1.writeDiscrete(0x00, RELAY_SLOT);    // relays off
    Serial.println("Ready");
}

void loop() {
    // --- GPIO shield: potentiometer -> PWM LED brightness ---
    int adc = analogRead(PIN_POT);
    analogWrite(PIN_PWMLED, shield::potToPwm((uint16_t)adc, 1023));

    // --- GPIO shield: digital-out LED mirrors the button (active-low) ---
    bool pressed = (digitalRead(PIN_BTN) == LOW);
    digitalWrite(PIN_DOLED, pressed ? HIGH : LOW);

    // --- Relay (slot 2, ch 1) follows the button so you see it energize ---
    P1.writeDiscrete(pressed ? HIGH : LOW, RELAY_SLOT, 1);

    // --- Digital output (slot 1): each button press steps the 15-ch chase ---
    if (pressed && !lastBtn) {
        uint32_t mask = shield::chaseMask(chaseStep, DOUT_CH);
        P1.writeDiscrete(mask, DOUT_SLOT);            // channel 0 => bitmap
        Serial.print("DO step ");
        Serial.print(chaseStep);
        Serial.print(" mask=0x");
        Serial.println(mask, HEX);
        chaseStep = (uint8_t)((chaseStep + 1) % DOUT_CH);
    }
    lastBtn = pressed;

    // --- Analog in (slot 3) + thermocouple (slot 4): report ~once/second ---
    if (++tick >= 50) {
        tick = 0;
        int   ai1 = P1.readAnalog(AIN_SLOT, 1);
        float t1  = P1.readTemperature(TEMP_SLOT, 1);
        Serial.print("AI1=");
        Serial.print(ai1);
        Serial.print(" counts, T1=");
        Serial.print(t1, 1);
        Serial.println(" C");
    }

    delay(20);   // crude debounce / loop pacing
}
