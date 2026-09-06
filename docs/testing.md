# Testing & Validation Log

Informal test log kept during the build. Documented here because most of the real
engineering happened in diagnosing these issues, not just in the initial wiring.

## Motor direction inversion

**Symptom**: after final assembly (motors + wiring sealed inside the 3D-printed/
cardboard chassis), all three motors ran in reverse relative to their commanded
direction — `adelante` (forward) drove the robot backward and vice versa; turns
were mirrored too.

**Diagnosis**: since *every* motor was inverted consistently (not just one), this
ruled out a single bad connection and pointed to a systemic polarity mismatch —
either from how the motors were wired to the drivers, or simply the "forward"
convention chosen originally not matching the final wheel orientation.

**Fix**: rather than reopening the sealed chassis to swap physical leads, the fix
was applied entirely in firmware — every `HIGH`/`LOW` pair for every motor, in every
command and mode (movement, animations, autonomous mode), was swapped. Verified
no pin pair is ever driven `HIGH`/`HIGH` simultaneously (which would short the
H-bridge half) before re-flashing.

**Lesson**: fixing direction in software after sealing the chassis is a legitimate
and common approach in embedded systems — it's cheaper and safer than re-opening a
glued enclosure, as long as the fix is applied *consistently* across every code path
that drives a motor, not just the primary movement commands.

## Single motor unresponsive in reverse only

**Symptom**: `adelante` drove all three wheels correctly; `atras` only moved one
rear wheel, and turning right stalled (turning left worked).

**Diagnosis**: since forward worked perfectly across all three motors, the firmware
logic was ruled out — `atras` is the exact electrical opposite of `adelante` in code,
and if the code were wrong, forward would have failed too. That narrowed it to a
physical connection that only breaks under reversed current flow — consistent with
a loose motor lead splice (the wheel motors needed lead extensions to reach the
driver board once mounted in the chassis arms) making poor contact in one polarity.

**Fix**: re-seating and re-taping the splice on the affected motor's leads.

**Lesson**: when a fault is direction-dependent but code-symmetric, suspect the
physical joint before the code — a loose splice can pass current one way (enough
resistance/contact for one polarity) and not the other.

## Battery-under-load erratic motor behavior

**Symptom**: motors that had tested fine began behaving inconsistently — sometimes
one wheel moved, sometimes two, sometimes none — while the control page kept
responding normally the whole time.

**Diagnosis**: the control page staying responsive (served over a separate USB power
source) while motor behavior degraded pointed away from WiFi/firmware and toward the
motor power path specifically. Confirmed as battery depletion after extended test
cycles — depleted AA cells can't supply enough current for all three motors
simultaneously under load, producing partial/random motor response.

**Fix**: fresh battery pack.

## HTTPS control page unable to command an HTTP robot (mixed content)

**Symptom**: the Netlify-hosted control page (served over HTTPS) could not reliably
send commands to the robot's onboard HTTP server, even though the robot's own local
IP page worked perfectly and the exact same command logic (with `mode:'no-cors'`)
had worked from Netlify earlier in development.

**Diagnosis**: browsers block "mixed content" — an HTTPS page making requests to a
plain-HTTP endpoint — by default. The `no-cors` fetch mode allows the *request* to
be sent without the browser reading the response, which is enough for fire-and-forget
commands, but browser-level mixed-content permissions can still block it outright,
and that permission is scoped per-site — deploying to a new Netlify subdomain during
iteration reset it each time.

**Fix (workaround used for the final demo)**: run the control page locally via
`python -m http.server` instead of over Netlify's HTTPS. A page served over plain
HTTP has no mixed-content restriction talking to another HTTP device on the same
network, which sidesteps the issue entirely. Chrome's per-site "insecure content"
permission override was also attempted but proved inconsistent across new deploys.

**Lesson**: for a browser-based controller talking to a local, plain-HTTP embedded
device, serving the controller itself over HTTP (e.g. locally) is more reliable than
trying to grant HTTPS→HTTP exceptions per deployment. A proper long-term fix would
be enabling HTTPS on the device side (e.g. via a self-signed cert or a local proxy).

## Ultrasonic sensor readings lost after chassis assembly

**Symptom**: distance readings stopped updating (`--`) after wiring was routed
inside the finished chassis, despite working correctly on the open breadboard.

**Diagnosis**: isolated to the physical `Echo`/`Trig` connections rather than
firmware, since the sensor had already been validated in code earlier in the build.

**Fix**: reseated the `Trig` (GPIO5) and `Echo` (GPIO18) breadboard connections in
different rows — the breadboard used had a number of unreliable contact points
discovered throughout the build (same root cause as the DFPlayer and front-motor
issues below).

## DFPlayer Mini never acknowledged (before being removed from the build)

**Symptom**: firmware reported `DFPlayer no respondió` on every boot, even after
verifying wiring, power, and RX/TX pin swaps in both orientations.

**Diagnosis**: the module's status LED confirmed it was powered, ruling out a supply
issue. With RX/TX verified in both orientations and pins moved to a different set of
GPIOs entirely (to rule out a bad breadboard row) with no change, the remaining
candidates were a bad breadboard contact elsewhere in the signal path or a missing
pull-up/series resistor on RX that some DFPlayer Mini clones require — neither of
which could be resolved without spare resistors on hand. This was ultimately
superseded by the decision to move audio playback client-side (see main README).

## Front motor initially unresponsive

**Symptom**: after wiring driver #2 for the added front motor, `IN2` had no effect —
traced to `GPIO14` specifically.

**Diagnosis**: swapping the affected wire to a different breadboard row fixed it
immediately, confirming a bad breadboard contact rather than a code or driver issue.

**Recurring theme**: several issues above trace back to unreliable breadboard
contacts rather than logic errors — a reminder to physically re-seat/move a
connection early when a component "should" work but doesn't, before assuming the
component or code is at fault.
