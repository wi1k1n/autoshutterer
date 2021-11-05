# Auto Window Shutters Manager

Automatically controls electric shutters depending on datetime and location (open after the sunrise and close on the sunset).

The ESP-based hardware serves web-server for interaction (calibration and commands). Digest Authentication is used to reject the unauthorized access.

Only external scheduling (e.g. using external python script) supported atm.

### Installation
- [TimerLED](https://github.com/wi1k1n/TimerLED)
- Other dependencies listed in platformio.ini
