# User Manual — Automatic Keg Washer

> This project is a fork of [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher)
> and [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher).
> License: GNU GPL v3.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Prerequisites](#2-prerequisites)
3. [Controls and display](#3-controls-and-display)
4. [Starting up and selecting a mode](#4-starting-up-and-selecting-a-mode)
5. [Mode descriptions](#5-mode-descriptions)
6. [During a wash cycle](#6-during-a-wash-cycle)
7. [End of cycle](#7-end-of-cycle)
8. [Cancelling a cycle](#8-cancelling-a-cycle)
9. [Emergency stop](#9-emergency-stop)
10. [After use](#10-after-use)
11. [Practical tips](#11-practical-tips)

---

## 1. Overview

The automatic keg washer cleans stainless steel kegs **without removing the spears**. Cleaning products are injected through wash heads and circulated inside the keg by a pump.

The machine can wash **2 kegs simultaneously**.

It is equipped with:
- 2 product tanks: **detergent** (2 % caustic soda, kept at 80 °C) and **sanitizer** (1 % peracetic acid)
- 1 mains water inlet
- 1 compressed air inlet
- 1 CO₂ inlet
- 1 drain outlet

A full wash cycle takes approximately **5 minutes 30 seconds**.

---

## 2. Prerequisites

Before starting any cycle, check the following:

- [ ] **Product tanks are filled** to the correct level (detergent and sanitizer)
- [ ] **Detergent tank is at temperature** (80 °C — check the STC1000 thermostat)
- [ ] **Water supply** is open
- [ ] **Compressed air supply** is open (recommended pressure: **2.5 bar**)
- [ ] **CO₂ supply** is open (recommended pressure: **2.5 bar**)
- [ ] **Kegs are connected** to the wash heads (spear facing down)
- [ ] **Drain outlet** is clear
- [ ] **Emergency stop button** is released (turn to unlock)

---

## 3. Controls and display

The machine has three controls on the front panel:

| Control | Function |
|---|---|
| **Rotary encoder** | Browse modes (rotate = change mode) |
| **Action button** | Confirm selected mode / cancel a running cycle |
| **Emergency stop button** | Immediate full power cut via contactor |

The **LCD screen** (2 lines × 16 characters) shows:
- Line 1: the name of the selected mode or the current step
- Line 2: cycle progress (elapsed time / total time)

The **LED** on the front panel:
- **Solid on**: waiting for selection
- **Blinking**: cycle in progress

The **buzzer** sounds:
- **1 beep**: cycle cancelled
- **3 beeps**: cycle completed normally

---

## 4. Starting up and selecting a mode

1. Power the machine on.
2. The screen shows `Mode :` on the first line and the name of the last used mode on the second *(the last mode is stored in EEPROM and restored after a power cut)*.
3. **Rotate the encoder** to browse the available modes.
4. **Press the action button** to launch the displayed mode.
5. The screen shows `Preparation` while the machine configures the solenoid valves, then the cycle begins.

---

## 5. Mode descriptions

> For the actuator sequence diagrams of each mode (which valves are active at each step), see the [Mode sequence diagrams (GRAFCET)](GRAFCET_ALL_MODES.md).

### Full wash modes

#### `Lavage + CO2` — 330 s (5 min 30)
Full wash cycle. Recommended for kegs that have contained beer.

| Phase | Duration | Description |
|---|---|---|
| Initial drain | 10 s | Residual contents flushed to drain |
| Initial rinse | 30 s | Water + air purge to remove coarse residue |
| Detergent (×3) | 75 s | Hot caustic soda circulation at 80 °C + air purge between each pass |
| Intermediate rinse (×3) | 60 s | Complete removal of detergent traces |
| Sanitizer (×3) | 65 s | Peracetic acid circulation + air purge between each pass |
| Final rinse + CO₂ purge | 40 s | Water + CO₂ purge to displace oxygen |
| CO₂ pressurisation | 10 s | Keg pressurised before filling |

✅ **Keg ready to fill directly.**

---

#### `Lavage sans CO2` — 320 s (5 min 20)
Same as the above but without the final CO₂ pressurisation step. The final purge uses compressed air.

Use this mode if CO₂ pressurisation is done separately, or if the keg will not be filled immediately.

---

#### `Detergent seul` — 180 s (3 min 00)
Detergent wash and rinses only — no sanitization, no CO₂.

Use for a quick clean between two closely-spaced uses, or when sanitization will be done in a separate step.

---

### Sanitization and pressurisation modes

#### `Desinf. + CO2` — 190 s (3 min 10)
Sanitization only, followed by a CO₂ purge and pressurisation.

Use on a keg that **has already been washed with detergent** in a previous session.

---

#### `CO2` — 50 s
CO₂ purge and pressurisation only — no chemicals.

Use to re-pressurise a clean keg that has been left open, or to displace residual air.

---

### Product tank maintenance modes

#### `Vidange fut` — 70 s (1 min 10)
Drains the keg to the drain outlet, followed by a long air purge to dry the lines.

Use to empty a keg before storage or return.

---

#### `Vidange desinf.` — 200 s (3 min 20)
Empties the **sanitizer tank** through the circuit to the drain via the pump.

Use at the end of a session to empty the sanitizer tank, or to renew the solution.

---

#### `Vidange deter.` — 200 s (3 min 20)
Empties the **detergent tank** through the circuit to the drain via the pump.

Use to renew the detergent solution or at the end of the season.

---

#### `Rempl. desinf.` — 120 s (2 min 00)
Fills the **sanitizer tank** with mains water. Water enters the keg via the water inlet, then is pumped back into the sanitizer tank through the return valve (`SANITIZER_OUT`).

> ⚠️ Make sure the tank is empty or has enough capacity before starting this mode.

---

#### `Rempl. deter.` — 120 s (2 min 00)
Fills the **detergent tank** with mains water. Water enters the keg via the water inlet, then is pumped back into the detergent tank through the return valve (`CLEANER_OUT`).

> ⚠️ Same precaution as for sanitizer refilling. Remember to add caustic soda after filling.

---

### Test mode

#### `Test vannes` — 24 s
Activates each actuator **individually** for 1 second, in the physical order of the relay board (left to right, top to bottom).

> ⚠️ **All tanks must be empty before running this mode.** The screen displays `Cuves vides ?` for 5 seconds at the start. Wait for this prompt and confirm before the test begins.

Use to verify wiring, diagnose a silent solenoid valve, or confirm correct assembly after build or repair.

Firing order: sanitizer return → drain → detergent return → air → CO₂ → sanitizer inlet → water → detergent inlet → pump.

---

## 6. During a wash cycle

- Line 1 of the screen shows the current step name (alternating with the mode name).
- Line 2 shows elapsed time and total cycle time.
- The LED blinks every 2 seconds.
- **Do not disconnect kegs during a cycle.**

---

## 7. End of cycle

When all steps have completed:
1. All solenoid valves close and the pump stops.
2. The LED turns off.
3. The screen displays `Termine`.
4. The buzzer sounds **3 beeps**.
5. The machine returns automatically to the selection menu.

---

## 8. Cancelling a cycle

Press the **action button** during a running cycle to cancel it immediately:
1. All solenoid valves close and the pump stops.
2. The screen displays `Annule`.
3. The buzzer sounds **1 beep**.
4. The machine returns to the selection menu.

> ⚠️ If a cycle is cancelled mid-run, the kegs and lines may contain product residue. Run a rinse cycle before filling the kegs.

---

## 9. Emergency stop

The **mushroom-head button** on the front panel cuts the main power via a contactor.

It acts on the **entire machine** (pump, solenoid valves, Arduino).

To reset: **turn the button** in the indicated direction until it clicks out.

> ⚠️ After an emergency stop, all solenoid valves return to their rest state (closed = safe). Check the circuit before restoring power.

---

## 10. After use

- Check that product tanks are at the right level for the next session.
- If the session is over for the day, product tanks can be left in place (they are designed to remain in circuit).
- Close the water, air, and CO₂ supplies if the machine will not be used for an extended period.
- Rinse and dry the wash heads.

---

## 11. Practical tips

- **Time efficiency**: washing (5 min) can run in parallel with filling already-clean kegs. This is the main advantage of automation.
- **Detergent temperature**: wait for the STC1000 thermostat to reach 80 °C before starting a full wash cycle. Cold detergent is significantly less effective. A stirrer could help homogenise the tank temperature, but in practice the turbulence generated by the circulation pump during detergent cycles is sufficient to keep the solution well mixed.
- **Product renewal**: use the `Vidange` and `Rempl.` modes to renew solutions at the end of the season or according to the product supplier's recommendations.
- **Wash head check**: make sure the wash heads are properly clicked onto the spears before each cycle. A loose connection causes leaks and poor cleaning results.
