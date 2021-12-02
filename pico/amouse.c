/*
 *   __ _   _ __  ___ _  _ ___ ___ 
 *  / _` | | '  \/ _ \ || (_-</ -_)
 *  \__,_| |_|_|_\___/\_,_/__/\___=====_____)
 *
 * Anachro Mouse, a usb to serial mouse adaptor. Copyright (C) 2021 Aviancer <oss+amouse@skyvian.me>
 *
 * This library is free software; you can redistribute it and/or modify it under the terms of the 
 * GNU Lesser General Public License as published by the Free Software Foundation; either version 
 * 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this library; 
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
*/

#include <time.h>
#include "stdbool.h"
#include "pico/stdlib.h"

#include "include/serial.h"
#include "../shared/utils.h"
#include "../shared/mouse.h"

#include "bsp/board.h"
#include "tusb.h"

// Technically could support multiple mice connected to the same system if we kept more mouse states in memory.


/*** Program parameters ***/ 

// Struct for storing pointers to dynamically allocated memory containing options.
typedef struct opts {
  int wheel;
} opts_t;

void set_opts(struct opts *options) {
  options->wheel = 1;
}

// NEW
#define MAX_HID_REPORT  4
// Each HID instance can has multiple reports
static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_HID_REPORT];
}hid_info[CFG_TUH_HID];


/*** Global state variables ****/

// Set default options, support mouse wheel.
opts_t options = { .wheel=1 };

extern mouse_state_t mouse; // Needs to be available for serial functions.
mouse_state_t mouse; // int values default to 0 

static uint32_t txtimer_target; // Serial transmit timer target time

// Aggregate movements before sending
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report;
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report_prev;
CFG_TUSB_MEM_SECTION static char serial_in_buffer[64] = { 0 };

// DEBUG
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
int led_state = 0;


/*** Timing ***/

void queue_tx(mouse_state_t *mouse) {
  // Update timer target for next transmit
  // Use variable send rate depending on whether a 3 or 4 byte update was sent
  if(mouse->update > 2) { txtimer_target = time_us_32() + U_SERIALDELAY_4B; }
  else                  { txtimer_target = time_us_32() + U_SERIALDELAY_3B; }
}


/*** USB comms ***/

static inline void process_mouse_report(mouse_state_t *mouse, hid_mouse_report_t const *p_report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

// invoked ISR context
void tuh_cdc_xfer_isr(uint8_t dev_addr, xfer_result_t event, cdc_pipeid_t pipe_id, uint32_t xferred_bytes)
{
  (void) event;
  (void) pipe_id;
  (void) xferred_bytes;

  printf(serial_in_buffer);
  tu_memclr(serial_in_buffer, sizeof(serial_in_buffer));

  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // waiting for next data
}

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  //printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  //printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_HID_REPORT, desc_report, desc_len);
    //printf("HID has %u reports \r\n", hid_info[instance].report_count);
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    //printf("Error: cannot request to receive report\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  //printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      //TU_LOG2("HID receive boot keyboard report\r\n");
      //process_kbd_report( (hid_keyboard_report_t const*) report );
    break;

    case HID_ITF_PROTOCOL_MOUSE:
      //TU_LOG2("HID receive boot mouse report\r\n");
      process_mouse_report(&mouse, (hid_mouse_report_t const*) report );
    break;

    default:
      //TU_LOG2("HID receive generic report\r\n");
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    //printf("Error: cannot request to receive report\r\n");
  }
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) dev_addr;

  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the arrray
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info)
  {
    //printf("Couldn't find the report info for this report !\r\n");
    return;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        //TU_LOG1("HID receive keyboard report\r\n");
        // Assume keyboard follow boot report layout
        //process_kbd_report( (hid_keyboard_report_t const*) report );
      break;

      case HID_USAGE_DESKTOP_MOUSE:
        //TU_LOG1("HID receive mouse report\r\n");
        // Assume mouse follow boot report layout
        process_mouse_report(&mouse, (hid_mouse_report_t const*) report );
      break;

      default: break;
    }
  }
}


/*** Mouse specific USB handling ***/

int test_mouse_button(uint8_t buttons_state, uint8_t button) {
  if(buttons_state & button) { return 1; }
  return 0;
}

static inline void process_mouse_report(mouse_state_t *mouse, hid_mouse_report_t const *p_report) {
 
  uint8_t button_changed_mask = p_report->buttons ^usb_mouse_report_prev.buttons; // xor to set bits true if any state is different.
  //if(button_changed_mask & p_report->buttons) { // Could be used to act only on any button down press.
  if(button_changed_mask) { // If button pressed or released
    mouse->force_update = 1;

    mouse->lmb = test_mouse_button(p_report->buttons, MOUSE_BUTTON_LEFT);
    mouse->rmb = test_mouse_button(p_report->buttons, MOUSE_BUTTON_RIGHT);
    push_update(mouse, mouse->mmb);

    if(options.wheel && (button_changed_mask & MOUSE_BUTTON_MIDDLE)) {
      mouse->mmb = test_mouse_button(p_report->buttons, MOUSE_BUTTON_MIDDLE);
      push_update(mouse, true);
    }
  }
    
  // ### Handle relative movement ###
  // Clamp to larger than valid protocol output values to allow for sensitivity scaling.
  if(p_report->x) {
    mouse->x += p_report->x;
    mouse->x  = clampi(mouse->x, -36862, 36862); 
    push_update(mouse, mouse->mmb);
  }
  if(p_report->y) {
    mouse->y += p_report->y;
    mouse->y  = clampi(mouse->y, -36862, 36862);
    push_update(mouse, mouse->mmb);
  }
  if(options.wheel && p_report->wheel) {
      mouse->wheel += p_report->wheel;
      mouse->wheel  = clampi(mouse->wheel, -63, 63);
      push_update(mouse, true);
  }

  // Update previous mouse state
  usb_mouse_report_prev = *p_report;
}


/*** Main init & loop ***/

int main() {
  // Initialize serial parameters 
  mouse_serial_init(0); 

  // Set up initial state 
  //enable_pins(UART_RTS_BIT | UART_DTR_BIT);
  reset_mouse_state(&mouse);
  mouse.pc_state = CTS_UNINIT;
  mouse.sensitivity = 1.0;

  // Initialize USB
  tusb_init();

  // Onboard LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // CTS Pin
  gpio_init(UART_CTS_PIN); 
  gpio_set_dir(UART_CTS_PIN, GPIO_IN);

  // Set initial serial transmit timer target
  txtimer_target = time_us_32() + U_SERIALDELAY_3B; 

  while(1) {
    bool cts_pin = gpio_get(UART_CTS_PIN);

    // ### Check if mouse driver trying to initialize
    if(cts_pin) { // Computers RTS is low, with MAX3232 this shows reversed as high instead? Check spec.
      mouse.pc_state = CTS_LOW_INIT;
      gpio_put(LED_PIN, false);
    }

    // ### Mouse initiaizing request detected
    if(!cts_pin && mouse.pc_state == CTS_LOW_INIT) {
      mouse.pc_state = CTS_TOGGLED;
      mouse_ident(0, options.wheel);
    }

    /*** Mouse update loop ***/
    if(mouse.pc_state == CTS_TOGGLED) {
      //led_state ^= 1; // Flip state between 0/1 // DEBUG
      gpio_put(LED_PIN, true);

      tuh_task(); // tinyusb host task
 
      if(time_reached(txtimer_target) || mouse.force_update) {
        runtime_settings(&mouse);
	input_sensitivity(&mouse);
	update_mouse_state(&mouse);

	queue_tx(&mouse); // Update next serial timing
        serial_write(0, mouse.state, mouse.update);
        reset_mouse_state(&mouse);
      }

    }
    //sleep_us(1);
  }

  return(0);
}
