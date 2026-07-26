# Electronics Assembly Guide — Keg Washer

> This project is a fork of [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher)
> and [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher).
> License: GNU GPL v3.

This guide walks through the assembly of the electronic control system, sub-system by sub-system, in the recommended build order. Component references and sourcing are covered in the separate [BOM](BOM.md).

> 📷 *Photo placeholders are marked throughout. Add your own images at the indicated locations.*

---

## Prerequisites

> 💡 **Before building**, you can test the complete circuit in the Wokwi simulator: [https://wokwi.com/projects/464738080652364801](https://wokwi.com/projects/464738080652364801). The simulation includes the Arduino, LCD, relay board, encoder, button, buzzer, and LEDs representing each solenoid valve. It is a useful way to familiarise yourself with the firmware behaviour before committing to physical wiring. Note that Wokwi simulates time slower than real speed — do not rely on it for timing validation.

A detailed Fritzing wiring diagram is provided in the repository (`fritzing/kegwasher.fzz` and `fritzing/kegwasher.png`). It reflects the actual physical layout of components inside the enclosure and highlights four zones: deported components (LCD, action button), safety block (emergency stop + contactor), power outlets (heater + pump), and hydraulic zone (solenoid valves + relay bus bars). The `.fzz` source file is included for anyone who wants to modify or extend the diagram.

### Skills
- **Beginner-friendly sections** are marked 🟢. They require no prior electronics knowledge — just the ability to follow instructions carefully.
- **Intermediate sections** are marked 🟡. Basic familiarity with multimeters, wire stripping, and reading a simple wiring diagram is helpful.

### Tools required
- Wire strippers
- Flat-head and Phillips screwdrivers
- Multimeter (voltage and continuity)
- Crimping tool (for ferrule end sleeves, recommended)
- Label maker or adhesive labels + marker

### Safety rules — read before you start
> ⚠️ **The enclosure contains 230 V AC mains voltage. Never work on the enclosure with the power cable connected.**

- Always disconnect the machine from the mains before opening the enclosure or touching any wiring.
- The 230 V section (transformer primary, SSR input, emergency stop contactor) must be wired with appropriately rated cable (2.5 mm² minimum) and kept physically separated from the 12 V / 5 V section.
- Use ferrule end sleeves on all stranded wire ends inserted into screw terminals. Loose strands can cause short circuits.
- When in doubt, measure before connecting. A multimeter in continuity mode costs nothing and prevents a lot of damage.

---

## Overview of the electrical architecture

```
230 V AC mains
     │
     ├──► Emergency stop contactor (mushroom-head button)
     │         │
     │         ▼
     │    230 V AC bus
     │         │
     │         ├──► 12 V DC power supply (120 W)
     │         │         │
     │         │         ├──► 12 V bus ──► Relay board (coils)
     │         │         │                 Solenoid valves
     │         │         │                 Cooling fan (SSR)
     │         │         │
     │         │         └──► 5 V (from relay board) ──► Arduino Uno
     │         │
     │         └──► SSR (40 A) ──► Immersion heater (3500 W)
     │                   ▲
     │              STC1000 thermostat (control signal)
     │
     └──► STC1000 thermostat (230 V supply)
```

The control electronics (Arduino, relay board, LCD, encoder, buttons) all operate at **12 V / 5 V DC** and are fully isolated from the 230 V section during normal operation. The only 230 V components that require wiring are the transformer primary, the SSR, the contactor, and the thermostat.

---

## Sub-system 1 — Enclosure preparation 🟢

**What it is**: the plastic enclosure that houses all electronics, mounted on the side of the frame.

**Steps**:
1. Plan the internal layout before drilling. Suggested arrangement (top to bottom, front to back):
   - Front panel: LCD, rotary encoder, action button (LED), emergency stop button
   - Upper interior: Arduino + terminal screw shield, relay board
   - Lower interior: 12 V power supply, SSR + heatsink + fan
   - Cable entry points: bottom of enclosure (keeps water ingress away from components)

2. Drill front panel cutouts for:
   - LCD window (rectangular)
   - Rotary encoder shaft (round, + flat for the knob grub screw)
   - Action button (round)
   - Emergency stop button (round, 22 mm standard)

3. Drill cable entry holes at the bottom of the enclosure. Use cable glands to provide strain relief and maintain IP rating.

4. Mount DIN rails or stand-offs for component fixings if used.

> 📷 *[Photo: front panel cutouts before component installation]*
> 📷 *[Photo: internal layout overview with components dry-fitted]*

---

## Sub-system 2 — 12 V power supply 🟡

**What it is**: a 230 VAC → 12 VDC switch-mode power supply (120 W rated). It powers the relay board coils, the solenoid valves, and the SSR cooling fan. The 5 V supply for the Arduino is derived from the relay board's onboard regulator.

**Steps**:
1. Mount the power supply in the lower section of the enclosure.
2. Connect the 230 V AC input terminals (L, N, PE) using 1.5 mm² cable from the contactor output bus. **Do not connect to mains yet.**
3. Leave the 12 V DC output terminals (V+, V−) accessible for the next sub-system.
4. ✅ **Check**: with a multimeter set to DC voltage, verify the output is 12 V ± 0.5 V before connecting any load.

> 📷 *[Photo: power supply mounted and wired]*

---

## Sub-system 3 — Emergency stop contactor 🟡

**What it is**: a contactor (electromechanical switch) whose coil is driven by the mushroom-head emergency stop button. Pressing the button de-energises the coil, opening the main power contacts and cutting all 230 V to the machine.

Using a contactor rather than wiring the button directly in the 230 V line means the button only switches a low-current coil (~VA), not the full machine load. This improves safety and button longevity in a damp environment.

**Steps**:
1. Mount the contactor inside the enclosure.
2. Connect the 230 V AC mains input to the contactor's line input terminals.
3. Connect the contactor's load output terminals to the 230 V AC bus that feeds the power supply and the SSR.
4. Wire the contactor coil terminals to the emergency stop button on the front panel. When the button is released (normal state), the coil is energised and the contacts are closed. Pressing the button breaks the coil circuit, dropping the contacts open.
5. Wire the emergency stop button to a 230 V source that is **upstream** of the contactor (the button must remain powered even when the contactor is open, in order to reset it).

> 📷 *[Photo: contactor wiring detail]*
> 📷 *[Photo: emergency stop button on front panel]*

> ⚠️ Double-check polarity and terminal labelling on your specific contactor model. Wiring diagrams vary between manufacturers.

---

## Sub-system 4 — SSR and immersion heater 🟡

**What it is**: a 40 A solid-state relay (SSR) switches the 3500 W immersion heater in the detergent tank. The SSR is controlled by the STC1000 temperature controller, which monitors the tank temperature via a stainless steel NTC probe.

A mechanical relay cannot reliably switch a 3500 W resistive load at 230 V. The SSR handles the load current, while the STC1000 output (a low-current relay) drives the SSR control input.

**Heatsink and cooling fan** *(Ecolab07 improvement)*: SSRs dissipate heat proportional to load current (approximately 1–1.5 W per amp). At full load, a 40 A SSR can dissipate 40–60 W without a heatsink — enough to destroy it. A heatsink and a 12 V fan are fitted to keep the SSR within its rated operating temperature during long heating sessions.

**Steps**:
1. Mount the SSR on its heatsink using thermal paste. Secure the assembly to the enclosure wall (metal wall preferred for additional heat spreading).
2. Mount the 12 V cooling fan so it blows air across the heatsink fins. Connect to the 12 V bus — the fan runs continuously when the machine is powered.
3. Connect the SSR load terminals (AC side) in series with the immersion heater supply cable (2.5 mm²).
4. Connect the SSR control terminals (DC side, typically 3–32 V) to the STC1000 output relay terminals.
5. Wire the STC1000:
   - 230 V supply to the thermostat's power input
   - Temperature probe to the thermostat's sensor input
   - Thermostat output relay to SSR control input (as above)
6. Install the temperature probe in the detergent tank via its 1/2" fitting and sealing kit.

> 📷 *[Photo: SSR mounted on heatsink with fan]*
> 📷 *[Photo: STC1000 wired and mounted]*

> ✅ **Check**: with the heater immersed and the machine powered, set the STC1000 to a target temperature above ambient. Verify the SSR LED lights when the thermostat calls for heat, and that the heater element warms up.

---

## Sub-system 5 — Relay board 🟡

**What it is**: a 16-channel relay board (only 9 relays are used) that switches the 12 V supply to each solenoid valve and the pump. Each relay is an electromechanical switch with a 12 V coil driven by a logic-level signal from the Arduino. The board uses **active-low logic**: a LOW signal on a control pin energises the relay.

**Flyback diode protection**: solenoid valves are inductive loads. When a valve de-energises, it produces a brief high-voltage back-EMF spike that can destroy relay contacts over time. A 1N4148 flyback diode placed across each valve terminal (as close to the relay output as possible) clamps this spike safely. **Do not omit these diodes** — the original Vieuxsinge build destroyed several relays before they were added.

**Steps**:
1. Mount the relay board inside the enclosure, above the power supply.
2. Connect the relay board's 12 V power input to the 12 V bus.
3. Connect the relay board's 5 V output to the Arduino's 5 V pin (this powers the Arduino).
4. Connect the relay board's GND to the Arduino GND.
5. Connect Arduino digital pins 2–10 to the relay board's IN1–IN9 control inputs, following the pin mapping table below.
6. For each of the 9 active relay outputs, wire the COM and NO (Normally Open) terminals to the corresponding solenoid valve or pump, with a 1N4148 flyback diode across the load terminals (cathode toward the positive supply).

**Pin mapping**:

| Arduino pin | Relay channel | Actuator |
|---|---|---|
| 2 | IN1 | Air solenoid valve |
| 3 | IN2 | CO₂ solenoid valve |
| 4 | IN3 | Water solenoid valve |
| 5 | IN4 | Detergent inlet valve |
| 6 | IN5 | Sanitizer inlet valve |
| 7 | IN6 | Detergent return valve |
| 8 | IN7 | Sanitizer return valve |
| 9 | IN8 | Drain valve |
| 10 | IN9 | Pump |

> 📷 *[Photo: relay board wired, showing flyback diodes on output terminals]*

---

## Sub-system 6 — Relay output bus bars 🟢

**What it is**: bus bar rails on the relay output side that group the common 12 V return wire for sets of valves. *(Ecolab07 improvement.)*

Without bus bars, each valve requires its own return wire all the way back to the power supply, producing a dense bundle of identically-coloured wires that is hard to trace and maintain. Bus bars allow one common return wire per group, with short jumpers from each relay to the bar.

**Steps**:
1. Mount a bus bar strip alongside the relay output terminals.
2. Connect one end of the bus bar to the 12 V supply common (V−).
3. For each relay output, run a short wire from the relay NO terminal to the bus bar (this is the common return side of the valve).
4. Run a dedicated wire from each relay COM terminal to the corresponding valve's positive terminal.
5. Place a 1N4148 flyback diode at each valve connection point (see Sub-system 5).

> 📷 *[Photo: bus bar rails with valve wiring]*

---

## Sub-system 7 — Arduino and terminal screw shield 🟢

**What it is**: the Arduino Uno is the brain of the machine. A **terminal screw shield** stacked on top of it (Ecolab07 improvement) replaces the fragile Dupont pin connectors of the original design with robust screw terminals, and provides a small prototyping area for the LED resistor.

**Steps**:
1. Stack the terminal screw shield onto the Arduino Uno headers.
2. On the prototyping area of the shield, solder a current-limiting resistor in series with the LED signal wire (pin 11). Typical value: 220–470 Ω depending on your LED's forward voltage and desired brightness.
3. Mount the Arduino + shield assembly inside the enclosure using stand-offs.
4. Connect Arduino pins to the relay board (see Sub-system 5 pin mapping).
5. Connect the remaining signal wires using the terminal blocks:

| Arduino pin | Connected to |
|---|---|
| A0 | Buzzer positive terminal |
| A1 | Rotary encoder pin A |
| A2 | Rotary encoder pin B |
| A3 | Action button (with pull-up via `INPUT_PULLUP` in firmware) |
| A4 | LCD SDA |
| A5 | LCD SCL |
| 11 | LED (via resistor on prototyping area) |

6. Connect 5 V and GND from the relay board to the corresponding Arduino terminals.

> 📷 *[Photo: Arduino with terminal screw shield, top view showing resistor on prototyping area]*
> 📷 *[Photo: Arduino mounted in enclosure with all wires connected]*

---

## Sub-system 8 — Front panel components 🟢

### LCD screen
The LCD is an I²C 16×2 display with a backpack module that reduces the connection to 4 wires: VCC (5 V), GND, SDA (A4), SCL (A5).

1. Mount the LCD behind the front panel cutout and secure with screws or hot glue.
2. Connect the 4-wire I²C cable to the Arduino terminal shield.

> 📷 *[Photo: LCD mounted on front panel]*

### Rotary encoder
The rotary encoder handles both mode selection (rotation) and confirmation (push).

1. Mount the encoder through its front panel hole and secure with the nut.
2. Fit the knob onto the shaft.
3. Connect encoder pins A and B to Arduino A1 and A2.
4. Connect the encoder's built-in push-button to a separate Arduino pin if used — **note**: in this firmware, the push-button function uses a dedicated action button (see below), not the encoder's built-in switch.

> 📷 *[Photo: rotary encoder with knob, front panel view]*

### Action button (with LED)
A momentary push-button with an integrated LED. The LED provides the visual heartbeat during cycle operation.

1. Mount the button through its front panel hole.
2. Connect the button terminals to Arduino A3 and GND. The firmware uses `INPUT_PULLUP`, so no external pull-up resistor is needed.
3. Connect the LED terminals to Arduino pin 11 (via the resistor on the prototyping area) and GND.

> 📷 *[Photo: action button mounted, rear view showing wiring]*

### Buzzer
A passive piezo buzzer driven by `tone()` on pin A0.

1. Mount or affix the buzzer inside the enclosure (it does not need a front panel cutout — the sound carries through the enclosure vents or wall).
2. Connect positive terminal to Arduino A0, negative terminal to GND.

---

## Sub-system 9 — Component labelling 🟢

*(Ecolab07 improvement.)* Label every component inside the enclosure. This makes fault-finding and future modifications accessible without needing to refer to this guide.

Suggested labels:
- Relay channels: `RELAY 1 — AIR`, `RELAY 2 — CO2`, etc. (follow the pin mapping table in Sub-system 5)
- Bus bar sections: `12V GND — VALVES`
- Power supply terminals: `12V OUT +`, `12V OUT −`
- SSR: `SSR — HEATER`
- Arduino terminal blocks: label each terminal with its signal name (AIR, CO2, WATER, etc.)

> 📷 *[Photo: labelled enclosure interior, full view]*
> 📷 *[Photo: labelled relay board channels]*

---

## Final checks before first power-up

Work through this checklist top to bottom. Do not skip steps.

**Mechanical**
- [ ] All screw terminals are tight (tug each wire gently)
- [ ] No bare wire strands visible outside terminals
- [ ] Cable glands are tightened
- [ ] Enclosure closes fully with no wires pinched

**230 V section** *(multimeter set to continuity, machine unplugged)*
- [ ] No continuity between L and N at the mains plug
- [ ] No continuity between L/N and PE (earth) at the mains plug
- [ ] Emergency stop button opens the L conductor when pressed

**12 V section** *(machine powered, 230 V connected, emergency stop released)*
- [ ] 12 V present at power supply output terminals
- [ ] 12 V present at relay board power input
- [ ] 5 V present at Arduino 5 V pin
- [ ] All relay LEDs are OFF (active-low: all relays should be de-energised at boot)

**Firmware upload**
- [ ] Connect Arduino to PC via USB
- [ ] Open `kegwasher.ino` in Arduino IDE
- [ ] Install required libraries: `Bounce2`, `LiquidCrystal_I2C` (fdebrabander fork), `RotaryEncoder`
- [ ] Select board: Arduino Uno, correct COM port
- [ ] Upload — no compilation errors
- [ ] LCD displays `Mode :` and a mode name on boot
- [ ] Rotary encoder scrolls through modes
- [ ] Action button launches a cycle

**Actuator test** *(run with all tanks empty)*
- [ ] Select `Test vannes` mode
- [ ] Confirm `Cuves vides ?` prompt on screen
- [ ] Each relay clicks once in sequence — verify by ear or multimeter on valve terminals
- [ ] All relays return to de-energised state after the test

---

## Troubleshooting

| Symptom | Likely cause | Check |
|---|---|---|
| All valves open at power-up | Pre-drive HIGH missing or ineffective | Verify firmware was uploaded correctly; check Arduino boot sequence |
| One valve never opens | Relay not triggering, or valve wiring | Test relay LED when that step runs; check valve terminal voltage |
| One valve never closes | Flyback diode short circuit | Remove diode and measure; replace if shorted |
| LCD blank / backlight only | Wrong I²C address | Try `0x3F` instead of `0x27` in firmware; run an I²C scanner sketch |
| Encoder scrolls in wrong direction | A/B pins swapped | Swap A1 and A2 wiring |
| SSR overheating | Insufficient cooling | Check fan is running; verify heatsink thermal paste contact |
| STC1000 not heating | SSR control signal absent | Measure DC voltage across SSR control terminals when heating is demanded |
