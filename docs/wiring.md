# Wiring Notes

## Power distribution

Two isolated power domains, joined only at ground:

- **Motor power**: 8x AA battery pack (~12V) → switch → L298N driver #1 `+12V` input.
  Driver #1 is bridged to driver #2 (`+12V` to `+12V`, `GND` to `GND`) so both share
  the same battery pack.
- **Logic power**: USB (power bank or wall charger) → ESP32 `5V`/`VIN`.

These two supplies are **not** merged — only their grounds are tied together (see
below). Powering the ESP32 from the same rail as the motors caused voltage sag and
resets under motor load during early testing; splitting the supplies fixed it.

**Power bank note**: some power banks auto-shutoff under the ESP32's low idle current
draw. If that happens, check for a "low current" / "trickle charge" mode (often a
double-click on the power button).

## Common ground

The negative terminal of the battery pack, the `GND` pin of both L298N drivers, and
the `GND` pin of the ESP32 are all connected together at one point. Without a common
ground reference, sensor readings and motor behavior become erratic even if every
individual connection looks correct.

## Motor drivers (L298N)

| Driver | Controls | ESP32 pins | Notes |
|--------|----------|------------|-------|
| #1 | Rear-left motor | IN1 → GPIO26, IN2 → GPIO27 | |
| #1 | Rear-right motor | IN3 → GPIO32, IN4 → GPIO33 | |
| #2 | Front motor | IN1 → GPIO25, IN2 → GPIO14 | Wired to driver #2's first terminal block |

Both `ENA`/`ENB` enable jumpers are left in place (motors run at full speed;
no PWM throttling in the current build — see main README's Known Limitations).

Direction logic: `HIGH`/`LOW` pin pairs set spin direction per motor. All three
motors were found to be wired with inverted polarity relative to the intended
"forward" direction after final assembly; this was corrected entirely in firmware
(swapping the `HIGH`/`LOW` pair for every motor in every movement command) rather
than re-wiring, since the chassis was already sealed. See `docs/testing.md`.

## Ultrasonic sensor (HC-SR04)

| Pin  | ESP32 pin |
|------|-----------|
| VCC  | 3V3 |
| Trig | GPIO5 |
| Echo | GPIO18 |
| GND  | GND |

**Important**: the HC-SR04 is a 5V sensor by spec; its `Echo` pin normally outputs a
5V logic-high pulse, which exceeds the ESP32's 3.3V-tolerant GPIO input. Without a
level shifter or a resistor voltage divider, that risks gradually damaging the pin.
This build powers the sensor from **3.3V instead of 5V** — most HC-SR04 modules
still trigger and echo correctly at 3.3V, which brings the `Echo` output down to a
safe level, avoiding the need for a divider circuit (no resistors were available
during the build). This is a workaround, not the textbook-correct fix — a proper
level shifter or divider is preferable when the parts are on hand.

## Laser module

2-pin "Laser Emitter" module (integrated driver circuit, no external resistor
needed):

| Pin | ESP32 pin |
|-----|-----------|
| VCC | 3V3 |
| SIG | GPIO23 |

Firmware note: the laser is active-`LOW` in this build (`laser_on` → `digitalWrite(laserPin, LOW)`).

## Dome servo (planned, not physically installed)

| Pin | Source |
|-----|--------|
| Signal | ESP32 GPIO13 |
| Power (+) | L298N `+5V` output pin (**not** ESP32 5V/3.3V) |
| GND | Common ground |

The MG995/SG90-class servo draws significantly more current than the ESP32 can
safely supply from its own regulator, especially under load — powering it from the
ESP32 caused resets when the servo moved. The L298N's onboard 5V regulator (fed
from the 12V battery rail, requires the driver's `+12V` jumper in place) is the
intended power source once physically wired in.

## DFPlayer Mini (not used in final build)

Originally wired for onboard audio via UART:

| DFPlayer pin | ESP32 pin |
|--------------|-----------|
| RX | GPIO17 |
| TX | GPIO16 |
| VCC | 5V/VIN |
| GND | GND |

The module was physically removed from the team mid-build (see main README). The
firmware's serial init and `DFRobotDFPlayerMini` calls were stripped out entirely
rather than left dormant, to avoid a boot-time serial handshake that would otherwise
delay startup waiting for a module that isn't there.
