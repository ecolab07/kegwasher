# Keg Washer — Automatic Keg Cleaning Machine

> 🇫🇷 [Version française](docs/FR/README_FR.md)

An open-source automatic keg washer controller built on Arduino Uno. Cleans stainless steel kegs **without removing the spears**, using hot caustic soda and peracetic acid circulated by a pump through wash heads.

This project is a fork of [vieuxsinge/kegwasher](https://github.com/vieuxsinge/kegwasher) and [Gummy35/kegwasher](https://github.com/Gummy35/kegwasher). Licensed under [GNU GPL v3](LICENSE).

---

## What it does

The machine automates the full keg cleaning sequence:

- Initial drain and rinse
- Detergent circulation (caustic soda at 80 °C, 3 passes)
- Intermediate rinse
- Sanitizer circulation (peracetic acid, 3 passes)
- Final rinse + CO₂ purge and pressurisation

A full wash cycle takes approximately **5 minutes 30 seconds**. The machine can wash **2 kegs simultaneously**.

📷 *[Photo: machine overview]*

---

## Features

- 11 selectable wash programs via rotary encoder menu
- I²C LCD display: mode name, current step, elapsed / total time
- EEPROM persistence of last selected mode across power cycles
- LED heartbeat and buzzer end-of-cycle notification
- Actuator test mode: fires each valve individually for wiring verification
- Emergency stop via contactor (mushroom-head button)

---

## Hardware

| Component | Details |
|---|---|
| Controller | Arduino Uno (ATmega328P) |
| Relay board | 16-channel, active-low, 12 V |
| Solenoid valves | 9 × 12 V |
| Pump | Novax 20B (or equivalent) |
| Heater | 2500 W immersion element |
| Thermostat | STC1000 |
| SSR | 40 A + heatsink + 12 V fan |
| Display | I²C LCD 16×2 |
| Encoder | Rotary encoder (A1/A2) + action button (A3) |

---

## Try it in the browser

A complete Wokwi simulation is available — no hardware required:

**[▶ Open simulation](https://wokwi.com/projects/464738080652364801)**

Solenoid valves are represented by labelled LEDs. The full sketch runs without modification. Note: Wokwi simulates time slower than real speed.

---

## Repository structure

```
/
├── kegwasher.ino               — Arduino firmware (main branch)
├── CHANGELOG.md                — Fork history and change log
├── CONTRIBUTING.md             — How to contribute
├── LICENSE                     — GNU GPL v3
│
├── docs/
│   ├── EN/
│   │   ├── USER_MANUAL.md      — Operating instructions
│   │   ├── README_TECHNICAL.md — Firmware design decisions
│   │   ├── ELECTRONICS_HOWTO.md— Wiring and assembly guide
│   │   ├── MAINTENANCE_GUIDE.md— Maintenance schedule and procedures
│   │   └── GRAFCET_ALL_MODES.md— Actuator sequence diagrams (all modes)
│   └── FR/
│       ├── README_FR.md
│       ├── MANUEL_UTILISATION.md
│       ├── README_TECHNIQUE.md
│       ├── GUIDE_ELECTRONIQUE.md
│       ├── GUIDE_ENTRETIEN.md
│       └── GRAFCET_TOUS_MODES.md
│
├── fritzing/
│   ├── kegwasher.fzz           — Fritzing source file
│   └── kegwasher.png           — Wiring diagram
│
└── Images/                     — Photos and illustrations
```

---

## Documentation

| Document | EN | FR |
|---|---|---|
| User manual | [EN](docs/EN/USER_MANUAL.md) | [FR](docs/FR/MANUEL_UTILISATION.md) |
| Technical README | [EN](docs/EN/README_TECHNICAL.md) | [FR](docs/FR/README_TECHNIQUE.md) |
| Electronics assembly guide | [EN](docs/EN/ELECTRONICS_HOWTO.md) | [FR](docs/FR/GUIDE_ELECTRONIQUE.md) |
| Maintenance guide | [EN](docs/EN/MAINTENANCE_GUIDE.md) | [FR](docs/FR/GUIDE_ENTRETIEN.md) |
| Mode sequence diagrams | [EN](docs/EN/GRAFCET_ALL_MODES.md) | [FR](docs/FR/GRAFCET_TOUS_MODES.md) |

---

## Fork history

| Generation | Author | Contributions |
|---|---|---|
| v1 | [vieuxsinge](https://github.com/vieuxsinge/kegwasher) | Original project: step engine, bitmask model, wash programs, state machine |
| v2 | [Gummy35](https://github.com/Gummy35/kegwasher) | Ordered actuator sequencing, LCD alternating display, new modes, CO₂ pressurisation |
| v3 | [ecolab07](https://github.com/ecolab07/kegwasher) | Test mode, minimal transition logic, timing fixes, type safety, hardware improvements |

See [CHANGELOG.md](CHANGELOG.md) for the full detailed history.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Issues and pull requests are welcome.
