// ============================================================
// Cycle de lavage — sélection, exécution, fin normale ou annulation
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
  // NOTE: no encoder re-anchoring here — select() is currently the only
  // screen polling menuselect, so the raw position can never drift out
  // of sync. If a second screen ever polls the encoder too (e.g. a
  // settings submenu), reintroduce a per-screen base offset (position -
  // current_index, applied on entry) to keep the two menus independent
  // — see the diagnostic sketch's menuselect_base pattern for a working
  // example of that.
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
