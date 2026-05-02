# robo_eyes — Animated robot eyes for ESPHome LVGL

Draws smoothly animated robot eyes on any ESP32 display using ESPHome's
built-in LVGL support.  Inspired by [FluxGarage/RoboEyes](https://github.com/FluxGarage/RoboEyes)
but implemented entirely with LVGL 9's canvas API — **no Adafruit GFX, no
OLED required**.

## Features

| Feature | Details |
|---|---|
| **All YAML, no C++ needed** | Every visual parameter exposed as a YAML key |
| **7 moods** | DEFAULT · HAPPY · ANGRY · TIRED · CURIOUS · CLOSED · CONFUSED |
| **Smooth blink** | Configurable interval + random variation |
| **Pupil wander** | Mood-driven gaze direction |
| **Eyelid shapes** | Triangle overlays for angry/tired; smile cut for happy |
| **Cyclops mode** | Single centred eye |
| **No-pupil mode** | `show_pupils: false` or `pupil_size: 0` |
| **PSRAM-friendly** | Buffer allocated in PSRAM automatically on ESP32 |
| **VA integration** | `set_phase(phase_id)` maps voice_assistant states to moods |

## Requirements

- ESPHome **≥ 2024.11** (LVGL 9 built-in)
- ESP32 (S3 recommended); PSRAM strongly recommended for 240×240+
- A display already configured with `lvgl:` in your YAML

## Installation

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/YOUR_USERNAME/robo_eyes_esphome
      ref: main
    components: [robo_eyes]
```

## Minimal config

```yaml
robo_eyes:
  id: eyes

# Show eyes (e.g. in a script or automation):
script:
  - id: my_script
    then:
      - robo_eyes.show:
          id: eyes
```

## Full configuration reference

```yaml
robo_eyes:
  id: robo_eyes_component          # required — use this ID in actions

  # ── Appearance ─────────────────────────────────────────────────────────
  eye_color:     0x0099FF          # hex RGB fill colour of the eyes
  bg_color:      0x000000          # background colour (match display bg)
  eye_width:     62                # eye width  in pixels
  eye_height:    46                # eye height in pixels (fully open)
  border_radius: 16                # corner radius (0 = sharp, high = oval)
  eye_gap:       28                # gap between the two eyes
  space_between: 0                 # extra spread (negative = closer)
  pupil_size:    10                # pupil radius in pixels
  show_pupils:   true              # false = no pupils
  cyclops:       false             # true = single centred eye

  # ── Animation ──────────────────────────────────────────────────────────
  blink_interval:  4s              # average time between blinks
  blink_variation: 2s              # random ± added to blink_interval
  wander_interval: 5s              # time between wander target changes
  fps:             25              # canvas redraw rate (5–60)

  # ── Display ────────────────────────────────────────────────────────────
  display_width:  240
  display_height: 240

  # ── Voice assistant phase IDs (defaults match ESPHome VA component) ────
  phase_idle_id:            1
  phase_listening_id:       2
  phase_thinking_id:        3
  phase_replying_id:        4
  phase_not_ready_id:      10
  phase_error_id:          11
  phase_muted_id:          12
  phase_timer_finished_id: 20
```

## Actions

| Action | Parameters | Description |
|---|---|---|
| `robo_eyes.show` | `id:` | Load eyes screen |
| `robo_eyes.hide` | `id:` | No-op; use `lvgl.page.show` to go back |
| `robo_eyes.set_phase` | `id:`, `phase: <int>` | Map VA phase → mood |
| `robo_eyes.set_mood` | `id:`, `mood: HAPPY` | Set mood directly |

### Available moods
`DEFAULT` · `HAPPY` · `ANGRY` · `TIRED` · `CURIOUS` · `CLOSED` · `CONFUSED`

## Mood → behaviour mapping

| Mood | Eye shape | Pupil movement | Extras |
|---|---|---|---|
| DEFAULT | Normal | Slow random wander | Auto-blink |
| CURIOUS | Taller | Fast restless wander | Fast blink |
| TIRED | Outer droopy corners | Gaze upper-left | Slow blink |
| HAPPY | Smile cut at bottom | Gentle sway | Bounce + fast blink |
| ANGRY | Inner raised corners | Centre | No blink + H-flicker |
| CLOSED | Gradually close | — | No blink |
| CONFUSED | Normal | Fast jitter | Fast blink + shake |

## Voice Assistant integration

See `example/robo_eyes_integration.yaml` for a complete drop-in integration
with the Xiaozhi Ball v3 / `va_intercom` project.

In short, replace the VA-mode section of your `draw_display` script:

```yaml
# instead of:  target = id(idle_page)->obj;  (etc.)
id(robo_eyes_component).set_phase(id(voice_assistant_phase));
id(robo_eyes_component).show();
```

## Repository structure

```
robo_eyes_esphome/
├── README.md
├── components/
│   └── robo_eyes/
│       ├── __init__.py       ESPHome schema + code generation
│       ├── robo_eyes.h       C++ class declaration
│       └── robo_eyes.cpp     Animation + LVGL drawing implementation
└── example/
    └── robo_eyes_integration.yaml   Integration guide for va_intercom YAML
```

## License

MIT
