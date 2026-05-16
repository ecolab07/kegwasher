#include <stdarg.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>

#define PIN_BUZZER                A0
#define PIN_MENUSELECT_1          A1
#define PIN_MENUSELECT_2          A2
#define PIN_BUTTON_ACTION         A3
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

#define CTRL_WATER          0b000000001
// Sortie de cuve = entrée dans le circuit
#define CTRL_CLEANER_IN     0b000000010
// Sortie de cuve = entrée dans le circuit
#define CTRL_SANITIZER_IN   0b000000100
#define CTRL_AIR            0b000001000
#define CTRL_CO2            0b000010000
#define CTRL_DRAIN          0b000100000
// entree haute de cuve = sortie du circuit
#define CTRL_CLEANER_OUT    0b001000000
// entree haute de cuve = sortie du circuit
#define CTRL_SANITIZER_OUT  0b010000000
#define CTRL_PUMP           0b100000000

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
#define VALVE_OPEN LOW

typedef struct step_s {
  unsigned int config;
  int duration;
} step_t;

typedef struct mode_s {
  const char *name;
  step_t *steps;
} mode_t;

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


// 579sec (9min39)
// https://www.btobeer.com/themes-conseils-techniques-bieres-brasseries/conseils-carbonatation-process-et-analyses/futs-de-biere-a-plongeurs-incorpores-problemes-lies-au-lavage-et-sterilisation-des-futs
step_t STEPS_WASH_KEG[] = {
  // 25sec
  {CONFIG_DRAIN, 25},
  // 103sec (1min43)
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  // 203sec (3min23)
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 3},
  {CONFIG_CLEAN_PURGE, 5},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 20},
  {CONFIG_CLEAN_PURGE, 25},
  {CONFIG_CLEAN, 20},
  {CONFIG_CLEAN_PURGE, 30},
  // 90sec (1min30)
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 7},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  // 113sec (1min53)
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 3},
  {CONFIG_SANITIZE_PURGE, 5},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 20},
  // 45sec
  {CONFIG_RINCE, 15},
  {CONFIG_RINCE_PURGE, 30},
  {CONFIG_END, 0}
};

step_t STEPS_DETER_KEG[] = {
  // 25sec
  {CONFIG_DRAIN, 25},
  // 103sec (1min43)
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  // 203sec (3min23)
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 3},
  {CONFIG_CLEAN_PURGE, 5},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 20},
  {CONFIG_CLEAN_PURGE, 25},
  {CONFIG_CLEAN, 20},
  {CONFIG_CLEAN_PURGE, 30},
  // 60sec (1min)
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 7},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_END, 0}
};

// 569sec (9min29)
step_t STEPS_WASH_KEG_PRESSURIZE[] = {
  // 25sec
  {CONFIG_DRAIN, 25},
  // 103sec (1min43)
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  // 203sec (3min23)
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 3},
  {CONFIG_CLEAN_PURGE, 5},
  {CONFIG_CLEAN, 10},
  {CONFIG_CLEAN_PURGE, 15},
  {CONFIG_CLEAN, 20},
  {CONFIG_CLEAN_PURGE, 25},
  {CONFIG_CLEAN, 20},
  {CONFIG_CLEAN_PURGE, 30},
  // 90sec (1min30)
  {CONFIG_RINCE, 3},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 7},
  {CONFIG_RINCE_PURGE, 10},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  // 113sec (1min53)
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 3},
  {CONFIG_SANITIZE_PURGE, 5},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 20},
  // 25sec
  {CONFIG_RINCE, 15},
  {CONFIG_RINCE_PURGE_CO2, 10},
  // 10sec
  {CONFIG_CO2, 10},
  {CONFIG_END, 0}
};

// 333sec (5min30)
step_t STEPS_SANITIZE_KEG_PRESSURIZE[] = {
  // 25sec
  {CONFIG_DRAIN, 10},
  // 30sec
  {CONFIG_RINCE, 10},
  {CONFIG_RINCE_PURGE, 20},
  // 203sec (3min23)
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 15},
  {CONFIG_SANITIZE, 3},
  {CONFIG_SANITIZE_PURGE, 5},
  {CONFIG_SANITIZE, 10},
  {CONFIG_SANITIZE_PURGE, 20},
  {CONFIG_SANITIZE, 15},
  {CONFIG_SANITIZE_PURGE, 25},
  {CONFIG_SANITIZE, 20},
  {CONFIG_SANITIZE_PURGE, 30},
  // 45sec
  {CONFIG_RINCE, 15},
  {CONFIG_RINCE_PURGE_CO2, 10},
  // 30sec
  {CONFIG_CO2, 10},
  {CONFIG_END, 0}
};

// 40sec
step_t STEPS_KEG_PRESSURIZE[] = {
  // 10sec
  {CONFIG_DRAIN, 10},
  // 10sec
  {CONFIG_RINCE_PURGE_CO2, 10},
  // 20sec
  {CONFIG_CO2, 20},
  {CONFIG_END, 0}
};

step_t STEPS_DRAIN_KEG[] = {
  {CONFIG_DRAIN, 10},
  {CONFIG_RINCE_PURGE, 60},
  {CONFIG_END, 0}
};

mode_t MODES[] = {
  {"Lavage + CO2", STEPS_WASH_KEG_PRESSURIZE},
  {"Lavage sans CO2", STEPS_WASH_KEG},
  {"Detergent seul", STEPS_DETER_KEG},
  {"CO2", STEPS_KEG_PRESSURIZE},
  {"Desinf. + CO2", STEPS_SANITIZE_KEG_PRESSURIZE},
  {"Vidange fut", STEPS_DRAIN_KEG},
  {"Vidange desinf.", STEPS_DRAIN_SANITIZER},
  {"Vidange deter.", STEPS_DRAIN_CLEANER},
  {"Rempl. desinf.", STEPS_FILL_SANITIZER},
  {"Rempl. deter.", STEPS_FILL_CLEANER},
};

int MODES_NUMBER = sizeof(MODES) / sizeof(mode_t);

RotaryEncoder menuselect(PIN_MENUSELECT_1, PIN_MENUSELECT_2);
Bounce buttonAction = Bounce();

LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDRESS, 16, 2);

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

#define EEPROM_ADDRESS_MODE  0

typedef enum state_e {
  STATE_SELECT,
  STATE_SELECT_UPDATE,
  STATE_RUN,
  STATE_RUN_UPDATE,
  STATE_TERMINATE,
  STATE_CANCEL
} state_t;


state_t state = STATE_SELECT;

int mode = 0;
int mode_start_time;
int mode_full_time;

int step;
int step_start_time;

const char *config_label;

void lcd_printf(const char *fmt, ...)
{
  char buf1[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf1, sizeof(buf1)-1, fmt, args);
  va_end(args);

  char buf2[17];
  snprintf(buf2, sizeof(buf2)-1, "%-16s", buf1);
  lcd.print(buf2);
}

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

void select_update()
{
  int new_mode;
  int pos;

  menuselect.tick();
  pos = menuselect.getPosition();
  new_mode = pos % MODES_NUMBER;

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

int seconds()
{
  return millis() / 1000;
}

void controls_set_state(unsigned int config, int state, int delay_time)
{
  switch(config) {
    case CONFIG_DRAIN:
      config_label = "Vidange";
      break;
    case CONFIG_DRAIN_SANITIZER:
      config_label = "Vidange desinf.";
      break;
    case CONFIG_DRAIN_CLEANER:
      config_label = "Vidange deter.";
      break;
    case CONFIG_FILL_SANITIZER:
      config_label = "Rempl. desinf.";
      break;
    case CONFIG_FILL_CLEANER:
      config_label = "Rempl. deter.";
      break;
    case CONFIG_RINCE:
      config_label = "Rincage";
      break;
    case CONFIG_RINCE_PURGE:
      config_label = "Purge air";
      break;
    case CONFIG_RINCE_PURGE_CO2:
      config_label = "Purge CO2";
      break;
    case CONFIG_CLEAN:
      config_label = "Detergent";
      break;
    case CONFIG_CLEAN_PURGE:
      config_label = "Purge deter.";
      break;
    case CONFIG_SANITIZE:
      config_label = "Desinfectant";
      break;
    case CONFIG_SANITIZE_PURGE:
      config_label = "Purge desinf.";
      break;
    case CONFIG_CO2:
      config_label = "CO2";
      break;
    default:
      config_label = "";
      break;
  }
  
  // Close and open are not done in the same order. 
  // Necessary if close delay time > 0 to avoid physical pressure damage and
  // chemical hazards
  if (state == VALVE_CLOSE) {  
    // First stop pump and all pressurized inputs
    if(config & CTRL_PUMP) {
      digitalWrite(PIN_PUMP, state);
      delay(delay_time);
    }
    if(config & CTRL_WATER) {
      digitalWrite(PIN_VALVE_WATER, state);
      delay(delay_time);
    }
    if(config & CTRL_CO2) {
      digitalWrite(PIN_VALVE_CO2, state);
      delay(delay_time);
    }
    if(config & CTRL_AIR) {
      digitalWrite(PIN_VALVE_AIR, state);
      delay(delay_time);
    }
    if(config & CTRL_CLEANER_IN) {
      digitalWrite(PIN_VALVE_CLEANER_IN, state);
      delay(delay_time);
    }
    if(config & CTRL_SANITIZER_IN) {
      digitalWrite(PIN_VALVE_SANITIZER_IN, state);
      delay(delay_time);
    }
    if(config & CTRL_CLEANER_OUT) {
      digitalWrite(PIN_VALVE_CLEANER_OUT, state);
      delay(delay_time);
    }
    if(config & CTRL_SANITIZER_OUT) {
      digitalWrite(PIN_VALVE_SANITIZER_OUT, state);
      delay(delay_time);
    }
    if(config & CTRL_DRAIN) {
      digitalWrite(PIN_VALVE_DRAIN, state);
      delay(delay_time);
    }
  }
  else {
    // first open outputs
    if(config & CTRL_DRAIN) {
      digitalWrite(PIN_VALVE_DRAIN, state);
      delay(delay_time);
    }
    if(config & CTRL_CLEANER_OUT) {
      digitalWrite(PIN_VALVE_CLEANER_OUT, state);
      delay(delay_time);
    }
    if(config & CTRL_SANITIZER_OUT) {
      digitalWrite(PIN_VALVE_SANITIZER_OUT, state);
      delay(delay_time);
    }

    // next unpressurized inputs    
    if(config & CTRL_CLEANER_IN) {
      digitalWrite(PIN_VALVE_CLEANER_IN, state);
      delay(delay_time);
    }
    if(config & CTRL_SANITIZER_IN) {
      digitalWrite(PIN_VALVE_SANITIZER_IN, state);
      delay(delay_time);
    }

    // then pump, effectively decreasing pressure in input channels
    if(config & CTRL_PUMP) {
      digitalWrite(PIN_PUMP, state);
      delay(delay_time);
    }

    // Air is neutral, can open it here
    if(config & CTRL_AIR) {
      digitalWrite(PIN_VALVE_AIR, state);
      delay(delay_time);
    }
    // next CO2
    if(config & CTRL_CO2) {
      digitalWrite(PIN_VALVE_CO2, state);
      delay(delay_time);
    }
    // and finally water. hopefully this will avoid pressure from pushing water into liquid tanks
    if(config & CTRL_WATER) {
      digitalWrite(PIN_VALVE_WATER, state);
      delay(delay_time);
    }
  }
}

void controls_set(unsigned int config)
{
  // Turn off all controls to avoid temporary overconsumption
  controls_set_state(~config, VALVE_CLOSE, 200);

  // Turn on all needed controls and wait slowly to avoid temporary overconsumption
  controls_set_state(config, VALVE_OPEN, 200);
}

unsigned int step_set(int index)
{
  step = index;
  step_start_time = seconds();

  controls_set(MODES[mode].steps[step].config);

  return MODES[mode].steps[step].config;
}

void run()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd_printf(MODES[mode].name);
  lcd.setCursor(0, 1);
  lcd_printf("Preparation");

  int saved_mode = EEPROM.read(EEPROM_ADDRESS_MODE);
  if( mode != saved_mode ) {
    EEPROM.write(EEPROM_ADDRESS_MODE, mode);
  }

  mode_start_time = seconds();

  mode_full_time = 0;
  for(int i=0 ; MODES[mode].steps[i].config != CONFIG_END ; i++ ) {
    mode_full_time += MODES[mode].steps[i].duration;
  }

  step_set(0);

  state = STATE_RUN_UPDATE;
}

void run_update()
{
  buttonAction.update();
  if( buttonAction.fell() ) {
    state = STATE_CANCEL;
    return;
  }

  int step_running_time = seconds() - step_start_time;
  if( step_running_time >= MODES[mode].steps[step].duration ) {
    unsigned int config = step_set( step + 1 );
    if( config == CONFIG_END ) {
      state = STATE_TERMINATE;
      return;
    }
  }

  int mode_running_time = seconds() - mode_start_time;
  int rtime_mn = mode_running_time / 60;
  int rtime_s = mode_running_time % 60;
  int ftime_mn = mode_full_time / 60;
  int ftime_s = mode_full_time % 60;

  lcd.setCursor(0, 1);
  lcd_printf(" %dmn%02d / %dmn%02d", rtime_mn, rtime_s, ftime_mn, ftime_s);

  if( mode_running_time % LED_BLINK_PERIOD < LED_BLINK_PERIOD/2 ) {
    digitalWrite(PIN_LED, HIGH);
    lcd.setCursor(0, 0);
    lcd_printf(config_label);
  }
  else {
    digitalWrite(PIN_LED, LOW);
    lcd.setCursor(0, 0);
    lcd_printf(MODES[mode].name);
  }

}

void terminate()
{
  controls_set(0);

  digitalWrite(PIN_LED, LOW);

  lcd.setCursor(0, 1);
  lcd_printf(" Termine");

  for( int i=0 ; i<3 ; i++ ) {
    tone(PIN_BUZZER, 1760, 800);
    delay(1000);
  }

  state = STATE_SELECT;
}

void cancel()
{
  controls_set(0);

  digitalWrite(PIN_LED, LOW);

  lcd.setCursor(0, 1);
  lcd_printf(" Annule");

  tone(PIN_BUZZER, 1760, 800);
  delay(1000);

  state = STATE_SELECT;
}


void setup()
{
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

  controls_set(0);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  buttonAction.attach(PIN_BUTTON_ACTION);
  buttonAction.interval(10);

  lcd.begin();
  lcd.backlight();
  lcd.createChar(CHAR_UP_DOWN, CHAR_UP_DOWN_SETUP);

  mode = EEPROM.read(EEPROM_ADDRESS_MODE);
  mode = constrain(mode, 0, MODES_NUMBER - 1);
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
  }
}
