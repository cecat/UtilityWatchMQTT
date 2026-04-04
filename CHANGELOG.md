# UtilityWatchMQTT Changelog

## [Unreleased] - 2026-04-03

### Changed
- `DANGER` converted from a `#define` preprocessor constant to a runtime `int` variable in `vars.h` so that it can be updated without reflashing.

### Added
- `Particle.function("setDanger", setDanger)` registered in `setup()` — allows the DANGER threshold to be updated at runtime via the Particle console or CLI:
  ```
  particle call <device> setDanger <value>
  ```
  Returns the new value on success, -1 on invalid input. Publishes a `setDanger` event confirming the change.
- `Particle.variable("danger", DANGER)` registered in `setup()` — allows the current DANGER threshold to be read via the console or:
  ```
  particle get <device> danger
  ```
- `set_danger.sh` — convenience script to get or set the DANGER threshold without remembering the particle call syntax:
  ```
  ./set_danger.sh        # print current value
  ./set_danger.sh <n>    # update to <n>
  ```
