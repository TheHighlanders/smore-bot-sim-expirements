/*
 * Host unit tests for the discrete-output path.
 *
 * Runs with:  pio test -e native
 * Uses MockTransport, which drives the SAME shared/base_model.h that the Wokwi
 * base-controller chip runs -- so a green bar here means the simulated hardware
 * behaves the same.
 */
#include <unity.h>
#include <string.h>
#include "P1AM_Sim.h"
#include "MockTransport.h"

static const char *LINEUP[] = { "P1-08TRS" };

void setUp(void) {}
void tearDown(void) {}

static void test_enumeration_reports_module(void) {
    MockTransport bus(LINEUP, 1);
    P1AM_Sim P1(bus);
    TEST_ASSERT_EQUAL_UINT8(1, P1.init());
    TEST_ASSERT_EQUAL_UINT8(1, P1.modules());
    TEST_ASSERT_EQUAL_STRING("P1-08TRS", P1.slotName(1));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", P1.slotName(2));
}

static void test_single_channel_write_read(void) {
    MockTransport bus(LINEUP, 1);
    P1AM_Sim P1(bus);
    P1.init();
    P1.writeDiscrete(HIGH, 1, 3);              // channel 3 -> bit 2
    TEST_ASSERT_EQUAL_UINT32(1, P1.readDiscrete(1, 3));
    TEST_ASSERT_EQUAL_UINT32(0, P1.readDiscrete(1, 2));
    TEST_ASSERT_EQUAL_UINT32(0x04, P1.readDiscrete(1, 0));   // bitmap read-back
    TEST_ASSERT_EQUAL_UINT8(0x04, bus.rawOutByte(1, 0));     // physical image
}

static void test_bitmapped_write(void) {
    MockTransport bus(LINEUP, 1);
    P1AM_Sim P1(bus);
    P1.init();
    P1.writeDiscrete(0xA5, 1);                 // 1010 0101, LSB = ch1
    TEST_ASSERT_EQUAL_UINT32(0xA5, P1.readDiscrete(1, 0));
    TEST_ASSERT_EQUAL_UINT32(1, P1.readDiscrete(1, 1));
    TEST_ASSERT_EQUAL_UINT32(0, P1.readDiscrete(1, 2));
    TEST_ASSERT_EQUAL_UINT32(1, P1.readDiscrete(1, 8));
}

static void test_clear_single_channel(void) {
    MockTransport bus(LINEUP, 1);
    P1AM_Sim P1(bus);
    P1.init();
    P1.writeDiscrete(0xFF, 1);
    P1.writeDiscrete(LOW, 1, 1);               // clear channel 1
    TEST_ASSERT_EQUAL_UINT32(0, P1.readDiscrete(1, 1));
    TEST_ASSERT_EQUAL_UINT32(0xFE, P1.readDiscrete(1, 0));
}

static void test_firmware_version(void) {
    MockTransport bus(LINEUP, 1);
    P1AM_Sim P1(bus);
    P1.init();
    TEST_ASSERT_EQUAL_UINT32(0x00010000UL, P1.getFwVersion());
}

static void test_bad_slot_is_safe(void) {
    MockTransport bus(LINEUP, 1);
    P1AM_Sim P1(bus);
    P1.init();
    P1.writeDiscrete(0x0F, 1);
    P1.writeDiscrete(0xFF, 2);                 // slot 2 does not exist -> ignored
    TEST_ASSERT_EQUAL_UINT32(0, P1.readDiscrete(2, 0));
    TEST_ASSERT_EQUAL_UINT32(0x0F, P1.readDiscrete(1, 0));   // slot 1 untouched
}

static void test_two_module_lineup(void) {
    const char *lineup2[] = { "P1-08TRS", "P1-16TR" };
    MockTransport bus(lineup2, 2);
    P1AM_Sim P1(bus);
    TEST_ASSERT_EQUAL_UINT8(2, P1.init());
    TEST_ASSERT_EQUAL_STRING("P1-16TR", P1.slotName(2));
    P1.writeDiscrete(0x0102, 2);               // 16-bit bitmap into the 2-byte module
    TEST_ASSERT_EQUAL_UINT32(0x0102, P1.readDiscrete(2, 0));
    TEST_ASSERT_EQUAL_UINT32(1, P1.readDiscrete(2, 9));       // bit 8 -> channel 9
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_enumeration_reports_module);
    RUN_TEST(test_single_channel_write_read);
    RUN_TEST(test_bitmapped_write);
    RUN_TEST(test_clear_single_channel);
    RUN_TEST(test_firmware_version);
    RUN_TEST(test_bad_slot_is_safe);
    RUN_TEST(test_two_module_lineup);
    return UNITY_END();
}
