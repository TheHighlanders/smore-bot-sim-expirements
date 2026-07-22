/*
 * Host unit tests for the GPIO-shield helper logic (ShieldLogic.h).
 * Runs with:  pio test -e native
 */
#include <unity.h>
#include "ShieldLogic.h"

void setUp(void) {}
void tearDown(void) {}

static void test_pot_to_pwm_endpoints(void) {
    TEST_ASSERT_EQUAL_UINT8(0,   shield::potToPwm(0, 1023));
    TEST_ASSERT_EQUAL_UINT8(255, shield::potToPwm(1023, 1023));
}

static void test_pot_to_pwm_midpoint(void) {
    // 512/1023 * 255 = 127.6 -> 127 (integer truncation)
    TEST_ASSERT_EQUAL_UINT8(127, shield::potToPwm(512, 1023));
}

static void test_pot_to_pwm_clamps_and_guards(void) {
    TEST_ASSERT_EQUAL_UINT8(255, shield::potToPwm(5000, 1023)); // over-range clamps
    TEST_ASSERT_EQUAL_UINT8(0,   shield::potToPwm(100, 0));     // adcMax==0 guard
}

static void test_chase_mask_walks_one_hot(void) {
    TEST_ASSERT_EQUAL_UINT32(0x01, shield::chaseMask(0, 8));
    TEST_ASSERT_EQUAL_UINT32(0x02, shield::chaseMask(1, 8));
    TEST_ASSERT_EQUAL_UINT32(0x80, shield::chaseMask(7, 8));
}

static void test_chase_mask_wraps(void) {
    TEST_ASSERT_EQUAL_UINT32(0x01, shield::chaseMask(8, 8));    // wraps back to ch1
    TEST_ASSERT_EQUAL_UINT32(0x00, shield::chaseMask(3, 0));    // zero channels guard
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_pot_to_pwm_endpoints);
    RUN_TEST(test_pot_to_pwm_midpoint);
    RUN_TEST(test_pot_to_pwm_clamps_and_guards);
    RUN_TEST(test_chase_mask_walks_one_hot);
    RUN_TEST(test_chase_mask_wraps);
    return UNITY_END();
}
