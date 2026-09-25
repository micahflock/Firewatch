#include <unity.h>

#include "scene.h"

using namespace fw;

static TvStatus tv(TvPower p, bool fire = false) { return {p, fire}; }

void setUp() {}
void tearDown() {}

void test_remote_power_on_starts_scene_and_launches() {
  Scene s;
  s.observe(tv(TvPower::Standby), 0);
  TEST_ASSERT_FALSE(s.lightsOn());
  Actions a = s.observe(tv(TvPower::On), 2000);
  TEST_ASSERT_TRUE(s.lightsOn());
  TEST_ASSERT_TRUE(a.launch);
  // YouTube is resident but may be parked on its home screen: a real power-on relaunches.
  Scene s2;
  s2.observe(tv(TvPower::Standby), 0);
  TEST_ASSERT_TRUE(s2.observe(tv(TvPower::On, true), 2000).launch);
}

void test_launch_stops_once_fire_is_playing() {
  Scene s;
  s.observe(tv(TvPower::Standby), 0);
  s.observe(tv(TvPower::On), 1000);
  TEST_ASSERT_FALSE(s.observe(tv(TvPower::On), 3000).launch);  // within retry window
  TEST_ASSERT_FALSE(s.observe(tv(TvPower::On, true), 5000).launch);
  TEST_ASSERT_FALSE(s.observe(tv(TvPower::On, false), 60000).launch);  // user switched apps: leave it
}

void test_launch_retries_then_gives_up() {
  SceneTiming t;
  Scene s(t);
  s.observe(tv(TvPower::Standby), 0);
  int launches = 0;
  for (uint32_t now = 1000; now < 200000; now += 1000) launches += s.observe(tv(TvPower::On), now).launch;
  TEST_ASSERT_EQUAL(t.maxLaunchAttempts, launches);
  TEST_ASSERT_TRUE(s.lightsOn());
}

void test_remote_power_off_ends_scene() {
  Scene s;
  s.observe(tv(TvPower::Standby), 0);
  s.observe(tv(TvPower::On), 1000);
  Actions a = s.observe(tv(TvPower::Standby), 5000);
  TEST_ASSERT_FALSE(s.lightsOn());
  TEST_ASSERT_FALSE(a.powerOff);
}

void test_single_dropped_poll_does_not_end_scene() {
  Scene s;
  s.observe(tv(TvPower::Standby), 0);
  s.observe(tv(TvPower::On, true), 1000);
  s.observe(tv(TvPower::Unreachable), 3000);
  s.observe(tv(TvPower::Unreachable), 5000);
  TEST_ASSERT_TRUE(s.lightsOn());
  s.observe(tv(TvPower::On, true), 7000);
  TEST_ASSERT_TRUE(s.lightsOn());
  for (uint32_t now = 9000; now < 20000; now += 2000) s.observe(tv(TvPower::Unreachable), now);
  TEST_ASSERT_FALSE(s.lightsOn());
}

void test_command_on_wakes_tv_retries_and_launches() {
  Scene s;
  s.observe(tv(TvPower::Standby), 0);
  Actions a = s.command(Command::On, 1000);
  TEST_ASSERT_TRUE(a.wake);
  TEST_ASSERT_TRUE(s.lightsOn());  // lights start immediately, before the TV is up
  TEST_ASSERT_EQUAL(Scene::State::Waking, s.state());
  TEST_ASSERT_FALSE(s.observe(tv(TvPower::Standby), 3000).wake);
  TEST_ASSERT_TRUE(s.observe(tv(TvPower::Standby), 7500).wake);
  a = s.observe(tv(TvPower::On), 9000);
  TEST_ASSERT_EQUAL(Scene::State::On, s.state());
  TEST_ASSERT_TRUE(a.launch);
}

void test_wake_timeout_keeps_lights_then_follows_tv() {
  Scene s;
  s.observe(tv(TvPower::Unreachable), 0);
  s.command(Command::On, 0);
  for (uint32_t now = 2000; now <= 62000; now += 2000) s.observe(tv(TvPower::Unreachable), now);
  TEST_ASSERT_EQUAL(Scene::State::On, s.state());
  TEST_ASSERT_TRUE(s.lightsOn());
  // TV later comes up (e.g. via the remote): launch, and its power-off then ends the scene.
  TEST_ASSERT_TRUE(s.observe(tv(TvPower::On), 70000).launch);
  s.observe(tv(TvPower::Standby), 80000);
  TEST_ASSERT_FALSE(s.lightsOn());
}

void test_command_off_powers_tv_off_without_retriggering() {
  Scene s;
  s.observe(tv(TvPower::Standby), 0);
  s.observe(tv(TvPower::On, true), 1000);
  Actions a = s.command(Command::Off, 2000);
  TEST_ASSERT_TRUE(a.powerOff);
  TEST_ASSERT_FALSE(s.lightsOn());
  s.observe(tv(TvPower::On, true), 3000);  // TV still shutting down
  TEST_ASSERT_FALSE(s.lightsOn());
  s.observe(tv(TvPower::Standby), 5000);
  TEST_ASSERT_FALSE(s.lightsOn());
}

void test_toggle_and_on_when_tv_already_on() {
  Scene s;
  s.observe(tv(TvPower::On, true), 0);  // ESP32 booted under a running fire
  TEST_ASSERT_TRUE(s.lightsOn());
  TEST_ASSERT_TRUE(s.command(Command::Toggle, 1000).powerOff);
  TEST_ASSERT_FALSE(s.lightsOn());
}

void test_boot_under_running_fire_does_not_relaunch() {
  Scene s;
  Actions a = s.observe(tv(TvPower::On, true), 0);
  TEST_ASSERT_TRUE(s.lightsOn());
  TEST_ASSERT_FALSE(a.launch);
}

void test_follow_tv_disabled() {
  SceneTiming t;
  t.followTv = false;
  Scene s(t);
  s.observe(tv(TvPower::Standby), 0);
  s.observe(tv(TvPower::On), 1000);
  TEST_ASSERT_FALSE(s.lightsOn());
}

void test_relaunch_forces_launch() {
  Scene s;
  s.observe(tv(TvPower::Standby), 0);
  s.observe(tv(TvPower::On, true), 1000);
  s.observe(tv(TvPower::On, true), 3000);
  s.command(Command::Relaunch, 4000);
  TEST_ASSERT_TRUE(s.observe(tv(TvPower::On, true), 5000).launch);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_remote_power_on_starts_scene_and_launches);
  RUN_TEST(test_launch_stops_once_fire_is_playing);
  RUN_TEST(test_launch_retries_then_gives_up);
  RUN_TEST(test_remote_power_off_ends_scene);
  RUN_TEST(test_single_dropped_poll_does_not_end_scene);
  RUN_TEST(test_command_on_wakes_tv_retries_and_launches);
  RUN_TEST(test_wake_timeout_keeps_lights_then_follows_tv);
  RUN_TEST(test_command_off_powers_tv_off_without_retriggering);
  RUN_TEST(test_toggle_and_on_when_tv_already_on);
  RUN_TEST(test_boot_under_running_fire_does_not_relaunch);
  RUN_TEST(test_follow_tv_disabled);
  RUN_TEST(test_relaunch_forces_launch);
  return UNITY_END();
}
