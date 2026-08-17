// ============================================================
// kegwasher — sketch de production, réparti en plusieurs onglets .ino
//
// Cet onglet (qui porte le nom du dossier) est TOUJOURS concaténé en
// premier par l'IDE Arduino, avant tous les autres. C'est pour ça qu'il
// porte tout ce qui doit être visible ailleurs avant d'être utilisé :
// les #define, les typedef, les objets matériels et les variables d'état
// globales. Les autres onglets ne contiennent que des fonctions (l'IDE
// génère leurs prototypes automatiquement, donc leur ordre entre eux n'a
// pas d'importance) — mais ils ne doivent JAMAIS définir une variable
// globale ou un type dont cet onglet-ci (ou un onglet concaténé avant eux)
// aurait besoin.
//
// Ordre de concaténation réel :
//   kegwasher.ino (ce fichier)
//   a_donnees_modes.ino   — tableaux STEPS_*[], MODES[], read_step()
//   b_affichage.ino       — lcd_printf(), resolve_label()
//   c_actionneurs.ino     — pilotage des vannes, step_set()
//   d_cycle.ino           — select/run/terminate/cancel (le cœur du cycle)
//   e_reprise.ino         — détection de coupure, écran d'alerte au boot
// ============================================================

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

// Custom LCD character index: up/down arrows used in the selection screen
#define CHAR_UP_DOWN  1

// ============================================================
// EEPROM addresses
// ============================================================
#define EEPROM_ADDRESS_MODE        0
// 1 while a step-array mode is running, written back to 0 only by a clean
// terminate() or cancel(). If this is still 1 at boot, the Arduino restarted
// mid-cycle (most likely a relay-module brown-out) instead of finishing
// normally — see e_reprise.ino.
#define EEPROM_ADDRESS_CYCLE_FLAG  1
// Index of the last step reached before the interruption (diagnostic only)
#define EEPROM_ADDRESS_CYCLE_STEP  2

// ============================================================
// Data types
// ============================================================

// One step in a wash sequence: a valve configuration and its duration in seconds
typedef struct step_s {
  unsigned int  config;
  unsigned long duration;
} step_t;

// A named wash mode and its associated step array. `steps` points into
// flash (PROGMEM) — always read it via read_step() (a_donnees_modes.ino),
// never dereference it directly.
typedef struct mode_s {
  const char *name;
  const step_t *steps;
} mode_t;

// Explicit forward declaration. Arduino's automatic prototype generator
// has documented bugs when a function's signature uses a custom struct
// typedef and the function itself is defined in a secondary .ino tab —
// see arduino/arduino-cli issues #2696 and #2946. read_step() (defined
// in a_donnees_modes.ino) is the only function in this sketch affected,
// since it's the only one taking/returning step_t. Declaring it here,
// right after step_t itself, sidesteps the bug entirely.
step_t read_step(const step_t *steps, int index);

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

// ============================================================
// MODES[] is defined in a_donnees_modes.ino (a later tab). This forward
// declaration is what lets setup() below reference MODES_NUMBER despite
// that — see the header comment for why this is the one place in the
// whole sketch that needs it.
// ============================================================
extern const int MODES_NUMBER;

// ============================================================
// Hardware objects
// ============================================================
RotaryEncoder menuselect(PIN_MENUSELECT_1, PIN_MENUSELECT_2);
Bounce buttonAction = Bounce();
LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDRESS, 16, 2);

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

// ============================================================
// Global runtime variables
// ============================================================
state_t state = STATE_SELECT;

int           mode = 0;          // Index of the currently selected mode
unsigned long mode_start_time;   // Timestamp (s) when the mode started
unsigned long mode_full_time;    // Total duration of the current mode in seconds

int           step;              // Index of the current step within the mode
unsigned long step_start_time;   // Timestamp (s) when the current step started

// Tracks the previously active valve configuration, so controls_set()
// (c_actionneurs.ino) can compute minimal open/close transitions.
unsigned int previous_config = 0;

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
