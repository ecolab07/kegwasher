// ============================================================
// Pilotage des actionneurs
// ============================================================

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
