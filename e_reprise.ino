// ============================================================
// Détection de cycle interrompu et écran d'alerte au démarrage
// ============================================================

// ------------------------------------------------------------
// Reset-cause capture
//
// Reads MCUSR (which AVR sets on every reset and does not clear itself)
// as early as possible — before Arduino's own init() runs — so we can
// tell a normal power-up apart from a brown-out (voltage dip), external,
// or watchdog reset. This is what lets resume_warning_update() report
// whether an interrupted cycle was actually caused by a power dip.
// ------------------------------------------------------------
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
