/*
 * Host unit tests for the analog-input and temperature path.
 * Runs with:  pio test -e native
 * Same MockTransport -> shared base_model.h the Wokwi chip uses.
 */
#include <unity.h>
#include <string.h>
#include "P1AM_Sim.h"
#include "MockTransport.h"

void setUp(void) {}
void tearDown(void) {}

static void test_analog_enumeration(void) {
    const char *lineup[] = { "P1-04AD-2" };
    MockTransport bus(lineup, 1);
    P1AM_Sim P1(bus);
    TEST_ASSERT_EQUAL_UINT8(1, P1.init());
    TEST_ASSERT_EQUAL_STRING("P1-04AD-2", P1.slotName(1));
}

static void test_analog_read_counts(void) {
    const char *lineup[] = { "P1-04AD-2" };
    MockTransport bus(lineup, 1);
    P1AM_Sim P1(bus);
    P1.init();
    bus.setAnalog(1, 2, 13500);              // inject ch2 = 13500 counts
    TEST_ASSERT_EQUAL_INT(13500, P1.readAnalog(1, 2));
    TEST_ASSERT_EQUAL_INT(0, P1.readAnalog(1, 1)); // untouched channel
}

static void test_analog_channels_independent(void) {
    const char *lineup[] = { "P1-04ADL-2" };
    MockTransport bus(lineup, 1);
    P1AM_Sim P1(bus);
    P1.init();
    bus.setAnalog(1, 1, 100);
    bus.setAnalog(1, 4, -25000);             // negative int32 round-trips
    TEST_ASSERT_EQUAL_INT(100, P1.readAnalog(1, 1));
    TEST_ASSERT_EQUAL_INT(-25000, P1.readAnalog(1, 4));
}

static void test_temperature_float_roundtrip(void) {
    const char *lineup[] = { "P1-04THM" };
    MockTransport bus(lineup, 1);
    P1AM_Sim P1(bus);
    P1.init();
    bus.setTemperature(1, 3, 25.5f);
    TEST_ASSERT_EQUAL_FLOAT(25.5f, P1.readTemperature(1, 3));
    bus.setTemperature(1, 1, -40.0f);
    TEST_ASSERT_EQUAL_FLOAT(-40.0f, P1.readTemperature(1, 1));
}

static void test_mixed_lineup_addressing(void) {
    /* A realistic base: relay out + digital out + analog in + thermocouple. */
    const char *lineup[] = { "P1-08TRS", "P1-15TD1", "P1-04AD-2", "P1-04THM" };
    MockTransport bus(lineup, 4);
    P1AM_Sim P1(bus);
    TEST_ASSERT_EQUAL_UINT8(4, P1.init());
    TEST_ASSERT_EQUAL_STRING("P1-15TD1", P1.slotName(2));

    // 15-ch digital output uses 2 bytes; channel 9 lives in byte 1 (bit 0).
    P1.writeDiscrete(1UL << 8, 2);           // bitmap: only channel 9 on
    TEST_ASSERT_EQUAL_UINT32(1, P1.readDiscrete(2, 9));
    TEST_ASSERT_EQUAL_UINT32(0, P1.readDiscrete(2, 1));

    bus.setAnalog(3, 1, 2048);
    bus.setTemperature(4, 2, 100.0f);
    TEST_ASSERT_EQUAL_INT(2048, P1.readAnalog(3, 1));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, P1.readTemperature(4, 2));

    // Reading analog from the relay slot (no AI bytes) is safe -> 0.
    TEST_ASSERT_EQUAL_INT(0, P1.readAnalog(1, 1));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_analog_enumeration);
    RUN_TEST(test_analog_read_counts);
    RUN_TEST(test_analog_channels_independent);
    RUN_TEST(test_temperature_float_roundtrip);
    RUN_TEST(test_mixed_lineup_addressing);
    return UNITY_END();
}
