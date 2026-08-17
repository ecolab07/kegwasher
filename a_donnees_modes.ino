// ============================================================
// Données des modes de lavage
//
// Tableaux d'étapes marqués PROGMEM : ils restent en flash au lieu
// d'être recopiés en RAM au démarrage. On ne les lit jamais directement
// (steps[i].config planterait) — toujours via read_step(), qui copie
// une entrée en RAM avec memcpy_P().
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

// Computed at runtime so new modes added above are counted automatically.
// Declared `extern` in kegwasher.ino so setup() can clamp the restored
// mode index against it despite this tab being concatenated afterwards.
const int MODES_NUMBER = sizeof(MODES) / sizeof(mode_t);

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
