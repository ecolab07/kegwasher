#include <stdarg.h> 
#include <EEPROM.h>
#include <Bounce2.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>

// ============================================================
// Pin assignments
// ============================================================
#define PIN_BUZZER                A0
#define PIN_MENUSELECT_1          A1  // Rotary encoder A
#define PIN_MENUSELECT_2          A2  // Rotary encoder B
#define PIN_BUTTON_ACTION         A3  // Confirm / cancel button
#define PIN_VALVE_AIR             2
#define PIN_VALVE_CO2             3
#define PIN_VALVE_WATER           4
#define PIN_VALVE_CLEANER_IN      5
#define PIN_VALVE_SANITIZER_IN    6
#define PIN_VALVE_CLEANER_OUT     7
#define PIN_VALVE_SANITIZER_OUT   8
#define PIN_VALVE_DRAIN           9
#define PIN_PUMP                  10
#define PIN_LED                   11

#define DISPLAY_I2C_ADDRESS       0x27

// ============================================================
// Individual actuator control bits (9 bits, one per output)
// ============================================================
#define CTRL_WATER          0b000000001
// Tank outlet = circuit inlet (liquid drawn from cleaner tank)
#define CTRL_CLEANER_IN     0b000000010
// Tank outlet = circuit inlet (liquid drawn from sanitizer tank)
#define CTRL_SANITIZER_IN   0b000000100
#define CTRL_AIR            0b000001000
#define CTRL_CO2            0b000010000
#define CTRL_DRAIN          0b000100000
// Tank upper inlet = circuit outlet (liquid returns to cleaner tank)
#define CTRL_CLEANER_OUT    0b001000000
// Tank upper inlet = circuit outlet (liquid returns to sanitizer tank)
#define CTRL_SANITIZER_OUT  0b010000000
#define CTRL_PUMP           0b100000000

// Extended flags used by the "Test vannes" mode only (bits 10 and 11,
// outside the range of real actuator bits)
#define CONFIG_WARNING  0b1000000000   // Prompt operator before firing any actuator (bit 10)
#define CONFIG_WAIT     0b10000000000  // Idle gap between individual actuator pulses (bit 11)

// ============================================================
// Composite valve configurations used by normal wash sequences
// ============================================================
#define CONFIG_DRAIN            (CTRL_DRAIN)
#define CONFIG_DRAIN_SANITIZER  (CTRL_PUMP + CTRL_SANITIZER_IN + CTRL_DRAIN)
#define CONFIG_DRAIN_CLEANER    (CTRL_PUMP + CTRL_CLEANER_IN + CTRL_DRAIN)
#define CONFIG_FILL_SANITIZER   (CTRL_PUMP + CTRL_SANITIZER_OUT + CTRL_WATER)
#define CONFIG_FILL_CLEANER     (CTRL_PUMP + CTRL_CLEANER_OUT + CTRL_WATER)
#define CONFIG_RINCE            (CTRL_PUMP + CTRL_WATER + CTRL_DRAIN)
#define CONFIG_RINCE_PURGE      (CTRL_AIR + CTRL_DRAIN)
#define CONFIG_RINCE_PURGE_CO2  (CTRL_CO2 + CTRL_DRAIN)
#define CONFIG_CLEAN            (CTRL_PUMP + CTRL_CLEANER_IN + CTRL_CLEANER_OUT)
#define CONFIG_CLEAN_PURGE      (CTRL_AIR + CTRL_CLEANER_OUT)
#define CONFIG_SANITIZE         (CTRL_PUMP + CTRL_SANITIZER_IN + CTRL_SANITIZER_OUT)
#define CONFIG_SANITIZE_PURGE   (CTRL_AIR + CTRL_SANITIZER_OUT)
#define CONFIG_CO2              (CTRL_CO2)
#define CONFIG_END              0  // Sentinel value marking the end of a step array

// LED blink period in seconds during a running sequence
#define LED_BLINK_PERIOD    2

// Relay logic: relays are active-low
#define VALVE_CLOSE HIGH
#define VALVE_OPEN  LOW

// ============================================================
// Data types
// ============================================================

// One step in a wash sequence: a valve configuration and its duration in seconds
typedef struct step_s {
  unsigned int  config;
  unsigned long duration;
} step_t;

// A named wash mode and its associated step array. `steps` points into
// flash (PROGMEM) — always read it via read_step(), never dereference
// it directly.
typedef struct mode_s {
  const char *name;
  const step_t *steps;
} mode_t;

// ============================================================
// Step arrays — one per operational mode
//
// Marked PROGMEM: these stay in flash instead of being copied into RAM
// at boot. Read them only through read_step(), which copies a single
// entry into a RAM-resident step_t via memcpy_P() — direct indexing
// (steps[i].config) would read garbage on AVR once a table is PROGMEM.
// ============================================================

// Drain sanitizer tank through the keg circuit — 200 s (3 min 20)
const step_t STEPS_DRAIN_SANITIZER[] PROGMEM = {
  {CONFIG_DRAIN_SANITIZER, 200},
  {CONFIG_END, 0}
};

// Drain cleaner tank through the keg circuit — 200 s (3 min 20)
const step_t STEPS_DRAIN_CLEANER[] PROGMEM = {
  {CONFIG_DRAIN_CLEANER, 200},
  {CONFIG_END, 0}
};

// Fill sanitizer tank — 120 s (2 min 00)
const step_t STEPS_FILL_SANITIZER[] PROGMEM = {
  {CONFIG_FILL_SANITIZER, 120},
  {CONFIG_END, 0}
};

// Fill cleaner tank — 120 s (2 min 00)
const step_t STEPS_FILL_CLEANER[] PROGMEM = {
  {CONFIG_FILL_CLEANER, 120},
  {CONFIG_END, 0}
};

// Full wash cycle without CO2 pressurisation — 325 s (5 min 25)
// Reference: https://www.btobeer.com/themes-conseils-techniques-bieres-brasseries/conseils-carbonatation-process-et-analyses/futs-de-biere-a-plongeurs-incorpores-problemes-lies-au-lavage-et-sterilisation-des-futs
const step_t STEPS_WASH_KEG[] PROGMEM = {
  // ===== Initial drain =====
  {CONFIG_DRAIN, 10},           // Adjust if needed

  // ===== Initial rinse =====
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},

  // ===== Detergent cycle =====
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 20},

  // ===== Intermediate rinse =====
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 7},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},

  // ===== Sanitization cycle =====
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 30},        // Adjust if needed
  {CONFIG_SANITIZE_PURGE, 20},

  // ===== Final rinse + air purge =====
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 30},     // Adjust if needed

  {CONFIG_END, 0}
};

// Detergent-only cycle (no sanitization, no CO2) — 185 s (3 min 05)
const step_t STEPS_DETER_KEG[] PROGMEM = {
  {CONFIG_DRAIN, 10},           // Adjust if needed

  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},

  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 20},

  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 7},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_END, 0}
};

// Full wash cycle with CO2 pressurisation at the end — 335 s (5 min 35)
const step_t STEPS_WASH_KEG_PRESSURIZE[] PROGMEM = {
  // ===== Initial drain =====
  {CONFIG_DRAIN, 10},           // Adjust if needed

  // ===== Initial rinse =====
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},

  // ===== Detergent cycle =====
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},

  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},

  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 20},

  // ===== Intermediate rinse =====
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},

  {CONFIG_RINCE, 7},
  {CONFIG_RINCE_PURGE, 10},

  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},

  // ===== Sanitization cycle =====
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},

  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},

  {CONFIG_SANITIZE, 30},        // Adjust if needed
  {CONFIG_SANITIZE_PURGE, 20},

  // ===== Final rinse + CO2 purge + pressurisation =====
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE_CO2, 30}, // Adjust if needed
  {CONFIG_CO2, 10},             // Adjust if needed

  {CONFIG_END, 0}
};

// Sanitization-only cycle with CO2 pressurisation — 190 s (3 min 10)
const step_t STEPS_SANITIZE_KEG_PRESSURIZE[] PROGMEM = {
  {CONFIG_DRAIN, 10},           // Adjust if needed

  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},

  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 30},        // Adjust if needed
  {CONFIG_SANITIZE_PURGE, 20},

  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE_CO2, 30}, // Adjust if needed

  {CONFIG_CO2, 10},             // Adjust if needed
  {CONFIG_END, 0}
};

// CO2 pressurisation only — 40 s
const step_t STEPS_KEG_PRESSURIZE[] PROGMEM = {
  {CONFIG_DRAIN, 10},           // Adjust if needed
  {CONFIG_RINCE_PURGE_CO2, 30}, // Adjust if needed
  {CONFIG_CO2, 10},             // Adjust if needed
  {CONFIG_END, 0}
};

// Keg drain + air purge — 70 s (1 min 10)
const step_t STEPS_DRAIN_KEG[] PROGMEM = {
  {CONFIG_DRAIN, 10},           // Adjust if needed
  {CONFIG_RINCE_PURGE, 60},
  {CONFIG_END, 0}
};

// Actuator click-test — 24 s total. Maintenance check, kept in production:
// fires each valve and the pump individually, one at a time, in relay-board
// order (left to right, top to bottom), with a 1 s idle gap between each
// pulse — a quick way to confirm every actuator still clicks and every
// wire is still connected, without running a full wash cycle.
// WARNING: ensure all tanks are empty before running this mode.
const step_t STEPS_TEST_ACTUATORS[] PROGMEM = {
    {CONFIG_WARNING, 5},      // Display safety prompt: confirm tanks are empty
    {CONFIG_WAIT,    1},      // Idle gap

    {CTRL_SANITIZER_OUT, 1},  // Sanitizer return valve
    {CONFIG_WAIT,        1},
    {CONFIG_DRAIN,         1},  // Drain valve
    {CONFIG_WAIT,        1},
    {CTRL_CLEANER_OUT,   1},  // Cleaner return valve
    {CONFIG_WAIT,        1},
    {CTRL_AIR,           1},  // Air inlet valve
    {CONFIG_WAIT,        1},
    {CONFIG_CO2,           1},  // CO2 inlet valve
    {CONFIG_WAIT,        1},
    {CTRL_SANITIZER_IN,  1},  // Sanitizer feed valve
    {CONFIG_WAIT,        1},
    {CTRL_WATER,         1},  // Water inlet valve
    {CONFIG_WAIT,        1},
    {CTRL_CLEANER_IN,    1},  // Cleaner feed valve
    {CONFIG_WAIT,        1},
    {CTRL_PUMP,          1},  // Circulation pump
    {CONFIG_WAIT,        1},

    {CONFIG_END, 0}
};

// ============================================================
// Mode table — order determines rotary-encoder menu order
// ============================================================
mode_t MODES[] = {
  {"Lavage + CO2",    STEPS_WASH_KEG_PRESSURIZE},
  {"Lavage sans CO2", STEPS_WASH_KEG},
  {"Detergent seul",  STEPS_DETER_KEG},
  {"CO2",             STEPS_KEG_PRESSURIZE},
  {"Desinf. + CO2",   STEPS_SANITIZE_KEG_PRESSURIZE},
  {"Vidange fut",     STEPS_DRAIN_KEG},
  {"Vidange desinf.", STEPS_DRAIN_SANITIZER},
  {"Vidange deter.",  STEPS_DRAIN_CLEANER},
  {"Rempl. desinf.",  STEPS_FILL_SANITIZER},
  {"Rempl. deter.",   STEPS_FILL_CLEANER},
  {"Test vannes",     STEPS_TEST_ACTUATORS},
};

// Computed at runtime so new modes added above are counted automatically
const int MODES_NUMBER = sizeof(MODES) / sizeof(mode_t);

// ============================================================
// Hardware objects
// ============================================================
RotaryEncoder menuselect(PIN_MENUSELECT_1, PIN_MENUSELECT_2);
Bounce buttonAction = Bounce();

LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDRESS, 16, 2);

// Custom LCD character: up/down arrows used in the selection screen
#define CHAR_UP_DOWN  1
byte CHAR_UP_DOWN_SETUP[] = {
  B00100,
  B01010,
  B10001,
  B00000,
  B00000,
  B10001,
  B01010,
  B00100
};

// EEPROM address where the last selected mode index is persisted
#define EEPROM_ADDRESS_MODE        0
// 1 while a step-array mode is running, written back to 0 only by a clean
// terminate() or cancel(). If this is still 1 at boot, the Arduino restarted
// mid-cycle (most likely a relay-module brown-out) instead of finishing
// normally — see resume_warning().
#define EEPROM_ADDRESS_CYCLE_FLAG  1
// Index of the last step reached before the interruption (diagnostic only)
#define EEPROM_ADDRESS_CYCLE_STEP  2

// ============================================================
// Reset-cause capture
//
// Reads MCUSR (which AVR sets on every reset and does not clear itself)
// as early as possible — before Arduino's own init() runs — so we can
// tell a normal power-up apart from a brown-out (voltage dip), external,
// or watchdog reset. This is what lets resume_warning() report whether
// an interrupted cycle was actually caused by a power dip.
// ============================================================
#if defined(__AVR__)
#include <avr/wdt.h>

uint8_t reset_cause __attribute__((section(".noinit")));

void capture_reset_cause(void) __attribute__((naked, used, section(".init3")));
void capture_reset_cause(void)
{
  reset_cause = MCUSR;
  MCUSR = 0;
  wdt_disable();
}

// Cause labels kept in flash (PROGMEM): read only via lcd_printf_P().
const char MSG_CAUSE_BOR[]     PROGMEM = "coupure (BOR)";
const char MSG_CAUSE_WDT[]     PROGMEM = "watchdog";
const char MSG_CAUSE_EXT[]     PROGMEM = "reset externe";
const char MSG_CAUSE_PWR[]     PROGMEM = "mise s. tension";
const char MSG_CAUSE_UNKNOWN[] PROGMEM = "inconnue";

// Short label for the captured reset cause (kept short: 16-char LCD line).
// Returns a flash pointer — display it with lcd_printf_P(), not lcd_printf().
const char* reset_cause_label()
{
  if( reset_cause & (1 << BORF) )  return MSG_CAUSE_BOR;
  if( reset_cause & (1 << WDRF) )  return MSG_CAUSE_WDT;
  if( reset_cause & (1 << EXTRF) ) return MSG_CAUSE_EXT;
  if( reset_cause & (1 << PORF) )  return MSG_CAUSE_PWR;
  return MSG_CAUSE_UNKNOWN;
}
#else
// Non-AVR fallback so this file stays portable for off-target syntax checks.
const char* reset_cause_label() { return "inconnue"; }
#endif

// ============================================================
// State machine
// ============================================================
typedef enum state_e {
  STATE_SELECT,         // Enter selection screen
  STATE_SELECT_UPDATE,  // Poll encoder and button on selection screen
  STATE_RUN,            // Initialise and start the selected mode
  STATE_RUN_UPDATE,     // Advance through steps and update display
  STATE_TERMINATE,      // Sequence completed normally
  STATE_CANCEL,         // Sequence aborted by the operator

  // Shown at boot only, if the previous cycle never reached a clean exit
  STATE_RESUME_WARNING,         // Display the interrupted-cycle warning
  STATE_RESUME_WARNING_UPDATE   // Poll button to acknowledge and clear the flag
} state_t;

state_t state = STATE_SELECT;

// ============================================================
// Global runtime variables
// ============================================================
int           mode = 0;          // Index of the currently selected mode
unsigned long mode_start_time;   // Timestamp (s) when the mode started
unsigned long mode_full_time;    // Total duration of the current mode in seconds

int           step;              // Index of the current step within the mode
unsigned long step_start_time;   // Timestamp (s) when the current step started

// ============================================================
// Helpers
// ============================================================

// Printf-style wrapper for the LCD: formats into a 16-character
// left-justified string and prints it at the current cursor position.
// Use this when `fmt` (or any %s argument) lives in RAM.
void lcd_printf(const char *fmt, ...)
{
  char buf1[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf1, sizeof(buf1), fmt, args);
  va_end(args);

  char buf2[17];
  snprintf(buf2, sizeof(buf2), "%-16s", buf1);
  lcd.print(buf2);
}

// Same as lcd_printf(), but for a format string that lives in flash
// (PROGMEM) — e.g. the fixed headers on the interrupted-cycle screen,
// or the strings returned by reset_cause_label().
void lcd_printf_P(const char *fmt, ...)
{
  char buf1[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf_P(buf1, sizeof(buf1), fmt, args);
  va_end(args);

  char buf2[17];
  snprintf(buf2, sizeof(buf2), "%-16s", buf1);
  lcd.print(buf2);
}

// Returns the current time in whole seconds.
unsigned long seconds()
{
  return millis() / 1000;
}

// Returns a human-readable label for the given valve configuration.
const char* resolve_label(unsigned int config)
{
  switch(config) {
    case CONFIG_DRAIN:           return "Vidange";
    case CONFIG_DRAIN_SANITIZER: return "Vidange desinf.";
    case CONFIG_DRAIN_CLEANER:   return "Vidange deter.";
    case CONFIG_FILL_SANITIZER:  return "Rempl. desinf.";
    case CONFIG_FILL_CLEANER:    return "Rempl. deter.";
    case CONFIG_RINCE:           return "Rincage";
    case CONFIG_RINCE_PURGE:     return "Purge air";
    case CONFIG_RINCE_PURGE_CO2: return "Purge CO2";
    case CONFIG_CLEAN:           return "Detergent";
    case CONFIG_CLEAN_PURGE:     return "Purge deter.";
    case CONFIG_SANITIZE:        return "Desinfectant";
    case CONFIG_SANITIZE_PURGE:  return "Purge desinf.";
    case CONFIG_CO2:             return "CO2";
    case CTRL_WATER:             return "Eau";
    case CTRL_CLEANER_IN:        return "Deter. IN";
    case CTRL_SANITIZER_IN:      return "Desinf. IN";
    case CTRL_AIR:               return "Air";
    case CTRL_CLEANER_OUT:       return "Deterg. OUT";
    case CTRL_SANITIZER_OUT:     return "Desinf. OUT";
    case CTRL_PUMP:              return "Pompe";
    case CONFIG_WARNING:         return "Cuves vides ??";
    case CONFIG_WAIT:            return "Attente...";
    default:                     return "";
  }
}

// Copies a single step from a PROGMEM step array into a RAM-resident
// step_t. Every access to a STEPS_*[] table must go through this —
// direct indexing (steps[i].config) is undefined once the array is
// PROGMEM, since it would read raw flash bytes as if they were RAM.
step_t read_step(const step_t *steps, int index)
{
  step_t s;
  memcpy_P(&s, &steps[index], sizeof(step_t));
  return s;
}

// ============================================================
// State handlers
// ============================================================

// Display the mode selection screen and turn the LED on.
void select()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd_printf("Mode :");
  lcd.setCursor(0, 1);
  lcd_printf("%c %s", CHAR_UP_DOWN, MODES[mode].name);

  digitalWrite(PIN_LED, HIGH);

  state = STATE_SELECT_UPDATE;
}

// Poll the rotary encoder and button during mode selection.
void select_update()
{
  int new_mode;
  int pos;

  menuselect.tick();
  pos = menuselect.getPosition();

  // Modulo always positive, regardless of the amplitude
  // NOTE: no encoder re-anchoring here (unlike an earlier revision of this
  // sketch) — select() is currently the only screen polling menuselect,
  // so the raw position can never drift out of sync. If a second screen
  // ever polls the encoder too (e.g. a settings submenu), reintroduce a
  // per-screen base offset (position - current_index, applied on entry)
  // to keep the two menus independent — see the diagnostic sketch's
  // menuselect_base pattern for a working example of that.
  new_mode = ((pos % MODES_NUMBER) + MODES_NUMBER) % MODES_NUMBER;

  if( new_mode != mode ) {
    mode = new_mode;
    lcd.setCursor(0, 1);
    lcd_printf("%c %s", CHAR_UP_DOWN, MODES[mode].name);
  }

  buttonAction.update();
  if( buttonAction.fell() ) {
    state = STATE_RUN;
  }
}

// Closes the actuators in the given bitmask, in safe order.
void close_actuators(unsigned int config)
{
  // Closing order: stop pressurised sources first, then outputs.
  // This prevents pressure from being trapped in the circuit.
  if(config & CTRL_PUMP)          { digitalWrite(PIN_PUMP,              VALVE_CLOSE); delay(200); }
  if(config & CTRL_WATER)         { digitalWrite(PIN_VALVE_WATER,       VALVE_CLOSE); delay(200); }
  if(config & CTRL_CO2)           { digitalWrite(PIN_VALVE_CO2,         VALVE_CLOSE); delay(200); }
  if(config & CTRL_AIR)           { digitalWrite(PIN_VALVE_AIR,         VALVE_CLOSE); delay(200); }
  if(config & CTRL_CLEANER_IN)    { digitalWrite(PIN_VALVE_CLEANER_IN,  VALVE_CLOSE); delay(200); }
  if(config & CTRL_SANITIZER_IN)  { digitalWrite(PIN_VALVE_SANITIZER_IN,VALVE_CLOSE); delay(200); }
  if(config & CTRL_CLEANER_OUT)   { digitalWrite(PIN_VALVE_CLEANER_OUT, VALVE_CLOSE); delay(200); }
  if(config & CTRL_SANITIZER_OUT) { digitalWrite(PIN_VALVE_SANITIZER_OUT,VALVE_CLOSE);delay(200); }
  if(config & CTRL_DRAIN)         { digitalWrite(PIN_VALVE_DRAIN,       VALVE_CLOSE); delay(200); }
}

// Opens the actuators in the given bitmask, in safe order.
void open_actuators(unsigned int config)
{
  // Opening order: outputs first, then inputs, then pump.
  // Ensures the circuit has a clear path before any pressure is applied.
  if(config & CTRL_DRAIN)         { digitalWrite(PIN_VALVE_DRAIN,        VALVE_OPEN); delay(200); }
  if(config & CTRL_CLEANER_OUT)   { digitalWrite(PIN_VALVE_CLEANER_OUT,  VALVE_OPEN); delay(200); }
  if(config & CTRL_SANITIZER_OUT) { digitalWrite(PIN_VALVE_SANITIZER_OUT,VALVE_OPEN); delay(200); }
  if(config & CTRL_CLEANER_IN)    { digitalWrite(PIN_VALVE_CLEANER_IN,   VALVE_OPEN); delay(200); }
  if(config & CTRL_SANITIZER_IN)  { digitalWrite(PIN_VALVE_SANITIZER_IN, VALVE_OPEN); delay(200); }
  if(config & CTRL_PUMP)          { digitalWrite(PIN_PUMP,               VALVE_OPEN); delay(200); }
  if(config & CTRL_AIR)           { digitalWrite(PIN_VALVE_AIR,          VALVE_OPEN); delay(200); }
  if(config & CTRL_CO2)           { digitalWrite(PIN_VALVE_CO2,          VALVE_OPEN); delay(200); }
  if(config & CTRL_WATER)         { digitalWrite(PIN_VALVE_WATER,        VALVE_OPEN); delay(200); }
}

// Transition to a new valve configuration:
// first close everything that is not needed, then open what is needed.
// A 200 ms delay between each actuator change limits inrush current
// from multiple relays switching simultaneously.

// Tracks the previously active configuration to compute minimal transitions.
unsigned int previous_config = 0;

void controls_set(unsigned int config)
{
  unsigned int to_close = previous_config & ~config;
  unsigned int to_open  = config & ~previous_config;

  // Close all actuators not required by the new configuration
  close_actuators(to_close);

  // Open all actuators required by the new configuration
  open_actuators(to_open);

  previous_config = config;
}

// Activate the step at the given index: record its start time,
// apply its valve configuration, and return that configuration.
// step_start_time is recorded AFTER controls_set() so that step duration
// is measured from when the valves are in their target state.
unsigned int step_set(int index)
{
  step = index;
  unsigned int cfg = read_step(MODES[mode].steps, index).config;

  // Record progress in case power is lost before the mode finishes cleanly.
  EEPROM.update(EEPROM_ADDRESS_CYCLE_STEP, index);

  controls_set(cfg);           // blocking transitions happen here
  step_start_time = seconds(); // start chrono AFTER transitions

  return cfg;
}

// Initialise a mode run: save the selected mode to EEPROM, compute the
// total duration, and start step 0.
void run()
{
  // Persist the selected mode so it is pre-selected on next power-up
  int saved_mode = EEPROM.read(EEPROM_ADDRESS_MODE);
  if( mode != saved_mode ) {
    EEPROM.write(EEPROM_ADDRESS_MODE, mode);
  }

  mode_full_time = 0;
  for(int i=0 ; ; i++ ) {
    step_t s = read_step(MODES[mode].steps, i);
    if( s.config == CONFIG_END ) break;
    mode_full_time += s.duration;
  }

  // Mark a cycle as in-progress. Cleared only by a clean terminate() or
  // cancel(); if it's still set at the next boot, we restarted mid-cycle.
  EEPROM.update(EEPROM_ADDRESS_CYCLE_FLAG, 1);

  step_set(0);                   // blocking transitions happen here
  mode_start_time = seconds();   // start chrono AFTER first step transitions

  state = STATE_RUN_UPDATE;
}

// Called every loop iteration while a mode is running.
// Advances to the next step when the current one expires,
// updates the progress display, and blinks the LED + label.
void run_update()
{
  buttonAction.update();
  if( buttonAction.fell() ) {
    state = STATE_CANCEL;
    return;
  }

  step_t cur = read_step(MODES[mode].steps, step);

  // Advance to the next step if the current one has expired
  unsigned long step_running_time = seconds() - step_start_time;
  if( step_running_time >= cur.duration ) {
    unsigned int config = step_set( step + 1 );
    if( config == CONFIG_END ) {
      state = STATE_TERMINATE;
      return;
    }
  }

  // Update the progress line: elapsed / total time
  unsigned long mode_running_time = seconds() - mode_start_time;
  unsigned long rtime_mn = mode_running_time / 60;
  unsigned long rtime_s  = mode_running_time % 60;
  unsigned long ftime_mn = mode_full_time / 60;
  unsigned long ftime_s  = mode_full_time % 60;

  lcd.setCursor(0, 1);
  lcd_printf(" %lumn%02lu / %lumn%02lu", rtime_mn, rtime_s, ftime_mn, ftime_s);

  // Re-read: `step` may have just advanced above, so this reflects
  // whichever step is current now, not the one we started this call with.
  step_t display_step = read_step(MODES[mode].steps, step);

  // Blink the top line between the step label and the mode name
  if( mode_running_time % LED_BLINK_PERIOD < LED_BLINK_PERIOD/2 ) {
    digitalWrite(PIN_LED, HIGH);
    lcd.setCursor(0, 0);
    lcd_printf(resolve_label(display_step.config));
  }
  else {
    digitalWrite(PIN_LED, LOW);
    lcd.setCursor(0, 0);
    lcd_printf(MODES[mode].name);
  }
}

// Called when all steps have completed: close all actuators,
// play three confirmation beeps, and return to the selection screen.
void terminate()
{
  controls_set(0);

  // Clean exit: clear the interrupted-cycle flag.
  EEPROM.update(EEPROM_ADDRESS_CYCLE_FLAG, 0);

  digitalWrite(PIN_LED, LOW);

  lcd.setCursor(0, 1);
  lcd_printf(" Termine");

  for( int i=0 ; i<3 ; i++ ) {
    tone(PIN_BUZZER, 1760, 800);
    delay(1000);
  }

  state = STATE_SELECT;
}

// Called when the operator aborts a running sequence: close all
// actuators, play a single beep, and return to the selection screen.
void cancel()
{
  controls_set(0);

  // Clean exit (operator-initiated): clear the interrupted-cycle flag.
  EEPROM.update(EEPROM_ADDRESS_CYCLE_FLAG, 0);

  digitalWrite(PIN_LED, LOW);

  lcd.setCursor(0, 1);
  lcd_printf(" Annule");

  tone(PIN_BUZZER, 1760, 800);
  delay(1000);

  state = STATE_SELECT;
}

// ------------------------------------------------------------
// Interrupted-cycle warning (shown at boot only)
//
// If EEPROM_ADDRESS_CYCLE_FLAG is still 1 when setup() runs, the previous
// cycle never reached terminate() or cancel() — the Arduino restarted
// mid-run. Surface that instead of silently landing back on the selection
// screen: without this, an operator has no way to know the keg may still
// contain caustic mid-cycle. Requires a button press to acknowledge and
// clear the flag before continuing.
// ------------------------------------------------------------

// Fixed screen headers, kept in flash (PROGMEM): read only via lcd_printf_P().
const char MSG_CYCLE_INTERROMPU[] PROGMEM = "CYCLE INTERROMPU";
const char MSG_CAUSE_PROBABLE[]   PROGMEM = "Cause probable:";
const char MSG_INTERROMPU_A[]     PROGMEM = "Interrompu %d/%d";

void resume_warning()
{
  digitalWrite(PIN_LED, HIGH);

  // Distinct alert pattern: 5 short high beeps, vs. 3 for a normal end
  // and 1 for an operator cancel.
  for( int i=0 ; i<5 ; i++ ) {
    tone(PIN_BUZZER, 2637, 150);
    delay(300);
  }

  state = STATE_RESUME_WARNING_UPDATE;
}

// Returns the number of real steps in a mode's step array (excludes the
// CONFIG_END sentinel). Used both to bound-check the interrupted-step
// index read from EEPROM (a fresh, never-written chip reads 0xFF there)
// and to display the "X/Y" total on the interrupted-cycle screen.
int count_steps(int mode_index)
{
  int i = 0;
  while( read_step(MODES[mode_index].steps, i).config != CONFIG_END ) i++;
  return i;
}

void resume_warning_update()
{
  int interrupted_mode = constrain(EEPROM.read(EEPROM_ADDRESS_MODE), 0, MODES_NUMBER - 1);
  int total_steps = count_steps(interrupted_mode);
  int interrupted_step = constrain(EEPROM.read(EEPROM_ADDRESS_CYCLE_STEP), 0, total_steps - 1);
  unsigned int interrupted_config = read_step(MODES[interrupted_mode].steps, interrupted_step).config;

  // Alternate every 2 seconds across three screens (mode/step, cause, and
  // the action that was actually interrupted), same blink idiom as
  // run_update(), since they don't all fit on one 16x2 screen at once.
  switch( (seconds() / 2) % 3 ) {
    case 1:
      lcd.setCursor(0, 0);
      lcd_printf_P(MSG_CYCLE_INTERROMPU);
      lcd.setCursor(0, 1);
      lcd_printf(MODES[interrupted_mode].name);
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd_printf_P(MSG_CAUSE_PROBABLE);
      lcd.setCursor(0, 1);
      lcd_printf_P(reset_cause_label());
      break;
    case 0:
      lcd.setCursor(0, 0);
      // 1-indexed for the operator (etape 1 of N, not 0 of N)
      lcd_printf_P(MSG_INTERROMPU_A, interrupted_step + 1, total_steps);
      lcd.setCursor(0, 1);
      lcd_printf(resolve_label(interrupted_config));
      break;
  }

  buttonAction.update();
  if( buttonAction.fell() ) {
    EEPROM.update(EEPROM_ADDRESS_CYCLE_FLAG, 0);
    digitalWrite(PIN_LED, LOW);
    state = STATE_SELECT;
  }
}

// ============================================================
// Arduino entry points
// ============================================================

void setup()
{
  // Pre-drive all relay pins HIGH before configuring them as outputs.
  // This prevents the brief LOW glitch that would otherwise energise
  // relays at power-up and avoids an inrush current spike.
  digitalWrite(PIN_VALVE_AIR, HIGH);
  digitalWrite(PIN_VALVE_CO2, HIGH);
  digitalWrite(PIN_VALVE_WATER, HIGH);
  digitalWrite(PIN_VALVE_CLEANER_IN, HIGH);
  digitalWrite(PIN_VALVE_SANITIZER_IN, HIGH);
  digitalWrite(PIN_VALVE_CLEANER_OUT, HIGH);
  digitalWrite(PIN_VALVE_SANITIZER_OUT, HIGH);
  digitalWrite(PIN_VALVE_DRAIN, HIGH);
  digitalWrite(PIN_PUMP, HIGH);

  pinMode(PIN_BUTTON_ACTION, INPUT_PULLUP);
  pinMode(PIN_VALVE_AIR, OUTPUT);
  pinMode(PIN_VALVE_CO2, OUTPUT);
  pinMode(PIN_VALVE_WATER, OUTPUT);
  pinMode(PIN_VALVE_CLEANER_IN, OUTPUT);
  pinMode(PIN_VALVE_SANITIZER_IN, OUTPUT);
  pinMode(PIN_VALVE_CLEANER_OUT, OUTPUT);
  pinMode(PIN_VALVE_SANITIZER_OUT, OUTPUT);
  pinMode(PIN_VALVE_DRAIN, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  delay(100);

  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  buttonAction.attach(PIN_BUTTON_ACTION);
  buttonAction.interval(10);  // Debounce interval in ms

  lcd.init();
  lcd.backlight();
  lcd.createChar(CHAR_UP_DOWN, CHAR_UP_DOWN_SETUP);

  // Restore the last selected mode from EEPROM, clamped to valid range
  mode = EEPROM.read(EEPROM_ADDRESS_MODE);
  mode = constrain(mode, 0, MODES_NUMBER - 1);

  // If the cycle-in-progress flag is still set, the previous run never
  // reached a clean terminate()/cancel() — show the warning instead of
  // silently starting on the normal selection screen.
  if( EEPROM.read(EEPROM_ADDRESS_CYCLE_FLAG) != 0 ) {
    state = STATE_RESUME_WARNING;
  }
}

void loop()
{
  switch(state) {
    case STATE_SELECT:
      select();
      break;
    case STATE_SELECT_UPDATE:
      select_update();
      break;
    case STATE_RUN:
      run();
      break;
    case STATE_RUN_UPDATE:
      run_update();
      break;
    case STATE_TERMINATE:
      terminate();
      break;
    case STATE_CANCEL:
      cancel();
      break;
    case STATE_RESUME_WARNING:
      resume_warning();
      break;
    case STATE_RESUME_WARNING_UPDATE:
      resume_warning_update();
      break;
  }
}
