#include <gtest/gtest.h>

extern "C" {
  #include "../../shared/crc8/crc8.h"
  #include "../../shared/mouse.h"
  #include "../../shared/settings.h"
}

class SettingsTest : public testing::Test {
  protected:

  settings_bin_t binary_settings;

  // Set-up once per test suite
  static void SetUpTestSuite() {
  }
 
  // Tear-down once per test suite
  static void TearDownTestSuite() {
  }
 
  // Per-test set-up logic as usual.
  void SetUp() override { 

    // Pre-calculated OK settings
    binary_settings.size = 8;
    binary_settings.bytes[0] = 0x4D;
    binary_settings.bytes[1] = 0x6F;
    binary_settings.bytes[2] = 0x00;
    binary_settings.bytes[3] = 0x14; // Ver 0, MS2Button, Sens 1.0, No swap, No wheel
    binary_settings.bytes[4] = 0x00;
    binary_settings.bytes[5] = 0x75;
    binary_settings.bytes[6] = 0x53;
    binary_settings.bytes[7] = 0x0E;

  }
 
  // Per-test tear-down logic
  void TearDown() override {  }
  
  void UpdateCRC() {
    binary_settings.bytes[7] = crc8(&binary_settings.bytes[0], 7, (uint8_t)0x00); // Update to correct CRC
  }

 };


/*** Base tests for encoding/decoding configuration options ***/

TEST_F(SettingsTest, DecodeFailsCRC) {

  binary_settings.bytes[3] = 0xAD; // Falsify options, should fail CRC

  mouse_opts_t mouse_options_decoded;
  EXPECT_FALSE(settings_decode(&binary_settings, &mouse_options_decoded));
}

TEST_F(SettingsTest, DecodeFailsCanary) {

  binary_settings.bytes[0] = 0xFF; // Falsify canary
  UpdateCRC();

  mouse_opts_t mouse_options_decoded;
  EXPECT_FALSE(settings_decode(&binary_settings, &mouse_options_decoded));
}

TEST_F(SettingsTest, DecodeFailsVersion) {

  binary_settings.bytes[2] = 0xFF; // Falsify version
  UpdateCRC();

  mouse_opts_t mouse_options_decoded;
  EXPECT_FALSE(settings_decode(&binary_settings, &mouse_options_decoded));
}

TEST_F(SettingsTest, EncodeSucceeds) {

  mouse_opts_t mouse_options;
  settings_bin_t test_binary_settings;

  mouse_options.protocol = PROTO_MS2BUTTON;
  mouse_options.sensitivity = 1.0;
  mouse_options.swap_buttons = false;
  mouse_options.wheel = false;

  settings_encode(&test_binary_settings, &mouse_options);

  for(int i=0; i < test_binary_settings.size; i++) {
    EXPECT_EQ(test_binary_settings.bytes[i], binary_settings.bytes[i]);
  }  
}

TEST_F(SettingsTest, DecodeSucceeds) {

  mouse_opts_t mouse_options;
  EXPECT_TRUE(settings_decode(&binary_settings, &mouse_options));

  EXPECT_EQ(mouse_options.protocol, PROTO_MS2BUTTON);
  EXPECT_EQ(mouse_options.sensitivity, 1.0);
  EXPECT_EQ(mouse_options.swap_buttons, false);
  EXPECT_EQ(mouse_options.wheel, false);
}

TEST_F(SettingsTest, EncodeDecodeSucceeds) {

  mouse_opts_t mouse_options;
  settings_bin_t binary_settings;

  mouse_options.protocol = PROTO_MS2BUTTON;
  mouse_options.sensitivity = 1.0;
  mouse_options.swap_buttons = true;
  mouse_options.wheel = true;

  settings_encode(&binary_settings, &mouse_options);

  mouse_opts_t mouse_options_decoded;
  EXPECT_TRUE(settings_decode(&binary_settings, &mouse_options_decoded));

  EXPECT_EQ(mouse_options_decoded.protocol, PROTO_MS2BUTTON);
  EXPECT_EQ(mouse_options_decoded.sensitivity, 1.0);
  EXPECT_EQ(mouse_options_decoded.swap_buttons, true);
  EXPECT_EQ(mouse_options_decoded.wheel, true);
}
