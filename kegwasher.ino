// ============================================================
// kegwasher.ino
// Refactored: interrupt-driven rotary encoder + button (PCINT),
// non-blocking actuator transitions, non-blocking buzzer.
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
#define PIN_MENUSELECT_1          A1  // Rotary encoder CLK
#define PIN_MENUSELECT_2          A2  // Rotary encoder DT
#define PIN_BUTTON_ACTION         A3  // Confirm / cancel button (SW)
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
#define CTRL_CLEANER_IN     0b000000010
#define CTRL_SANITIZER_IN   0b000000100
#define CTRL_AIR            0b000001000
#define CTRL_CO2            0b000010000
#define CTRL_DRAIN          0b000100000
#define CTRL_CLEANER_OUT    0b001000000
#define CTRL_SANITIZER_OUT  0b010000000
#define CTRL_PUMP           0b100000000

// Extended flags used by the test mode only
#define CONFIG_WARNING  0b1000000000
#define CONFIG_WAIT     0b10000000000

// ============================================================
// Composite valve configurations
// ============================================================
#define CONFIG_DRAIN            (CTRL_DRAIN)
#define CONFIG_DRAIN_SANITIZER  (CTRL_PUMP + CTRL_SANITIZER_IN + CTRL_DRAIN)
#define CONFIG_DRAIN_CLEANER    (CTRL_PUMP + CTRL_CLEANER_IN + CTRL_DRAIN)
#define CONFIG_FILL_SANITIZER   (CTRL_SANITIZER_IN + CTRL_WATER)
#define CONFIG_FILL_CLEANER     (CTRL_CLEANER_IN + CTRL_WATER)
#define CONFIG_RINCE            (CTRL_PUMP + CTRL_WATER + CTRL_DRAIN)
#define CONFIG_RINCE_PURGE      (CTRL_AIR + CTRL_DRAIN)
#define CONFIG_RINCE_PURGE_CO2  (CTRL_CO2 + CTRL_DRAIN)
#define CONFIG_CLEAN            (CTRL_PUMP + CTRL_CLEANER_IN + CTRL_CLEANER_OUT)
#define CONFIG_CLEAN_PURGE      (CTRL_AIR + CTRL_CLEANER_OUT)
#define CONFIG_SANITIZE         (CTRL_PUMP + CTRL_SANITIZER_IN + CTRL_SANITIZER_OUT)
#define CONFIG_SANITIZE_PURGE   (CTRL_AIR + CTRL_SANITIZER_OUT)
#define CONFIG_CO2              (CTRL_CO2)
#define CONFIG_END              0

#define LED_BLINK_PERIOD    2

#define VALVE_CLOSE HIGH
#define VALVE_OPEN  LOW

// Delay between individual actuator changes during a transition (ms)
#define ACTUATOR_STEP_DELAY_MS  200

// ============================================================
// Data types
// ============================================================
typedef struct step_s {
  unsigned int config;
  int duration;
} step_t;

typedef struct mode_s {
  const char *name;
  step_t *steps;
} mode_t;

// ============================================================
// Step arrays
// ============================================================
step_t STEPS_DRAIN_SANITIZER[] = {
  {CONFIG_DRAIN_SANITIZER, 200},
  {CONFIG_END, 0}
};

step_t STEPS_DRAIN_CLEANER[] = {
  {CONFIG_DRAIN_CLEANER, 200},
  {CONFIG_END, 0}
};

step_t STEPS_FILL_SANITIZER[] = {
  {CONFIG_FILL_SANITIZER, 120},
  {CONFIG_END, 0}
};

step_t STEPS_FILL_CLEANER[] = {
  {CONFIG_FILL_CLEANER, 120},
  {CONFIG_END, 0}
};

step_t STEPS_WASH_KEG[] = {
  {CONFIG_DRAIN, 10},
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
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 30},
  {CONFIG_SANITIZE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 30},
  {CONFIG_END, 0}
};

step_t STEPS_DETER_KEG[] = {
  {CONFIG_DRAIN, 10},
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

step_t STEPS_WASH_KEG_PRESSURIZE[] = {
  {CONFIG_DRAIN, 10},
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
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 30},
  {CONFIG_SANITIZE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE_CO2, 30},
  {CONFIG_CO2, 10},
  {CONFIG_END, 0}
};

step_t STEPS_SANITIZE_KEG_PRESSURIZE[] = {
  {CONFIG_DRAIN, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 30},
  {CONFIG_SANITIZE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE_CO2, 30},
  {CONFIG_CO2, 10},
  {CONFIG_END, 0}
};

step_t STEPS_KEG_PRESSURIZE[] = {
  {CONFIG_DRAIN, 10},
  {CONFIG_RINCE_PURGE_CO2, 30},
  {CONFIG_CO2, 10},
  {CONFIG_END, 0}
};

step_t STEPS_DRAIN_KEG[] = {
  {CONFIG_DRAIN, 10},
  {CONFIG_RINCE_PURGE, 60},
  {CONFIG_END, 0}
};

step_t STEPS_TEST_ACTUATORS[] = {
  {CONFIG_WARNING,     3},
  {CONFIG_WAIT,        1},
  {CTRL_SANITIZER_OUT, 1},
  {CONFIG_WAIT,        1},
  {CONFIG_DRAIN,       1},
  {CONFIG_WAIT,        1},
  {CTRL_CLEANER_OUT,   1},
  {CONFIG_WAIT,        1},
  {CTRL_AIR,           1},
  {CONFIG_WAIT,        1},
  {CONFIG_CO2,         1},
  {CONFIG_WAIT,        1},
  {CTRL_SANITIZER_IN,  1},
  {CONFIG_WAIT,        1},
  {CTRL_WATER,         1},
  {CONFIG_WAIT,        1},
  {CTRL_CLEANER_IN,    1},
  {CONFIG_WAIT,        1},
  {CTRL_PUMP,          1},
  {CONFIG_WAIT,        1},
  {CONFIG_END,         0}
};

// ============================================================
// Mode table
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

int MODES_NUMBER = sizeof(MODES) / sizeof(mode_t);

// ============================================================
// Hardware objects
// ============================================================

// Rotary encoder as pointer — required for ISR tick() call
RotaryEncoder *menuselect = nullptr;

// Button — ISR flag + millis() debounce (replaces Bounce2 for this use case)
volatile bool buttonPressedFlag = false;  // set by ISR on falling edge (A3 LOW)
uint32_t      buttonLastMs      = 0;      // timestamp of last validated press
#define BUTTON_DEBOUNCE_MS 20

LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDRESS, 16, 2);

#define CHAR_UP_DOWN  1
byte CHAR_UP_DOWN_SETUP[] = {
  B00100, B01010, B10001, B00000,
  B00000, B10001, B01010, B00100
};

#define EEPROM_ADDRESS_MODE  0

// ============================================================
// State machine — declared early so transition_t and beep_t
// can embed state_t without forward-declaration issues
// ============================================================
typedef enum state_e {
  STATE_SELECT,
  STATE_SELECT_UPDATE,
  STATE_RUN,
  STATE_ACTUATORS_TRANSITION, // Non-blocking actuator sequencing
  STATE_RUN_UPDATE,
  STATE_TERMINATE,
  STATE_TERMINATE_BEEP,       // Non-blocking buzzer after completion
  STATE_CANCEL,
  STATE_CANCEL_BEEP           // Non-blocking buzzer after cancel
} state_t;

state_t state = STATE_SELECT;

// ============================================================
// ISR — PCINT1_vect : Port C (A0–A5)
// Handles A1 (PCINT9), A2 (PCINT10), A3 (PCINT11) in one vector
// ============================================================
ISR(PCINT1_vect) {
  menuselect->tick();  // RotaryEncoder filters irrelevant pin changes internally
  if (!(PINC & (1 << PC3))) {  // A3=PC3, LOW = pressed
    buttonPressedFlag = true;
  }
}

// ============================================================
// Non-blocking actuator transition
// ============================================================

// Ordered lists of bits to process, closing then opening
// Closing order: pump → water → CO2 → air → cleaner_in → sanitizer_in → cleaner_out → sanitizer_out → drain
// Opening order: drain → cleaner_out → sanitizer_out → cleaner_in → sanitizer_in → pump → air → CO2 → water
static const unsigned int CLOSE_ORDER[] = {
  CTRL_PUMP, CTRL_WATER, CTRL_CO2, CTRL_AIR,
  CTRL_CLEANER_IN, CTRL_SANITIZER_IN,
  CTRL_CLEANER_OUT, CTRL_SANITIZER_OUT, CTRL_DRAIN
};
static const unsigned int OPEN_ORDER[] = {
  CTRL_DRAIN, CTRL_CLEANER_OUT, CTRL_SANITIZER_OUT,
  CTRL_CLEANER_IN, CTRL_SANITIZER_IN,
  CTRL_PUMP, CTRL_AIR, CTRL_CO2, CTRL_WATER
};
static const int ACTUATOR_COUNT = 9;

typedef struct transition_s {
  unsigned int to_close;    // bitmask of actuators to close
  unsigned int to_open;     // bitmask of actuators to open
  int          phase;       // 0 = closing phase, 1 = opening phase
  int          index;       // current position in CLOSE_ORDER / OPEN_ORDER
  uint32_t     last_ms;     // timestamp of last digitalWrite
  state_t      next_state;  // state to enter once transition is complete
} transition_t;

transition_t tr;

// Map a CTRL_ bit to its physical pin
int bit_to_pin(unsigned int bit) {
  switch(bit) {
    case CTRL_WATER:         return PIN_VALVE_WATER;
    case CTRL_CLEANER_IN:    return PIN_VALVE_CLEANER_IN;
    case CTRL_SANITIZER_IN:  return PIN_VALVE_SANITIZER_IN;
    case CTRL_AIR:           return PIN_VALVE_AIR;
    case CTRL_CO2:           return PIN_VALVE_CO2;
    case CTRL_DRAIN:         return PIN_VALVE_DRAIN;
    case CTRL_CLEANER_OUT:   return PIN_VALVE_CLEANER_OUT;
    case CTRL_SANITIZER_OUT: return PIN_VALVE_SANITIZER_OUT;
    case CTRL_PUMP:          return PIN_PUMP;
    default:                 return -1;
  }
}

// Schedule a non-blocking transition from previous_config to config,
// then go to next_state when done.
unsigned int previous_config = 0;

void controls_set(unsigned int config, state_t next_state) {
  tr.to_close   = previous_config & ~config;
  tr.to_open    = config & ~previous_config;
  tr.phase      = 0;
  tr.index      = 0;
  tr.last_ms    = millis();
  tr.next_state = next_state;
  previous_config = config;
  state = STATE_ACTUATORS_TRANSITION;
}

// Called every loop() iteration while STATE_ACTUATORS_TRANSITION is active.
// Advances one actuator per ACTUATOR_STEP_DELAY_MS, then moves to next_state.
void actuators_update() {
  if (millis() - tr.last_ms < ACTUATOR_STEP_DELAY_MS) return;
  tr.last_ms = millis();

  if (tr.phase == 0) {
    // Closing phase: scan CLOSE_ORDER
    while (tr.index < ACTUATOR_COUNT) {
      unsigned int bit = CLOSE_ORDER[tr.index++];
      if (tr.to_close & bit) {
        int pin = bit_to_pin(bit);
        if (pin >= 0) digitalWrite(pin, VALVE_CLOSE);
        return; // one actuator per call
      }
    }
    // All closes done — move to opening phase
    tr.phase = 1;
    tr.index = 0;
  }

  if (tr.phase == 1) {
    // Opening phase: scan OPEN_ORDER
    while (tr.index < ACTUATOR_COUNT) {
      unsigned int bit = OPEN_ORDER[tr.index++];
      if (tr.to_open & bit) {
        int pin = bit_to_pin(bit);
        if (pin >= 0) digitalWrite(pin, VALVE_OPEN);
        return; // one actuator per call
      }
    }
    // All opens done — transition complete
    state = tr.next_state;
  }
}

// ============================================================
// Non-blocking buzzer
// ============================================================
typedef struct beep_s {
  int      count;       // remaining beeps
  uint32_t last_ms;     // timestamp of last tone event
  bool     tone_on;     // true = tone running, false = silence gap
  state_t  next_state;  // state to enter when done
} beep_t;

beep_t beep;

#define BEEP_TONE_HZ      1760
#define BEEP_TONE_MS       800  // tone duration
#define BEEP_SILENCE_MS    200  // silence between beeps

void beep_start(int count, state_t next_state) {
  beep.count      = count;
  beep.last_ms    = millis();
  beep.tone_on    = false;   // start with silence → then first tone
  beep.next_state = next_state;
}

// Called every loop() iteration while STATE_TERMINATE_BEEP or STATE_CANCEL_BEEP
void beep_update() {
  uint32_t now = millis();

  if (!beep.tone_on) {
    // Silence gap elapsed → start next tone
    if (now - beep.last_ms >= BEEP_SILENCE_MS) {
      tone(PIN_BUZZER, BEEP_TONE_HZ, BEEP_TONE_MS);
      beep.tone_on = true;
      beep.last_ms = now;
    }
  } else {
    // Tone elapsed → end of one beep
    if (now - beep.last_ms >= BEEP_TONE_MS) {
      beep.count--;
      beep.tone_on = false;
      beep.last_ms = now;
      if (beep.count == 0) {
        state = beep.next_state;
      }
    }
  }
}

// ============================================================
// Helpers
// ============================================================
void lcd_printf(const char *fmt, ...) {
  char buf1[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf1, sizeof(buf1)-1, fmt, args);
  va_end(args);
  char buf2[17];
  snprintf(buf2, sizeof(buf2)-1, "%-16s", buf1);
  lcd.print(buf2);
}

int seconds() {
  return millis() / 1000;
}

const char* resolve_label(unsigned int config) {
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
    case CONFIG_WARNING:         return "Cuves vides ?";
    case CONFIG_WAIT:            return "Attente...";
    default:                     return "";
  }
}

// ============================================================
// Global runtime variables
// ============================================================
int mode = 0;
int mode_start_time;
int mode_full_time;
int current_step;
int step_start_time;

// ============================================================
// State handlers
// ============================================================

void select() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd_printf("Mode :");
  lcd.setCursor(0, 1);
  lcd_printf("%c %s", CHAR_UP_DOWN, MODES[mode].name);
  digitalWrite(PIN_LED, HIGH);
  state = STATE_SELECT_UPDATE;
}

void select_update(bool buttonPressed) {
  // Encoder — no tick() here, ISR handles it
  int pos = menuselect->getPosition();
  int new_mode = (pos + MODES_NUMBER) % MODES_NUMBER;

  if (new_mode != mode) {
    mode = new_mode;
    lcd.setCursor(0, 1);
    lcd_printf("%c %s", CHAR_UP_DOWN, MODES[mode].name);
  }

  if (buttonPressed) {
    state = STATE_RUN;
  }
}

unsigned int step_set(int index) {
  current_step     = index;
  step_start_time  = seconds();
  unsigned int cfg = MODES[mode].steps[current_step].config;
  // Transition to new config, return to RUN_UPDATE when done
  controls_set(cfg, STATE_RUN_UPDATE);
  return cfg;
}

void run() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd_printf(MODES[mode].name);
  lcd.setCursor(0, 1);
  lcd_printf("Preparation");

  int saved_mode = EEPROM.read(EEPROM_ADDRESS_MODE);
  if (mode != saved_mode) EEPROM.write(EEPROM_ADDRESS_MODE, mode);

  mode_start_time = seconds();
  mode_full_time  = 0;
  for (int i = 0; MODES[mode].steps[i].config != CONFIG_END; i++) {
    mode_full_time += MODES[mode].steps[i].duration;
  }

  step_set(0);
  // state is now STATE_ACTUATORS_TRANSITION → will return to STATE_RUN_UPDATE
}

void run_update(bool buttonPressed) {
  // Advance step if duration elapsed
  int step_running_time = seconds() - step_start_time;
  if (step_running_time >= MODES[mode].steps[current_step].duration) {
    unsigned int config = MODES[mode].steps[current_step + 1].config;
    if (config == CONFIG_END) {
      state = STATE_TERMINATE;
      return;
    }
    step_set(current_step + 1);
    // state → STATE_ACTUATORS_TRANSITION → STATE_RUN_UPDATE
    return;
  }

  // Progress display
  int mode_running_time = seconds() - mode_start_time;
  int rtime_mn = mode_running_time / 60;
  int rtime_s  = mode_running_time % 60;
  int ftime_mn = mode_full_time / 60;
  int ftime_s  = mode_full_time % 60;

  lcd.setCursor(0, 1);
  lcd_printf(" %dmn%02d / %dmn%02d", rtime_mn, rtime_s, ftime_mn, ftime_s);

  // Blink top line between step label and mode name
  if (mode_running_time % LED_BLINK_PERIOD < LED_BLINK_PERIOD / 2) {
    digitalWrite(PIN_LED, HIGH);
    lcd.setCursor(0, 0);
    lcd_printf(resolve_label(MODES[mode].steps[current_step].config));
  } else {
    digitalWrite(PIN_LED, LOW);
    lcd.setCursor(0, 0);
    lcd_printf(MODES[mode].name);
  }
}

void terminate() {
  // Close all actuators first (non-blocking), then beep
  controls_set(0, STATE_TERMINATE_BEEP);
  digitalWrite(PIN_LED, LOW);
  lcd.setCursor(0, 1);
  lcd_printf(" Termine");
  beep_start(3, STATE_SELECT);
  // actual beeping starts once STATE_ACTUATORS_TRANSITION → STATE_TERMINATE_BEEP
}

void cancel() {
  controls_set(0, STATE_CANCEL_BEEP);
  digitalWrite(PIN_LED, LOW);
  lcd.setCursor(0, 1);
  lcd_printf(" Annule");
  beep_start(1, STATE_SELECT);
}

// ============================================================
// Arduino entry points
// ============================================================

void setup() {
  // Pre-drive relay pins HIGH before OUTPUT mode to avoid power-on glitch
  digitalWrite(PIN_VALVE_AIR,          HIGH);
  digitalWrite(PIN_VALVE_CO2,          HIGH);
  digitalWrite(PIN_VALVE_WATER,        HIGH);
  digitalWrite(PIN_VALVE_CLEANER_IN,   HIGH);
  digitalWrite(PIN_VALVE_SANITIZER_IN, HIGH);
  digitalWrite(PIN_VALVE_CLEANER_OUT,  HIGH);
  digitalWrite(PIN_VALVE_SANITIZER_OUT,HIGH);
  digitalWrite(PIN_VALVE_DRAIN,        HIGH);
  digitalWrite(PIN_PUMP,               HIGH);

  pinMode(PIN_VALVE_AIR,          OUTPUT);
  pinMode(PIN_VALVE_CO2,          OUTPUT);
  pinMode(PIN_VALVE_WATER,        OUTPUT);
  pinMode(PIN_VALVE_CLEANER_IN,   OUTPUT);
  pinMode(PIN_VALVE_SANITIZER_IN, OUTPUT);
  pinMode(PIN_VALVE_CLEANER_OUT,  OUTPUT);
  pinMode(PIN_VALVE_SANITIZER_OUT,OUTPUT);
  pinMode(PIN_VALVE_DRAIN,        OUTPUT);
  pinMode(PIN_LED,                OUTPUT);
  pinMode(PIN_PUMP,               OUTPUT);
  pinMode(PIN_BUZZER,             OUTPUT);

  delay(100);
  digitalWrite(PIN_LED,    LOW);
  digitalWrite(PIN_BUZZER, LOW);

  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("=== kegwasher boot ==="));

  // Rotary encoder — pointer, required for ISR
  menuselect = new RotaryEncoder(PIN_MENUSELECT_1, PIN_MENUSELECT_2, RotaryEncoder::LatchMode::FOUR3);

  // Button pin — INPUT_PULLUP (ISR detects falling edge on PC3/A3)
  pinMode(PIN_BUTTON_ACTION, INPUT_PULLUP);

  Serial.print(F("[SETUP] PIN_BUTTON_ACTION initial state="));
  Serial.println(digitalRead(PIN_BUTTON_ACTION) == LOW ? F("LOW") : F("HIGH"));

  // Activate Pin Change Interrupts for Port C:
  // A1 = PCINT9, A2 = PCINT10, A3 = PCINT11
  PCICR  |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);

  Serial.print(F("[SETUP] PCICR=0b"));  Serial.println(PCICR,  BIN);
  Serial.print(F("[SETUP] PCMSK1=0b")); Serial.println(PCMSK1, BIN);

  lcd.init();
  lcd.backlight();
  lcd.createChar(CHAR_UP_DOWN, CHAR_UP_DOWN_SETUP);

  mode = EEPROM.read(EEPROM_ADDRESS_MODE);
  mode = constrain(mode, 0, MODES_NUMBER - 1);
}

void loop() {
  // Button — validate ISR flag with millis() debounce
  bool buttonPressed = false;
  if (buttonPressedFlag) {
    buttonPressedFlag = false;
    uint32_t now = millis();
    if (now - buttonLastMs >= BUTTON_DEBOUNCE_MS) {
      buttonLastMs  = now;
      buttonPressed = true;
      Serial.print(F("[BTN] press validated state="));
      Serial.println((int)state);
    } else {
      Serial.println(F("[BTN] press ignored (debounce)"));
    }
  }

  // Cancel is handled globally — valid once a sequence is actually running.
  // STATE_RUN is excluded: it's a one-shot init state, not an execution state.
  if (buttonPressed &&
      (state == STATE_RUN_UPDATE ||
       state == STATE_ACTUATORS_TRANSITION)) {
    Serial.println(F("[BTN] CANCEL"));
    state = STATE_CANCEL;
  }

  switch (state) {
    case STATE_SELECT:
      select();
      break;
    case STATE_SELECT_UPDATE:
      select_update(buttonPressed);
      break;
    case STATE_RUN:
      run();
      break;
    case STATE_ACTUATORS_TRANSITION:
      actuators_update();
      break;
    case STATE_RUN_UPDATE:
      run_update(buttonPressed);
      break;
    case STATE_TERMINATE:
      terminate();
      break;
    case STATE_TERMINATE_BEEP:
      beep_update();
      break;
    case STATE_CANCEL:
      cancel();
      break;
    case STATE_CANCEL_BEEP:
      beep_update();
      break;
  }
}
