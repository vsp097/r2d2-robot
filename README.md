# Arturito — R2-D2 Inspired Mobile Robot

Arturito is a WiFi-controlled mobile robot inspired by Star Wars' R2-D2, built on an ESP32.
It combines embedded systems, a REST-style HTTP API, a browser control panel, and a
voice interface powered by the Gemini API to translate natural-language speech into
real-time motor commands.

Built for the *Robótica Industrial* course at Universidad Metropolitana (Caracas, Venezuela).

> Add your build photos to `media/` and reference them here, e.g.
> `![Arturito finished build](media/arturito_final.jpg)`

## Features

- **Tricycle drive** — 3 DC motors (two rear, one front) driven by two L298N H-bridge modules
- **Obstacle sensing** — HC-SR04 ultrasonic sensor for real-time distance measurement
- **Dual control surface** — an onboard web page served directly from the ESP32, and an
  external voice-controlled page (hosted on Netlify) using the Gemini API for natural
  language understanding
- **Personality modes** — timed motor + light sequences (happy, party, angry, scared,
  and an autonomous obstacle-avoidance mode)
- **Autonomous behavior** — a perception-action loop: the robot drives forward, samples
  the ultrasonic sensor every 100ms, and reactively steers around obstacles it detects
  within 20cm
- **Client-side audio** — sound effects and voice lines play from the controlling
  device (phone/laptop) in sync with robot commands, over the existing WiFi link

## Architecture

```
┌─────────────────────┐        WiFi / HTTP        ┌──────────────────────┐
│   Control clients    │ ─────────────────────────▶│        ESP32          │
│                       │                            │  (AsyncWebServer)     │
│  • Onboard web page   │◀───────────────────────── │                        │
│    (served by ESP32)  │      JSON telemetry        │  GET  /               │
│  • Netlify voice page │                            │  GET  /api/estado     │
│    + Gemini API       │                            │  GET  /api/cmd?c=...  │
└─────────────────────┘                            └───────────┬────────────┘
                                                                 │
                                                     digitalWrite / PWM
                                                                 │
                                          ┌──────────────────────┼──────────────────────┐
                                          ▼                      ▼                      ▼
                                    L298N driver #1        L298N driver #2        HC-SR04 / Laser
                                    (2x rear motors)        (1x front motor)      (sensing + FX)
```

The ESP32 exposes a small REST-style API:

| Endpoint       | Method | Description                                                  |
|----------------|--------|----------------------------------------------------------------|
| `/`            | GET    | Serves the onboard control page (stored in flash via `PROGMEM`) |
| `/api/estado`  | GET    | Returns live ultrasonic distance as JSON: `{"distancia": 42.3}` |
| `/api/cmd?c=X` | GET    | Executes a command (movement, mode, accessory)                  |

The server runs on **ESPAsyncWebServer**, a non-blocking (asynchronous) HTTP server —
this lets the ESP32 keep sampling the ultrasonic sensor and stay responsive to new
commands while a previous request is still being handled, instead of stalling on
each request like a synchronous server would.

## Repository structure

```
arturito/
├── firmware/
│   ├── arturito_robot.ino     # Main ESP32 sketch (WiFi, motors, sensor, modes, API)
│   └── index_html.h           # Onboard control page, embedded via PROGMEM
├── web/
│   └── index.html             # External control page: voice (Gemini) + manual controls
│                               # (deployed to Netlify; also runs locally via `python -m http.server`)
├── docs/
│   ├── wiring.md               # Pinout and wiring notes
│   └── testing.md              # Test log / validation notes
└── media/                      # Build photos
```

## Hardware

| Component                    | Qty | Notes                                      |
|-------------------------------|-----|---------------------------------------------|
| ESP32 DevKit (WROOM)          | 1   | Main controller, WiFi                        |
| L298N H-bridge driver         | 2   | One per motor pair (rear / front)             |
| DC gear motor + wheel         | 3   | Tricycle traction layout                      |
| HC-SR04 ultrasonic sensor     | 1   | Powered at 3.3V to stay within ESP32 I/O limits |
| SG90 / MG995 servo            | 1   | Dome rotation (implemented in firmware; pending physical install — see [Known limitations](#known-limitations)) |
| Laser emitter module          | 1   | Front-facing, digital on/off                  |
| 8x AA battery pack             | 1   | Motor power (separate from ESP32 supply)      |
| USB power bank / cable        | 1   | ESP32 power (isolated from motor supply)      |

### Pinout

| Function          | ESP32 pin |
|-------------------|-----------|
| Rear motor 1 (IN1/IN2) | GPIO26 / GPIO27 |
| Rear motor 2 (IN3/IN4) | GPIO32 / GPIO33 |
| Front motor (IN5/IN6)  | GPIO25 / GPIO14 |
| Ultrasonic Trig        | GPIO5  |
| Ultrasonic Echo        | GPIO18 |
| Dome servo signal      | GPIO13 |
| Laser signal           | GPIO23 |

All grounds (battery pack, both L298N drivers, ESP32) are tied to a single common
reference point — a basic but essential rule to keep signal readings stable and
prevent brownout resets when the motors draw current.

Full wiring notes: [`docs/wiring.md`](docs/wiring.md)

## Software

**Firmware** (`firmware/`), built with Arduino IDE for ESP32:
- `WiFi.h` — station-mode WiFi connection
- `ESPAsyncWebServer` — non-blocking HTTP server
- `ESP32Servo` — PWM control for the dome servo

**Control page** (`web/index.html`):
- Browser SpeechRecognition API for voice capture (Spanish)
- Gemini API (`gemini-2.5-flash`) to translate natural-language speech into a fixed
  command vocabulary
- `fetch(..., {mode:'no-cors'})` to send commands to the robot's HTTP API from a
  browser tab — required to work around mixed-content restrictions when the control
  page is served over HTTPS (Netlify) and the robot serves plain HTTP
- Client-side audio playback (`HTMLAudioElement`), triggered by the same commands
  that drive the robot, so sound stays in sync with movement without needing an
  onboard audio module

## Setup

### 1. Firmware
1. Open `firmware/arturito_robot.ino` in Arduino IDE (ESP32 board package installed)
2. Edit the WiFi credentials at the top of the file
3. Keep `index_html.h` in the same sketch folder
4. Upload to the ESP32, then open the Serial Monitor at `115200` baud to read the
   assigned IP address

### 2. Control page
The page can run two ways:

**Locally** (recommended when the page needs to send commands to a robot on a plain
HTTP local network — avoids browser mixed-content blocking entirely):
```bash
cd web
python -m http.server 8000
# open http://localhost:8000
```

**Deployed** (e.g. Netlify), for remote/voice-only demos:
1. Drag the `web/` folder contents (including any audio files) into Netlify's deploy
   zone
2. Paste a Gemini API key into `web/index.html` where marked
3. Open the deployed URL, enter the robot's IP in the settings (⚙) panel

### 3. Audio files
Sound effects and voice lines are referenced by base filename (e.g. `saludo.mp3`,
`0001.mp3`) and must sit alongside `index.html`, whichever way it's served. They are
not included in this repository (own recordings / clips).

## Known limitations

- **Dome servo**: implemented and tested in firmware (`mira_izq` / `mira_der` /
  animation commands all move the servo in code), but not physically mounted in this
  build — the servo's current draw needed a separate protected power path that wasn't
  sourced in time. Wiring plan: signal from ESP32 GPIO13, power from the L298N's 5V
  rail (not the ESP32's own 5V/3.3V), common ground.
- **Onboard audio (DFPlayer Mini)**: originally planned as an onboard MP3 module, but
  the team lost access to that component mid-build. Audio was moved client-side
  (see [Software](#software)) using the existing WiFi link — same command vocabulary,
  different playback location.
- **Speed control (PWM)**: motors currently run open-loop (full on/off via
  `digitalWrite`), not PWM-throttled. Left as a documented future improvement rather
  than retrofitted late in the build (see [`docs/testing.md`](docs/testing.md)).

## Testing

See [`docs/testing.md`](docs/testing.md) for the validation log: motor direction
checks, sensor range tests, and the browser mixed-content issue encountered when
mixing an HTTPS control page with a plain-HTTP robot.

## Team

Built by a 3-person team for *Robótica Industrial*, Universidad Metropolitana:
hardware assembly, firmware, sensor/actuator integration, systems integration
(WiFi API, voice control, audio), 3D-printed and cardboard chassis fabrication,
and finishing/paint.

## References

- [Espressif — Arduino-ESP32 core docs](https://docs.espressif.com/projects/arduino-esp32/)
- [Google AI for Developers — Gemini API](https://ai.google.dev/gemini-api/docs)
- [Netlify Docs](https://docs.netlify.com/)
- R2-D2 3D model base: [Teon, *R2D2 Star Wars* (remix), Thingiverse](https://www.thingiverse.com/thing:2791655/files)

## License

MIT — see [`LICENSE`](LICENSE).
