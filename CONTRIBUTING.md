# Contributing to kegwasher

Thank you for your interest in contributing! This project is a DIY open-source keg washer controller. Contributions are welcome from brewers, makers, and developers alike — whether you are fixing a bug, improving documentation, or sharing a new wash program.

---

## Table of Contents

1. [Code of conduct](#1-code-of-conduct)
2. [Ways to contribute](#2-ways-to-contribute)
3. [Reporting a bug](#3-reporting-a-bug)
4. [Suggesting an improvement](#4-suggesting-an-improvement)
5. [Submitting a pull request](#5-submitting-a-pull-request)
6. [Code conventions](#6-code-conventions)
7. [Adding a new wash mode](#7-adding-a-new-wash-mode)
8. [Documentation contributions](#8-documentation-contributions)

---

## 1. Code of conduct

Be respectful and constructive. This is a small community project maintained in spare time. Issues and pull requests that are rude, dismissive, or off-topic will be closed without response.

---

## 2. Ways to contribute

You do not need to write code to contribute. All of the following are valuable:

- **Report a bug** you encountered building or using the machine.
- **Share timing adjustments** that work better for your water pressure, pump, or products.
- **Propose a new wash mode** for a use case not covered by the existing programs.
- **Improve the documentation** — fix a typo, add a photo, clarify a step.
- **Share your build** — photos, component substitutions, enclosure layouts.
- **Translate documentation** into another language.

---

## 3. Reporting a bug

Use a [GitHub Issue](../../issues/new) with the label `bug`.

Please include:

- **What you expected to happen.**
- **What actually happened** — include LCD output, relay behaviour, or any error you observed.
- **Steps to reproduce** — which mode were you running, what were the conditions (tank levels, temperatures, etc.).
- **Firmware version or commit hash** if known.
- **Hardware differences from the reference build** — different relay board, pump model, Arduino variant, etc.

> For safety-critical bugs (unexpected valve activation, inability to stop a cycle, emergency stop not working), please open an issue immediately and mark it with the `safety` label.

---

## 4. Suggesting an improvement

Use a [GitHub Issue](../../issues/new) with the label `enhancement`.

Please describe:

- **The problem you are trying to solve** — not just the solution you have in mind.
- **Your proposed approach**, if you have one.
- **Any trade-offs or risks** you are aware of.

Timing adjustments and new wash mode proposals are welcome as issues before any code is written — it is useful to discuss the chemistry and intent before implementation.

---

## 5. Submitting a pull request

1. **Open an issue first** for anything beyond a trivial fix (typo, comment). This avoids duplicate work and allows discussion before you invest time in implementation.
2. **Fork the repository** and create a branch from `main`:
   ```
   git checkout -b fix/valve-ordering
   git checkout -b feature/new-wash-mode
   git checkout -b docs/update-bom
   ```
3. **Make your changes** following the conventions below.
4. **Test on hardware** if your change touches the firmware. Describe your test conditions in the PR. For a first sanity check before physical testing, the [Wokwi simulation](https://wokwi.com/projects/464738080652364801) runs the full sketch and can help catch logic errors early. Note that Wokwi simulates time slower than real speed — do not rely on it for timing validation.
5. **Update documentation** if your change affects user-visible behaviour, hardware requirements, or wiring.
6. **Open the pull request** against `main`. Fill in the PR template:
   - What does this change do?
   - How was it tested?
   - Does it affect any existing mode or behaviour?
   - Does it require a documentation update?

Pull requests that lack hardware testing evidence or break existing modes without justification will be asked to revise before merging.

---

## 6. Code conventions

The firmware is a single Arduino `.ino` file. Keep it that way unless there is a compelling reason to split it.

### Naming
- Constants: `UPPER_SNAKE_CASE` — e.g. `CONFIG_RINCE`, `CTRL_AIR`
- Variables and functions: `lower_snake_case` — e.g. `step_start_time`, `controls_set()`
- Types: `lower_snake_case_t` — e.g. `step_t`, `mode_t`, `state_t`

### Comments
- All comments in **English**.
- Comments explain *why*, not *what*. The code already says what it does.
- Non-obvious hardware constraints (active-low logic, pin pre-drive trick, valve ordering rationale) must be documented at the relevant code site.

### Step arrays
- Each `step_t` array must end with `{CONFIG_END, 0}`.
- Steps with tunable durations should be followed by `// Adjust if needed`.
- Group related steps with a `// ===== Phase name =====` separator comment.
- Add a one-line comment above the array with the total duration and a human-readable breakdown — e.g. `// Full wash cycle — 335 s (5 min 35)`.

### New `#define` flags
- New `CTRL_*` bits must use the next available power-of-two bit and must not overlap with existing bits.
- New `CONFIG_*` composite values must be defined as a sum of existing `CTRL_*` bits.
- Pseudo-configuration flags used only in special modes (like `CONFIG_WARNING` and `CONFIG_WAIT`) must use bits beyond the 9-bit actuator range and must be clearly documented.

### State machine
- Each state must have exactly one handler function.
- State transitions are written as `state = STATE_*` assignments — never call another state's handler directly.

### Rotary encoder menu wrap
The menu position uses a double-modulo guard to handle any depth of counter-clockwise rotation:
```c
new_mode = ((pos % MODES_NUMBER) + MODES_NUMBER) % MODES_NUMBER;
```
Do not simplify this to `(pos + MODES_NUMBER) % MODES_NUMBER` — that form silently produces a negative index when `pos < -MODES_NUMBER` (C modulo sign follows the dividend).

---

## 7. Adding a new wash mode

New wash modes are the most common contribution. The architecture makes this straightforward.

**Steps:**

1. Define a new `step_t` array. Follow the naming convention `STEPS_<DESCRIPTION>[]`.
2. Add a corresponding entry to the `MODES[]` table: `{"Display name", STEPS_<DESCRIPTION>}`.
3. `MODES_NUMBER` is computed automatically from `sizeof(MODES)` — no manual update needed.
4. Add a description of the new mode to `USER_MANUAL.md` (and `MANUEL_UTILISATION.md` if you speak French) following the existing format:
   - Mode name, total duration, use case description
   - Phase table (phase name, duration, description)
5. Add an entry to the `CHANGELOG.md` under the appropriate version section.
6. Test the mode on hardware with actual product tanks and kegs. Report fill levels, water pressure, and pump model in the PR.

**Important**: do not change the durations of existing modes in a contribution. If your hardware requires different timings, open an issue to discuss whether the existing defaults should change, or document how to adjust them locally.

---

## 8. Documentation contributions

Documentation is as important as code for a DIY hardware project — it is what allows other brewers to build and use the machine.

- All documentation is written in **Markdown** for native GitHub rendering.
- English is the primary language. French translations are maintained in parallel (separate files).
- Photo placeholders are marked `📷 *[Photo: description]*`. If you have a relevant photo, replace the placeholder with `![description](images/filename.jpg)` and include the image file in the PR.
- Keep a consistent tone: practical, direct, safety-first. Avoid marketing language.
- If you correct a factual error (wrong pin number, wrong component value, wrong chemical concentration), cite your source or describe how you verified the correction.
