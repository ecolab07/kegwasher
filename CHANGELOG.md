# Changelog

All notable changes to this project are documented in this file.

This project follows a fork lineage:
- **v1.x** — [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher) (original project)
- **v2.x** — [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher) (first fork)
- **v3.x** — [ecolab07/kegwasher](https://github.com/ecolab07/kegwasher) (this fork)

---

## [3.x] — ecolab07 fork

### Added
- **Actuator test mode** (`Test vannes` / `STEPS_TEST_ACTUATORS`): fires each of the 9 actuators individually for 1 second in relay-board order, with a safety prompt (`CONFIG_WARNING`) before the first pulse and a 1 s idle gap (`CONFIG_WAIT`) between each actuation. Implemented entirely as a `step_t` array — no changes to the control layer. Two pseudo-configuration flags introduced: `CONFIG_WARNING` (bit 10) and `CONFIG_WAIT` (bit 11), both outside the 9-bit actuator range so no physical output is triggered.
- **Pin pre-initialisation to HIGH**: all relay control pins are driven HIGH before `pinMode(OUTPUT)` is called in `setup()`, preventing the brief LOW glitch that would otherwise energise all relays at power-up and cause an inrush current spike.
- **`resolve_label()`**: pure function returning a `const char*` label for any valve configuration, replacing the former `config_label` global variable. Used by `run_update()` for the alternating LCD display.
- **`close_actuators()` / `open_actuators()`**: `controls_set_state()` split into two dedicated functions, one per direction, each with its own fixed actuator order. Removes the direction-switching `if/else` and makes the sequencing intent explicit.
- **Minimal transition logic in `controls_set()`**: transitions now only touch actuators that actually change state, using bitmask arithmetic:
  ```c
  unsigned int to_close = previous_config & ~config;
  unsigned int to_open  = config & ~previous_config;
  ```
  `previous_config` is tracked globally and reset to 0 at each mode start. This eliminates unnecessary relay switching, reduces inrush current, and makes `CONFIG_WARNING` / `CONFIG_WAIT` safe by design (their bits above bit 9 produce zero when AND-masked against the 9-bit actuator range).

### Changed
- **Rotary encoder negative value guard** — corrected formula:
  - v3.0 (initial): `(pos + MODES_NUMBER) % MODES_NUMBER` — silently fails when `pos` is more negative than `-MODES_NUMBER` (C modulo sign follows dividend)
  - v3.1 (current): `((pos % MODES_NUMBER) + MODES_NUMBER) % MODES_NUMBER` — always produces a result in `[0, MODES_NUMBER-1]` regardless of how far counter-clockwise the encoder has been turned
- **Type safety**: `step_t.duration`, all time variables (`mode_start_time`, `mode_full_time`, `step_start_time`) and `seconds()` return type changed from `int` to `unsigned long`, consistent with `millis()` and correct for AVR where `int` is 16 bits (max 32 767 s ≈ 9 h).
- **`MODES_NUMBER`**: changed from mutable `int` to `const int`.
- **`lcd_printf`**: removed spurious `sizeof-1` (vsnprintf already guarantees null termination within the provided buffer size).
- **`step_start_time` recorded after transitions**: in `step_set()`, the step chrono now starts after `controls_set()` returns, so each step's declared duration is measured from when the valves are actually in their target state.
- **`mode_start_time` recorded after `step_set(0)`**: same rationale — the mode chrono excludes the transition overhead of the first step.
- **`CONFIG_FILL_SANITIZER` / `CONFIG_FILL_CLEANER`**: pump (`CTRL_PUMP`) restored and inlet valves corrected. Gummy35 had removed the pump (filling by mains pressure only). In practice, water enters via the keg and must be pumped back into the tank through the return valves (`CTRL_SANITIZER_OUT` / `CTRL_CLEANER_OUT`) — the inlet valves (`*_IN`) are oriented in the wrong direction for this flow path.
- **Immersion heater downrated from 3500 W to 2500 W**: the 3500 W element was tripping the circuit breaker on shared circuits. The 2500 W element heats more slowly but stays within the breaker's sustained current rating.

### Known limitation — displayed time vs real time
The 200 ms inter-actuator delay in `close_actuators()` and `open_actuators()` is blocking: it consumes wall-clock time during which `loop()` does not run. As a result, displayed seconds are slightly longer than real seconds — each transition between steps adds a small overhead proportional to the number of actuators that change state. This is addressed in branch `feature/nonblocking-timers` (see below).

### In progress
- **`feature/nonblocking-timers`**: `step_start_time` and `mode_start_time` use `millis()` for ms-precision step expiry; a `transition_overhead` accumulator subtracts blocking transition time from the displayed elapsed time so the LCD always shows real hydraulic time.
- **`feature/rotary-interrupts`**: rotary encoder and button handling migrated to Pin Change Interrupts (PCINT), eliminating the need to call `menuselect.tick()` and `buttonAction.update()` in the main loop.
- Terminal screw shield on the Arduino with integrated prototyping area, used to mount the LED current-limiting resistor and provide robust screw-terminal connections in place of Dupont connectors.
- Emergency stop button now drives a **contactor** rather than being wired directly in the load circuit, improving safety and button longevity.
- SSR fitted with a **heatsink and 12 V cooling fan** to prevent thermal runaway during long detergent heating sessions.
- **Bus bar rails** on relay outputs to simplify solenoid valve wiring and improve enclosure readability.
- All components inside the enclosure identified with **adhesive labels**.

### Documentation
- Full bilingual documentation added (English + French): user manual, technical README, electronics assembly guide, maintenance guide, CHANGELOG, CONTRIBUTING.

---

## [2.x] — Gummy35 fork

### Added
- **Ordered actuator open/close sequencing** (`controls_set_state()` refactored from scratch): actuators are now opened and closed one at a time in a defined order with a configurable inter-actuator delay. Closing order: pump → water/CO₂/air → liquid inlets → return paths → drain. Opening order: drain/returns → liquid inlets → pump → air → CO₂ → water. Prevents pressure spikes, back-flow into gas lines, and relay inrush current spikes. Closing delay increased from 0 ms to 200 ms.
- **LCD alternating display**: `config_label` string resolved inside `controls_set_state()` via a `switch` on the active configuration; line 1 alternates between the current step label and the mode name in sync with the LED blink cadence.
- **New wash modes**:
  - `Detergent seul` (`STEPS_DETER_KEG`): detergent wash and rinses only, no sanitization.
  - `CO2` (`STEPS_KEG_PRESSURIZE`): CO₂ purge and pressurisation only.
  - `Desinf. + CO2` (`STEPS_SANITIZE_KEG_PRESSURIZE`): sanitization cycle followed by CO₂ pressurisation.
- **CO₂ pressurisation step** (`{CONFIG_CO2, 10}`) added to `STEPS_WASH_KEG_PRESSURIZE`.
- **`CONFIG_CO2` define** added (was missing from the original).
- **`VALVE_OPEN` / `VALVE_CLOSE` defines** replacing raw `HIGH`/`LOW` in control calls, making active-low relay logic explicit.
- **`CONFIG_FILL_SANITIZER/CLEANER`**: pump removed from the fill configurations (filling by mains pressure only, no pump).
- A fully commented-out `STEPS_WASH_KEG_PRESSURIZE_SAFE` variant that drains sanitizer to the waste outlet rather than returning it to the tank — left as a reference for future exploration.

### Changed — timing overhaul
Gummy35 significantly increased step durations across all wash cycles as a safety margin, with more intermediate passes at each phase. Key changes to `STEPS_WASH_KEG` and `STEPS_WASH_KEG_PRESSURIZE`:

| Phase | Vieuxsinge | Gummy35 |
|---|---|---|
| Initial drain | 10 s | 25 s |
| Initial rinse | 30 s (2 passes) | 103 s (4 passes) |
| Detergent | 75 s (3×10 s + purges) | 203 s (8 passes + purges) |
| Intermediate rinse | 60 s (3 passes) | 90 s (4 passes) |
| Sanitization | 65 s (3×10 s + purges) | 113 s (5 passes + purges) |
| Final purge | CO₂ 30 s | Air 30 s (no CO₂ in `STEPS_WASH_KEG`) |
| **Total** | **325 s (5 min 25)** | **579 s (9 min 39)** |

> ⚠️ **Note (ecolab07 fork)**: the timing increases introduced by Gummy35 were considered overly conservative for the reference installation. This fork restores the original Vieuxsinge timings for all pre-existing modes. The three new modes are kept with timings aligned to the Vieuxsinge baseline. If your installation requires longer cycles (lower water pressure, larger tank volume, different chemical concentrations), adjust step durations locally — see [CONTRIBUTING.md](CONTRIBUTING.md) §7.

---

## [1.x] — vieuxsinge original

### Foundation
- Arduino Uno-based control system for a 2-head keg washer.
- Step-based sequence engine: wash programs defined as `step_t` arrays terminated by `{CONFIG_END, 0}`.
- 9-bit actuator bitmask model (`CTRL_*` defines) with composite `CONFIG_*` configurations.
- Wash modes: `Lavage + CO2`, `Lavage sans CO2`, `Vidange fut`, `Vidange desinf.`, `Vidange deter.`, `Rempl. desinf.`, `Rempl. deter.`.
- Rotary encoder menu with EEPROM persistence of last selected mode.
- I²C LCD display, buzzer end-of-cycle notification, LED heartbeat.
- State machine: `STATE_SELECT` → `STATE_RUN` → `STATE_TERMINATE` / `STATE_CANCEL`.
