#include <unity.h>

#include "roku_parse.h"

using namespace fw;

void setUp() {}
void tearDown() {}

static const char* kDeviceInfoOn =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<device-info>\n"
    "\t<udn>29a00000-0000-1000-8000-000000000000</udn>\n"
    "\t<model-name>32S335</model-name>\n"
    "\t<wifi-mac>a8:b1:c2:00:11:ff</wifi-mac>\n"
    "\t<ethernet-mac>a8:b1:c2:00:11:fe</ethernet-mac>\n"
    "\t<is-tv>true</is-tv>\n"
    "\t<power-mode>PowerOn</power-mode>\n"
    "\t<supports-wake-on-wlan>true</supports-wake-on-wlan>\n"
    "</device-info>\n";

void test_power_mode() {
  TEST_ASSERT_EQUAL(TvPower::On, roku::powerFromDeviceInfo(kDeviceInfoOn));
  TEST_ASSERT_EQUAL(TvPower::Standby,
                    roku::powerFromDeviceInfo("<device-info><power-mode>DisplayOff</power-mode></device-info>"));
  TEST_ASSERT_EQUAL(TvPower::Standby,
                    roku::powerFromDeviceInfo("<device-info><power-mode>Ready</power-mode></device-info>"));
  TEST_ASSERT_EQUAL(TvPower::On, roku::powerFromDeviceInfo("<device-info><model-name>x</model-name></device-info>"));
  TEST_ASSERT_EQUAL(TvPower::Unreachable, roku::powerFromDeviceInfo(""));
  TEST_ASSERT_EQUAL(TvPower::Unreachable, roku::powerFromDeviceInfo("ECP command not allowed in limited mode"));
}

void test_active_app() {
  TEST_ASSERT_EQUAL_STRING("837", roku::activeAppId("<active-app>\n\t<app id=\"837\" type=\"appl\" "
                                                    "version=\"2.20.100005021\">YouTube</app>\n</active-app>")
                                      .c_str());
  TEST_ASSERT_EQUAL_STRING("", roku::activeAppId("<active-app><app>Roku</app></active-app>").c_str());
  TEST_ASSERT_EQUAL_STRING("tvinput.hdmi1",
                           roku::activeAppId("<active-app><app id=\"tvinput.hdmi1\" type=\"tvin\">HDMI 1</app>"
                                             "<screensaver id=\"55545\">x</screensaver></active-app>")
                               .c_str());
  // id attribute of a later element must not be picked up for an id-less <app>.
  TEST_ASSERT_EQUAL_STRING(
      "", roku::activeAppId("<active-app><app>Roku</app><screensaver id=\"1\">s</screensaver></active-app>").c_str());
}

void test_ssdp_and_mac() {
  const char* resp =
      "HTTP/1.1 200 OK\r\nCache-Control: max-age=3600\r\nST: roku:ecp\r\n"
      "Location: http://192.168.1.23:8060/\r\nUSN: uuid:roku:ecp:X00000000000\r\n\r\n";
  TEST_ASSERT_EQUAL_STRING("192.168.1.23", roku::ssdpLocationHost(resp).c_str());
  TEST_ASSERT_EQUAL_STRING("", roku::ssdpLocationHost("HTTP/1.1 200 OK\r\n\r\n").c_str());

  uint8_t mac[6];
  TEST_ASSERT_TRUE(roku::parseMac(roku::tagText(kDeviceInfoOn, "wifi-mac"), mac));
  TEST_ASSERT_EQUAL_HEX8(0xa8, mac[0]);
  TEST_ASSERT_EQUAL_HEX8(0xff, mac[5]);
  TEST_ASSERT_FALSE(roku::parseMac("00:00:00:00:00:00", mac));
  TEST_ASSERT_FALSE(roku::parseMac("", mac));
  TEST_ASSERT_FALSE(roku::parseMac("zz:b1:c2:00:11:ff", mac));
}

void test_url_encode() {
  TEST_ASSERT_EQUAL_STRING("abc-_.~123", roku::urlEncode("abc-_.~123").c_str());
  TEST_ASSERT_EQUAL_STRING("a%20b%26c", roku::urlEncode("a b&c").c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_power_mode);
  RUN_TEST(test_active_app);
  RUN_TEST(test_ssdp_and_mac);
  RUN_TEST(test_url_encode);
  return UNITY_END();
}
