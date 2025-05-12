#ifndef MOUSE_DEFS_H_
#define MOUSE_DEFS_H_

/* Protocol definitions */
#define MOUSE_LMB_BIT 5 // Defines << shift for bit position
#define MOUSE_RMB_BIT 4
#define MOUSE_MMB_BIT 4 // Shift 4 times in 4th byte

typedef struct mouse_proto {
  char    name[12];
  uint8_t serial_ident[2]; // M, M3, MZ..
  int     serial_ident_len;
  int     buttons;
  bool    wheel;
  int     report_len;
} mouse_proto_t;

extern mouse_proto_t mouse_protocol[3]; // Global options
extern uint mouse_protocol_num;

enum MOUSE_PROTOCOLS {
  PROTO_MS2BUTTON = 0, // 2 buttons, 3 bytes
  PROTO_LOGITECH  = 1, // 3 buttons, 3-4 bytes
  PROTO_MSWHEEL   = 2, // 3 buttons, wheel, 4 bytes.
  PROTO_MOUSESYS  = 3  // Mouse systems, TBD
};

// Delay between data packets for 1200 baud
#define U_FULL_SECOND 1000000L      // 1s in microseconds
#define U_SERIALDELAY_1B  7500      // 1 byte
#define U_SERIALDELAY_3B  22700     // 3 bytes (microseconds)
#define U_SERIALDELAY_4B  30000     // 4 bytes (microseconds)
// 1200 baud (bits/s) is 133.333333333... bytes/s
// 44.44.. updates per second with 3 bytes.
// 33.25.. updates per second with 4 bytes.
// ~0.0075 seconds per byte, target time calculated for 4 bytes.
#define NS_FULL_SECOND    1000000000L // 1s in nanoseconds
#define NS_SERIALDELAY_1B   7500000   // 1 byte
#define NS_SERIALDELAY_3B   22700000  // 3 bytes
#define NS_SERIALDELAY_4B   30000000  // 4 bytes

// Struct for storing information about accumulated mouse state
typedef struct mouse_state {
  int pc_state; // Current state of mouse driver initialization on PC.
  uint8_t state[4]; // Mouse state
  int x, y, wheel;
  int update; // How many bytes to send
  bool lmb, rmb, mmb, force_update;
} mouse_state_t;

// Struct for user settable mouse options
typedef struct mouse_opts {
  uint protocol;
  float sensitivity; // Sensitivity coefficient
  bool wheel;
  bool swap_buttons;
} mouse_opts_t;

extern mouse_opts_t mouse_options; // Global options

// States of mouse init request from PC
enum PC_INIT_STATES {
  CTS_UNINIT   = 0, // Initial state
  CTS_LOW_INIT = 1, // CTS pin has been set low, wait for high.
  CTS_TOGGLED  = 2, // CTS was low, now high -> do ident.
  CTS_LOW_RUN  = 3  // CTS drops low post-initialization, Windows behavior.
};

#endif // MOUSE_DEFS_H_
