# Maintenance Guide — Keg Washer

> This project is a fork of [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher)
> and [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher).
> License: GNU GPL v3.

---

## Table of Contents

1. [Safety reminders](#1-safety-reminders)
2. [Maintenance schedule overview](#2-maintenance-schedule-overview)
3. [After each session](#3-after-each-session)
4. [Weekly](#4-weekly)
5. [Monthly](#5-monthly)
6. [Seasonally / end of season](#6-seasonally--end-of-season)
7. [As needed](#7-as-needed)
8. [Troubleshooting common issues](#8-troubleshooting-common-issues)

---

## 1. Safety reminders

> ⚠️ **Always disconnect the machine from the mains before opening the enclosure or touching any internal component.**

- Never perform maintenance on the hydraulic circuit while a cycle is running.
- After an emergency stop, verify that all valves have returned to their closed (rest) state before inspecting the circuit.
- Wear appropriate PPE (gloves, eye protection) when handling detergent (caustic soda) or sanitizer (peracetic acid) solutions.
- Rinse any skin contact with running water immediately.

---

## 2. Maintenance schedule overview

| Task | After each session | Weekly | Monthly | Seasonal |
|---|---|---|---|---|
| Rinse wash heads | ✅ | | | |
| Check product tank levels | ✅ | | | |
| Check hose connections for leaks | ✅ | | | |
| Inspect wash heads for scale/fouling | | ✅ | | |
| Check solenoid valve response (listen for click) | | ✅ | | |
| Verify STC1000 temperature reading | | ✅ | | |
| Inspect flyback diodes and relay board | | | ✅ | |
| Check all screw terminal tightness | | | ✅ | |
| Full drain and renew product solutions | | | | ✅ |
| Descale hydraulic circuit | | | | ✅ |
| Inspect pump seals and impeller | | | | ✅ |
| Inspect hoses for cracks or hardening | | | | ✅ |
| Run `Test vannes` actuator test | | | ✅ | |
| Clean enclosure interior | | | | ✅ |

---

## 3. After each session

### 3.1 Rinse wash heads 🟢
After removing the kegs, flush the wash heads with clean water to remove residual product. Dry with a clean cloth and store in a clean location.

### 3.2 Check product tank levels 🟢
Visually check the detergent and sanitizer tank levels. Top up if needed before the next session so the machine is ready to use.

> Note: detergent solution (caustic soda) concentration degrades over time with use. Track the number of wash cycles between renewals and follow your product supplier's recommendations.

### 3.3 Check hose connections 🟢
Walk around the machine and visually inspect all hose connections for drips or weeping. Tighten any loose hose clamps. A small leak left unattended can corrode fittings and stainless steel surfaces over time.

---

## 4. Weekly

### 4.1 Inspect wash heads for scale and fouling 🟢
Hard water deposits (limescale) and peracetic acid residue can build up inside the wash head nozzles, reducing flow and cleaning effectiveness.

- Disassemble the wash heads if your model allows it.
- Soak in a dilute descaling solution (citric acid or proprietary descaler) for 30 minutes.
- Rinse thoroughly before reinstalling.

### 4.2 Check solenoid valve response 🟢
Run the `Test vannes` mode (all tanks empty) and listen for each relay click and valve actuation in sequence. A valve that does not click or does not produce a flow change when expected may be sticking or have a failed coil.

### 4.3 Verify STC1000 temperature reading 🟢
Check that the STC1000 display reads a plausible temperature for the detergent tank (ambient if cold, target temperature if recently heated). An implausible reading (e.g. `---` or a value far outside range) indicates a failed or disconnected temperature probe.

---

## 5. Monthly

### 5.1 Inspect flyback diodes and relay board 🟡
Open the enclosure (machine unplugged). Visually inspect:
- The 1N4148 flyback diodes at each relay output — look for any that are visibly burnt, cracked, or have discoloured surrounding wiring.
- The relay board itself — look for burnt or discoloured relay bodies, which indicate a relay that has been running near its current limit.
- All Dupont connectors or terminal block connections between the Arduino and relay board — re-seat any that feel loose.

### 5.2 Check all screw terminal tightness 🟡
With the machine unplugged, use a flat-head screwdriver to gently re-tighten every screw terminal in the enclosure. Thermal cycling (heating and cooling) causes terminals to loosen over time, which can cause intermittent faults or arcing.

Pay particular attention to:
- The 230 V section (transformer primary, SSR, contactor)
- The relay output terminals connected to solenoid valve cables

### 5.3 Run the actuator test 🟢
Run `Test vannes` with all tanks empty. This provides a monthly baseline: if a relay that previously clicked cleanly now sounds different or fails to actuate its valve, investigate before the issue becomes a cycle failure mid-wash.

---

## 6. Seasonally / end of season

### 6.1 Full drain and renew product solutions 🟢
At the end of the brewing season (or when solutions have been in use for the recommended duration):

1. Run `Vidange deter.` to pump out the detergent tank through the circuit.
2. Run `Vidange desinf.` to pump out the sanitizer tank.
3. Use `Rempl. deter.` and `Rempl. desinf.` to refill with fresh water.
4. Run a full wash cycle (`Lavage sans CO2`) with plain water to rinse all lines.
5. Drain again with `Vidange deter.` and `Vidange desinf.`.
6. Prepare and add fresh product solutions at the correct concentrations:
   - Detergent: caustic soda 2 % (by weight)
   - Sanitizer: peracetic acid 1 % (follow product datasheet)

> ⚠️ Always add chemical concentrates to water, not water to concentrate. Wear gloves and eye protection.

### 6.2 Descale the hydraulic circuit 🟡
Hard water scale builds up progressively inside hoses, fittings, the pump, and valve bodies. A seasonal descaling prevents blockages and maintains flow rates.

1. Fill the detergent tank with a dilute citric acid solution (1–2 % by weight) at room temperature.
2. Run `Lavage sans CO2` to circulate the descaling solution through the full circuit.
3. Drain with `Vidange deter.`.
4. Run two plain-water rinse cycles to flush residual acid before refilling with detergent solution.

> Note: do not use citric acid at high temperature — it accelerates corrosion of stainless steel fittings.

### 6.3 Inspect pump seals and impeller 🟡
The Novax 20B pump (or equivalent) has rubber seals that degrade with chemical exposure over time.

- Inspect the pump head for any weeping around the shaft seal.
- If the pump has been noisy or showing reduced flow, open the pump head and inspect the impeller for wear, cracking, or chemical damage.
- Replace seals annually or at the first sign of leakage.

### 6.4 Inspect hoses 🟢
Inspect all hoses for:
- Cracks, kinks, or hardening (especially Thermoclean hose near heat sources)
- Soft spots or bulges (indicating internal delamination)
- Discolouration or chemical degradation at fittings

Replace any suspect section before the start of the next season. Hose failure mid-cycle can cause chemical spills and machine damage.

### 6.5 Clean the enclosure interior 🟡
With the machine unplugged:
- Use a dry cloth or soft brush to remove dust from the relay board, Arduino, and terminal blocks.
- Do not use compressed air directly on the relay board — it can drive conductive dust into relay contacts.
- Wipe the interior walls of the enclosure with a slightly damp cloth, then dry thoroughly before closing.
- Check that the SSR heatsink and cooling fan are free of dust buildup. A clogged heatsink significantly reduces thermal performance.

---

## 7. As needed

### 7.1 Replace a solenoid valve 🟡
Signs of a failing solenoid valve: valve does not open (no flow despite relay clicking), valve does not close (constant drip at rest), or coil overheating.

1. Run `Vidange deter.` and `Vidange desinf.` to empty the lines.
2. Shut off the water supply.
3. Depressurise the circuit by running a short `Vidange fut` cycle.
4. Disconnect the valve's electrical connector at the relay output terminal.
5. Unscrew the valve from its fitting (PTFE tape seal will need replacing).
6. Install the new valve with fresh PTFE tape. Tighten to the fitting manufacturer's torque specification.
7. Reconnect the electrical terminals. Ensure the flyback diode is in place.
8. Run `Test vannes` to verify the replacement valve responds correctly before refilling the tanks.

### 7.2 Replace a relay 🟡
Signs of a failing relay: relay does not click when signalled, relay clicks but load voltage is absent, relay contacts welded (valve stays open when it should close).

1. Unplug the machine.
2. Identify the failed relay channel from the board labelling.
3. Disconnect the wiring from the relay output terminals.
4. Most relay boards use socketed relays — pull the relay body straight out of its socket and press the replacement in.
5. Reconnect wiring. Verify flyback diode is in place on the output.
6. Power up and run `Test vannes` to confirm.

### 7.3 Replace the SSR cooling fan 🟢
If the fan stops running (audible silence, or visible from enclosure vents):

1. Unplug the machine.
2. Disconnect the fan's 12 V power wires.
3. Unscrew and replace with an equivalent 12 V fan of the same dimensions.
4. Reconnect and verify rotation direction — airflow should blow across the heatsink fins, not away from them.

### 7.4 Adjust wash cycle timing 🟢
Step durations are defined directly in the `step_t` arrays in `kegwasher.ino`. To adjust a step duration:

1. Open `kegwasher.ino` in the Arduino IDE.
2. Find the relevant step array (e.g. `STEPS_WASH_KEG_PRESSURIZE`).
3. Change the duration value (in seconds) for the relevant step.
4. Re-upload the firmware to the Arduino.

Steps marked `// Adjust if needed` in the source code are the most likely candidates for tuning to your specific water pressure, pump, and product concentrations.

---

## 8. Troubleshooting common issues

| Symptom | Likely cause | Action |
|---|---|---|
| Detergent tank not reaching 80 °C | SSR not switching / probe fault / heater fault | Check STC1000 display; measure SSR control voltage; check heater continuity |
| Poor cleaning results | Detergent concentration low / temperature too low / step durations too short | Check concentration; verify STC1000 setpoint; increase step durations in firmware |
| Residual product smell after rinse | Rinse steps too short | Increase `CONFIG_RINCE` durations in `STEPS_WASH_KEG*` arrays |
| Pump noisy or low flow | Air in circuit / impeller wear / inlet restriction | Check inlet valve is opening; inspect pump head |
| Valve stays open after cycle | Relay contacts welded or debris in valve seat | Run `Test vannes`; replace relay or clean/replace valve |
| Valve never opens | Coil failure / relay failure / wiring fault | Check relay click; measure voltage at valve terminals |
| LCD shows wrong temperature on STC1000 | Probe disconnected or failed | Check probe connector; replace probe if reading is implausible |
| Machine trips RCD/GFCI | Earth leakage in heater or pump | Disconnect loads one by one to isolate; check heater element insulation resistance |
