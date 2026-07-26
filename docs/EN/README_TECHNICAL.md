# kegwasher — Technical README

> This project is a fork of [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher)
> and [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher).
> License: GNU GPL v3.

This document covers the technical design decisions made across the three generations of the project. For usage instructions, see [USER_MANUAL.md](USER_MANUAL.md).

---

## Table of Contents

1. [Architecture overview](#1-architecture-overview)
2. [Step-based sequence engine](#2-step-based-sequence-engine)
3. [Actuator control model](#3-actuator-control-model)
4. [Ordered open/close sequencing — Gummy35 + Ecolab07](#4-ordered-openclose-sequencing--gummy35--ecolab07)
5. [Pin pre-initialisation to HIGH — Ecolab07](#5-pin-pre-initialisation-to-high--ecolab07)
6. [State machine](#6-state-machine)
7. [LCD display: alternating step and mode labels — Gummy35 + Ecolab07](#7-lcd-display-alternating-step-and-mode-labels--gummy35--ecolab07)
8. [Rotary encoder: negative value guard — Ecolab07](#8-rotary-encoder-negative-value-guard--ecolab07)
9. [EEPROM persistence of selected mode](#9-eeprom-persistence-of-selected-mode)
10. [Actuator test mode — Ecolab07](#10-actuator-test-mode--ecolab07)
11. [Wash mode catalogue](#11-wash-mode-catalogue)
12. [Hardware notes](#12-hardware-notes)
13. [Dependencies](#13-dependencies)
14. [Wokwi simulation](#14-wokwi-simulation)
15. [Wiring diagram (Fritzing)](#15-wiring-diagram-fritzing)
16. [Known limitations and future work](#16-known-limitations-and-future-work)

---

## 1. Architecture overview

The firmware is a single `.ino` file structured around three independent layers:

```
┌─────────────────────────────────┐
│  Mode table  (MODES[])          │  Named modes → step arrays
├─────────────────────────────────┤
│  Sequence engine                │  Steps → actuator configs + timing
├─────────────────────────────────┤
│  Actuator control layer         │  Bitmasks → individual GPIO writes
└─────────────────────────────────┘
```

Adding a new wash program requires only a new `step_t` array and one entry in `MODES[]`. No control logic needs to change.

---

## 2. Step-based sequence engine

Each wash program is expressed as an array of `step_t` structs:

```c
typedef struct step_s {
  unsigned int  config;   // bitmask of actuators to activate
  unsigned long duration; // how long to hold this configuration (seconds)
} step_t;
```

The array is terminated by the sentinel value `{CONFIG_END, 0}` (where `CONFIG_END == 0`), which is how `run_update()` detects end-of-sequence without needing a length parameter.

At runtime, `step_set(index)` applies the valve configuration via `controls_set()` (blocking), then records `step_start_time` after the transitions complete. `run_update()` is called on every `loop()` iteration and advances to the next step as soon as `seconds() - step_start_time >= step.duration`.

Recording `step_start_time` after the transitions ensures each step's declared duration is measured from when the valves are actually in their target state, not from before the blocking delay. The same principle applies to `mode_start_time`, which is recorded after `step_set(0)` in `run()`.

Using wall-clock seconds (via `millis() / 1000`) rather than a counter makes step timing independent of loop execution time. All time variables use `unsigned long` (32 bits on AVR), consistent with `millis()` and safe from the 16-bit overflow that would affect `int` after ~9 hours.

---

## 3. Actuator control model

Each of the 9 physical outputs is assigned a unique power-of-two bit:

```c
#define CTRL_WATER          0b000000001
#define CTRL_CLEANER_IN     0b000000010
#define CTRL_SANITIZER_IN   0b000000100
#define CTRL_AIR            0b000001000
#define CTRL_CO2            0b000010000
#define CTRL_DRAIN          0b000100000
#define CTRL_CLEANER_OUT    0b001000000
#define CTRL_SANITIZER_OUT  0b010000000
#define CTRL_PUMP           0b100000000
```

Composite configurations for the wash sequences are built by OR-ing individual bits:

```c
#define CONFIG_CLEAN  (CTRL_PUMP + CTRL_CLEANER_IN + CTRL_CLEANER_OUT)
```

This gives a compact, readable, and easily extensible representation. Adding a 10th actuator only requires a new bit and a new pin — no structural changes.

The test mode deliberately uses raw `CTRL_*` bits (one actuator at a time) rather than composite `CONFIG_*` values, which is what makes it useful for diagnosing individual valve behaviour.

---

## 4. Ordered open/close sequencing — *Gummy35* + *Ecolab07*

The original firmware switched all actuators simultaneously with no delay between them, using raw `HIGH`/`LOW` values. Gummy35 refactored `controls_set_state()` entirely, introducing three changes.

**Named logic constants** replacing raw `HIGH`/`LOW`:
```c
#define VALVE_CLOSE HIGH
#define VALVE_OPEN  LOW
```
This makes the active-low relay logic explicit at every call site.

**Per-direction ordering** — actuators are now opened and closed in a carefully chosen order:

Closing order (pressure sources first, then outputs): pump → water/CO₂/air → liquid inlets → return paths → drain (last, so residual pressure can escape).

Opening order (clear path before pressure): drain/returns → liquid inlets → pump (draws liquid, reducing inlet pressure) → air → CO₂ → water (last — pump backpressure prevents back-flow into tanks).

This ordering prevents pressure spikes trapping liquid in lines when closing, water or chemicals back-flowing into CO₂ or air lines when opening, and pump cavitation.

**Inter-actuator delay on closing** increased from 0 ms to 200 ms, matching the opening delay already present in the original:

```c
// before (Vieuxsinge)
controls_set_state(~config, HIGH, 0);   // close: no delay
controls_set_state(config,  LOW,  200); // open:  200 ms

// after (Gummy35)
controls_set_state(~config, VALVE_CLOSE, 200); // close: 200 ms
controls_set_state(config,  VALVE_OPEN,  200); // open:  200 ms
```

**Also changed by Gummy35**: `CONFIG_FILL_SANITIZER` and `CONFIG_FILL_CLEANER` had `CTRL_PUMP` removed (filling by mains pressure only). This was reverted in the ecolab07 fork — the pump is needed to circulate water through the keg and back into the tank via the return valves (`CTRL_SANITIZER_OUT` / `CTRL_CLEANER_OUT`). The inlet valves (`CTRL_SANITIZER_IN` / `CTRL_CLEANER_IN`) are oriented in the wrong direction for this flow path and are not used during filling.

**Ecolab07 — minimal transition logic**: `controls_set_state()` was split into `close_actuators()` and `open_actuators()`, and `controls_set()` was rewritten to only touch actuators that actually change state:

```c
unsigned int previous_config = 0;  // tracks the active configuration

void controls_set(unsigned int config)
{
  unsigned int to_close = previous_config & ~config;
  unsigned int to_open  = config & ~previous_config;

  close_actuators(to_close);
  open_actuators(to_open);

  previous_config = config;
}
```

`previous_config & ~config` isolates actuators that were open and are no longer needed. `config & ~previous_config` isolates actuators that are needed and were not already open. Actuators that appear in both the old and new configuration are not touched. This has three benefits:

- **Inrush current**: only the relays that actually change state switch, never all 9 simultaneously.
- **Timing accuracy**: the 200 ms delay only applies to actuators that move, so transitions between similar configurations (e.g. `CONFIG_CLEAN` → `CONFIG_CLEAN_PURGE`, which share `CTRL_CLEANER_OUT`) are faster.
- **`CONFIG_WARNING` / `CONFIG_WAIT` safety by design**: these pseudo-configs have bits 10 and 11 set, outside the 9-bit actuator range. `to_close` and `to_open` mask these bits out naturally — no actuator is ever triggered, regardless of what `previous_config` held.

---

## 5. Pin pre-initialisation to HIGH — *Ecolab07*

The relay board uses **active-low logic**: a LOW signal on a control pin energises the relay and opens the valve. The Arduino's GPIO pins default to a high-impedance input state at power-up, which the relay board pulls LOW — briefly energising all relays before `pinMode(OUTPUT)` is called in `setup()`.

This causes an uncontrolled inrush current spike (all 9 relays switching simultaneously) and, more critically, a momentary opening of all valves.

The fix is to write `HIGH` to each pin *before* calling `pinMode()`:

```c
// Pre-drive HIGH before configuring as output to prevent
// the brief LOW glitch that would otherwise energise all relays at power-up
digitalWrite(PIN_VALVE_AIR, HIGH);
// ... all other valve and pump pins ...
pinMode(PIN_VALVE_AIR, OUTPUT);
// ...
```

On AVR-based Arduinos, `digitalWrite()` on a pin that is still in input mode writes to the pull-up register, which does not produce a LOW glitch. The transition to `OUTPUT` mode therefore starts from a committed HIGH level, keeping all relays de-energised.

---

## 6. State machine

The main loop dispatches on a `state_t` enum:

```
STATE_SELECT → STATE_SELECT_UPDATE ──────────────────────┐
                     │ (button press)                     │
                     ▼                                    │
               STATE_RUN                                  │
                     │                                    │
                     ▼                                    │
             STATE_RUN_UPDATE ──── (button press) ──► STATE_CANCEL ─┐
                     │ (CONFIG_END)                                  │
                     ▼                                               │
             STATE_TERMINATE ──────────────────────────────────────►┘
                                                         (all → STATE_SELECT)
```

Each state has a dedicated handler function. Transitions are written as assignments to the global `state` variable, which the next `loop()` iteration will dispatch on. This keeps each handler small and single-purpose.

---

## 7. LCD display: alternating step and mode labels — *Gummy35* + *Ecolab07*

During a running cycle, line 1 of the LCD alternates between two pieces of information on a `LED_BLINK_PERIOD` (2 s) cadence:

- **First half of period**: the current step label — e.g. `Detergent`, `Purge air`
- **Second half of period**: the mode name — e.g. `Lavage + CO2`

The LED blinks in sync with this alternation, providing a visible heartbeat that also confirms the Arduino is still running.

```c
if( mode_running_time % LED_BLINK_PERIOD < LED_BLINK_PERIOD/2 ) {
    digitalWrite(PIN_LED, HIGH);
    lcd.setCursor(0, 0);
    lcd_printf(resolve_label(MODES[mode].steps[step].config));
}
else {
    digitalWrite(PIN_LED, LOW);
    lcd.setCursor(0, 0);
    lcd_printf(MODES[mode].name);
}
```

**Gummy35** introduced this alternating display and resolved labels via a `config_label` global variable written inside `controls_set_state()`. **Ecolab07** replaced this with `resolve_label()`, a pure function that returns a `const char*` to a Flash literal with no side effects. The global variable is eliminated — the label is resolved on demand directly from the current step's config.

---

## 8. Rotary encoder: negative value guard — *Ecolab07*

The `RotaryEncoder` library returns a signed integer position that decrements below zero when the encoder is turned counter-clockwise past position 0. A naive modulo operation in C produces negative results for negative operands (the sign follows the dividend), which would result in an invalid array index into `MODES[]`.

**First approach** — adding `MODES_NUMBER` before the modulo:
```c
new_mode = (pos + MODES_NUMBER) % MODES_NUMBER;
```
This works correctly for one full counter-clockwise wrap (e.g. `pos = -3` with `MODES_NUMBER = 11` gives `8` ✅), but silently fails when `pos` is more negative than `-MODES_NUMBER` (e.g. `pos = -14` gives `-3` ❌, because `(-14 + 11) % 11 = -3` in C).

**Current approach** — double modulo:
```c
new_mode = ((pos % MODES_NUMBER) + MODES_NUMBER) % MODES_NUMBER;
```
`pos % MODES_NUMBER` first reduces any value into `(-MODES_NUMBER, MODES_NUMBER)`, then adding `MODES_NUMBER` makes it positive, and the final `% MODES_NUMBER` brings it back into `[0, MODES_NUMBER - 1]`. This is correct for any value of `pos`, regardless of how far counter-clockwise the encoder has been turned.

---

## 9. EEPROM persistence of selected mode

The index of the last selected mode is written to EEPROM address `0` when a cycle is launched (only if the value has changed, to limit write wear):

```c
int saved_mode = EEPROM.read(EEPROM_ADDRESS_MODE);
if( mode != saved_mode ) {
    EEPROM.write(EEPROM_ADDRESS_MODE, mode);
}
```

On power-up, the stored value is read and clamped to the valid range:

```c
mode = constrain(mode, 0, MODES_NUMBER - 1);
```

The clamp handles the case where the EEPROM contains `0xFF` (erased state) or a stale index from a firmware version with fewer modes.

---

## 10. Actuator test mode — *Ecolab07*

The test mode fires each of the 9 actuators individually for 1 second, in the physical order of the relay board, with a 1-second idle gap between each pulse.

**Design intent**: the test mode is implemented entirely as a `step_t` array (`STEPS_TEST_ACTUATORS`), using the same sequence engine as all other modes. No special-case code was added to the control layer.

Each step uses a raw `CTRL_*` bit rather than a composite `CONFIG_*` value, so exactly one relay clicks per step. This makes it straightforward to verify each valve by ear or by watching the circuit.

Two special pseudo-configurations are introduced exclusively for this mode:

| Constant | Bit | Role |
|---|---|---|
| `CONFIG_WARNING` | bit 10 | Displays a safety prompt (`Cuves vides ?`) before the first relay fires. No actuator is activated. |
| `CONFIG_WAIT`    | bit 11 | Idle step between pulses: `controls_set(CONFIG_WAIT)` closes everything (bits 0–9 are all zero in the mask), producing a clean off state between each individual actuation. |

Both bits are outside the 9-bit range of real actuator bits, so they cannot accidentally overlap with any physical output.

---

## 11. Wash mode catalogue

| Mode | Array | Duration | Gummy35 | Ecolab07 |
|---|---|---|---|---|
| `Lavage + CO2` | `STEPS_WASH_KEG_PRESSURIZE` | 335 s | ✅ CO₂ step | |
| `Lavage sans CO2` | `STEPS_WASH_KEG` | 325 s | | |
| `Detergent seul` | `STEPS_DETER_KEG` | 185 s | ✅ added | |
| `CO2` | `STEPS_KEG_PRESSURIZE` | 40 s | ✅ added | |
| `Desinf. + CO2` | `STEPS_SANITIZE_KEG_PRESSURIZE` | 190 s | ✅ added | |
| `Vidange fut` | `STEPS_DRAIN_KEG` | 70 s | | |
| `Vidange desinf.` | `STEPS_DRAIN_SANITIZER` | 200 s | | |
| `Vidange deter.` | `STEPS_DRAIN_CLEANER` | 200 s | | |
| `Rempl. desinf.` | `STEPS_FILL_SANITIZER` | 120 s | | |
| `Rempl. deter.` | `STEPS_FILL_CLEANER` | 120 s | | |
| `Test vannes` | `STEPS_TEST_ACTUATORS` | 22 s | | ✅ added |

---

## 12. Hardware notes

### Relay board — active-low logic
The 16-channel relay board uses active-low inputs. All control pins are pre-driven HIGH at boot (see §5) to prevent unintended actuation.

### Flyback diode protection
Solenoid valves are inductive loads. Without protection, the back-EMF spike when a valve de-energises can destroy relay contacts. A 1N4148 flyback diode is placed across each valve terminal, as close to the relay output as possible.

### SSR for the detergent heater
The immersion heater in the detergent tank is switched by a 40 A solid-state relay (SSR), controlled by the STC1000 thermostat. The original 3500 W element was downrated to **2500 W** to avoid tripping the circuit breaker on shared circuits — heating is slower but stays within the breaker's sustained current rating. The SSR is fitted with a **heatsink and a 12 V cooling fan** (Ecolab07 improvement) to prevent thermal runaway during long heating sessions.

### Emergency stop via contactor
The original design connected the emergency stop button directly in the control circuit. Ecolab07 replaced this with a **contactor** driven by the mushroom-head button, so the button switches only a low-current coil rather than the full load current of the machine. This improves button longevity and is safer in a wet environment.

### Arduino terminal screw shield
Ecolab07 added a **terminal screw shield** on top of the Arduino, fitted with an integrated prototyping area. This allows:
- the LED current-limiting resistor to be mounted cleanly on the board
- all external cables to be terminated on screw terminals rather than Dupont connectors, improving mechanical reliability in an environment subject to vibration and moisture.

### Relay output bus bars
Ecolab07 added **bus bar rails** on the relay output side to simplify solenoid valve wiring. Each bus bar groups the common 12 V return wire for a set of valves, reducing point-to-point wiring complexity and making the enclosure easier to read.

### Component labelling
All components inside the enclosure are identified by adhesive labels. This makes fault-finding and maintenance accessible without referring to a wiring diagram.

---

## 13. Dependencies

| Library | Version tested | Purpose |
|---|---|---|
| `Bounce2` | any | Button debouncing |
| `LiquidCrystal_I2C` | 1.1.2 | I²C LCD driver |
| `RotaryEncoder` | any | Rotary encoder reading |

**`LiquidCrystal_I2C`** — authored by Frank de Brabander, maintained by Marco Schwartz, hosted at [github.com/johnrickman/LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C). Install via the Arduino IDE Library Manager by searching for `LiquidCrystal I2C` — this installs version 1.1.2, which is the version used in this project and works without modification. There are several other forks of this library under similar names; using the wrong one may cause compilation errors or incorrect display behaviour.

---

## 14. Wokwi simulation

A complete circuit simulation is available at:
**[https://wokwi.com/projects/464738080652364801](https://wokwi.com/projects/464738080652364801)**

The simulation includes the Arduino Uno, I²C LCD display, relay board (active-low logic), status LED, buzzer, rotary encoder, and action button. Solenoid valves are represented by LEDs labelled with the corresponding valve name, allowing visual verification of each actuation sequence.

The full sketch runs without modification in the simulator.

**Known Wokwi limitations vs real hardware:**
- Displayed seconds run slower than real time — Wokwi does not simulate wall-clock time at 1:1 speed, so cycle durations appear longer than declared.
- The relay board's active-low logic is simulated with inverted polarity — verify the `VALVE_OPEN`/`VALVE_CLOSE` defines match your physical relay board before flashing to real hardware.

---

## 15. Wiring diagram (Fritzing)

A detailed Fritzing wiring diagram is provided in the repository (`fritzing/kegwasher.fzz` and `fritzing/kegwasher.png`). The diagram reflects the actual physical layout of components inside the enclosure and highlights four distinct zones:

- **Deported components**: LCD display and action button wired to the front panel
- **Safety block**: emergency stop mushroom-head button and contactor
- **Power outlets**: connection points for the immersion heater and circulation pump
- **Hydraulic zone**: solenoid valve wiring and relay output bus bars

The `.fzz` source file is included for anyone who wants to modify or extend the diagram.

---

## 16. Known limitations and future work

### Displayed time vs real time
The 200 ms inter-actuator delay in `close_actuators()` and `open_actuators()` is a blocking `delay()` call. During transitions, `loop()` does not run, so displayed seconds accumulate slightly slower than real seconds. The overhead per transition is proportional to the number of actuators that change state (0–9 relays × 200 ms). For the longest modes this adds approximately 15 s of drift over a 5 min 35 s cycle.

Branch `feature/nonblocking-timers` addresses this by measuring transition duration with `millis()` and subtracting it from the displayed elapsed time.

### Rotary encoder and button polling
The encoder and button are polled in the main loop (`menuselect.tick()`, `buttonAction.update()`). During blocking transitions, button presses may be missed if they occur within a 200 ms window.

Branch `feature/rotary-interrupts` addresses this by migrating encoder and button handling to Pin Change Interrupts (PCINT), making input detection independent of loop timing.
