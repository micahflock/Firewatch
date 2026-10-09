#include <unity.h>

#include "home_sync.h"

using namespace fw;

static const HomeState kOff{false, 60};
static const HomeState kOn{true, 60};

void setUp() {}
void tearDown() {}

void test_matching_state_reports_nothing() {
  HomeSync h;
  h.seed(kOff);
  HomeState out;
  TEST_ASSERT_FALSE(h.poll(kOff, 1000, out));
}

void test_remote_change_is_reported_once() {
  HomeSync h;
  h.seed(kOff);
  HomeState out;
  TEST_ASSERT_TRUE(h.poll(kOn, 1000, out));  // Roku remote turned the TV (and scene) on
  TEST_ASSERT_TRUE(out.on);
  TEST_ASSERT_EQUAL(60, out.brightness);
  TEST_ASSERT_FALSE(h.poll(kOn, 1250, out));  // Home already knows
}

void test_home_write_holds_until_device_catches_up() {
  HomeSync h;
  h.seed(kOff);
  HomeState out;
  TEST_ASSERT_TRUE(h.homeWrote(kOn, kOff, 1000));  // scene must turn on
  TEST_ASSERT_FALSE(h.poll(kOff, 1250, out));      // net task still busy: don't flip the tile back
  TEST_ASSERT_FALSE(h.poll(kOff, 5999, out));
  // Device catching up early ends the hold, so a later remote change is reported at once.
  HomeSync h2;
  h2.seed(kOff);
  h2.homeWrote(kOn, kOff, 1000);
  TEST_ASSERT_FALSE(h2.poll(kOn, 1300, out));  // caught up: nothing to report, hold over
  TEST_ASSERT_TRUE(h2.poll(kOff, 2000, out));  // remote turned it off 0.7 s later
  TEST_ASSERT_FALSE(out.on);
}

void test_hold_expires_and_reports_real_state() {
  HomeSync h;
  h.seed(kOff);
  HomeState out;
  h.homeWrote(kOn, kOff, 1000);
  TEST_ASSERT_FALSE(h.poll(kOff, 5999, out));
  TEST_ASSERT_TRUE(h.poll(kOff, 6000, out));  // command was lost (e.g. queue full): Home must not lie
  TEST_ASSERT_FALSE(out.on);
}

void test_home_write_matching_state_is_not_a_command() {
  HomeSync h;
  h.seed(kOn);
  // "Turn on all the lights" while the scene is already on: no command (it would relaunch the
  // fire over whatever the TV is showing).
  TEST_ASSERT_FALSE(h.homeWrote(kOn, kOn, 1000));
  TEST_ASSERT_TRUE(h.homeWrote(kOff, kOn, 2000));
  HomeSync h2;
  h2.seed(kOff);
  TEST_ASSERT_FALSE(h2.homeWrote(kOff, kOff, 1000));
}

void test_quick_reversing_tap_is_a_command() {
  // Tap On, then Off while the net task is still busy with the On (lightsOn not yet true):
  // the Off must still be sent, or the queued On wakes the TV that Home now shows as off.
  HomeSync h;
  h.seed(kOff);
  TEST_ASSERT_TRUE(h.homeWrote(kOn, kOff, 1000));
  TEST_ASSERT_TRUE(h.homeWrote(kOff, kOff, 1300));
  // And the other way round: Off, then On while the PowerOff request is still running.
  HomeSync h2;
  h2.seed(kOn);
  TEST_ASSERT_TRUE(h2.homeWrote(kOff, kOn, 1000));
  TEST_ASSERT_TRUE(h2.homeWrote(kOn, kOn, 1300));
  // Once the hold is over, the device's real state is the baseline again.
  TEST_ASSERT_FALSE(h2.homeWrote(kOn, kOn, 7000));
}

void test_brightness_only_change_is_reported() {
  HomeSync h;
  h.seed(kOn);
  HomeState out;
  TEST_ASSERT_TRUE(h.poll({true, 35}, 1000, out));  // web slider moved
  TEST_ASSERT_TRUE(out.on);
  TEST_ASSERT_EQUAL(35, out.brightness);
}

void test_brightness_write_from_home_is_not_echoed() {
  HomeSync h;
  h.seed(kOn);
  HomeState out;
  TEST_ASSERT_FALSE(h.homeWrote({true, 30}, kOn, 1000));  // brightness only: no scene command
  TEST_ASSERT_FALSE(h.poll({true, 30}, 1250, out));       // setter applied it synchronously
}

void test_hold_survives_millis_wraparound() {
  HomeSync h;
  h.seed(kOff);
  HomeState out;
  const uint32_t start = 0xFFFFF000u;  // ~4 s before millis() wraps
  h.homeWrote(kOn, kOff, start);
  TEST_ASSERT_FALSE(h.poll(kOff, start + 4999u, out));  // wrapped past 0, still inside the hold
  TEST_ASSERT_TRUE(h.poll(kOff, start + 5000u, out));
}

void test_setup_code_validation() {
  static_assert(validSetupCode("20481024"), "must be usable at compile time");
  TEST_ASSERT_TRUE(validSetupCode("20481024"));
  TEST_ASSERT_TRUE(validSetupCode("46637726"));
  TEST_ASSERT_FALSE(validSetupCode("1234567"));    // 7 digits
  TEST_ASSERT_FALSE(validSetupCode("123456789"));  // 9 digits
  TEST_ASSERT_FALSE(validSetupCode("466-37-726"));  // HomeSpan reads digits only
  TEST_ASSERT_FALSE(validSetupCode("11111111"));
  TEST_ASSERT_FALSE(validSetupCode("00000000"));
  TEST_ASSERT_FALSE(validSetupCode("12345678"));
  TEST_ASSERT_FALSE(validSetupCode("87654321"));
  TEST_ASSERT_FALSE(validSetupCode(""));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_matching_state_reports_nothing);
  RUN_TEST(test_remote_change_is_reported_once);
  RUN_TEST(test_home_write_holds_until_device_catches_up);
  RUN_TEST(test_hold_expires_and_reports_real_state);
  RUN_TEST(test_home_write_matching_state_is_not_a_command);
  RUN_TEST(test_quick_reversing_tap_is_a_command);
  RUN_TEST(test_brightness_only_change_is_reported);
  RUN_TEST(test_brightness_write_from_home_is_not_echoed);
  RUN_TEST(test_hold_survives_millis_wraparound);
  RUN_TEST(test_setup_code_validation);
  return UNITY_END();
}
