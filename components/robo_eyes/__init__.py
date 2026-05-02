"""
robo_eyes — Animated robot eyes for ESPHome LVGL displays
Requires: ESPHome >= 2024.11 (LVGL 9 built-in), ESP32 with PSRAM recommended
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.const import CONF_ID

CODEOWNERS = ["@yourusername"]
DEPENDENCIES = ["lvgl"]

robo_eyes_ns = cg.esphome_ns.namespace("robo_eyes")
RoboEyes = robo_eyes_ns.class_("RoboEyes", cg.Component)

# ── Mood enum (must mirror C++ enum class Mood) ───────────────────────────────
Mood = robo_eyes_ns.enum("Mood", is_class=True)
MOODS = {
    "DEFAULT":  Mood.DEFAULT,
    "HAPPY":    Mood.HAPPY,
    "ANGRY":    Mood.ANGRY,
    "TIRED":    Mood.TIRED,
    "CURIOUS":  Mood.CURIOUS,
    "CLOSED":   Mood.CLOSED,
    "CONFUSED": Mood.CONFUSED,
}

# ── Config keys ───────────────────────────────────────────────────────────────
CONF_EYE_COLOR            = "eye_color"
CONF_BG_COLOR             = "bg_color"
CONF_EYE_WIDTH            = "eye_width"
CONF_EYE_HEIGHT           = "eye_height"
CONF_BORDER_RADIUS        = "border_radius"
CONF_EYE_GAP              = "eye_gap"
CONF_SPACE_BETWEEN        = "space_between"
CONF_PUPIL_SIZE           = "pupil_size"
CONF_SHOW_PUPILS          = "show_pupils"
CONF_CYCLOPS              = "cyclops"
CONF_BLINK_INTERVAL       = "blink_interval"
CONF_BLINK_VARIATION      = "blink_variation"
CONF_WANDER_INTERVAL      = "wander_interval"
CONF_FPS                  = "fps"
CONF_DISPLAY_WIDTH        = "display_width"
CONF_DISPLAY_HEIGHT       = "display_height"
CONF_PHASE_IDLE           = "phase_idle_id"
CONF_PHASE_LISTENING      = "phase_listening_id"
CONF_PHASE_THINKING       = "phase_thinking_id"
CONF_PHASE_REPLYING       = "phase_replying_id"
CONF_PHASE_NOT_READY      = "phase_not_ready_id"
CONF_PHASE_ERROR          = "phase_error_id"
CONF_PHASE_MUTED          = "phase_muted_id"
CONF_PHASE_TIMER_FINISHED = "phase_timer_finished_id"

# ── Config schema ─────────────────────────────────────────────────────────────
# cv.int_ accepts 0xRRGGBB literals — YAML parses hex to int before validation.
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RoboEyes),
        # Appearance
        cv.Optional(CONF_EYE_COLOR,     default=0x0099FF): cv.int_,
        cv.Optional(CONF_BG_COLOR,      default=0x000000): cv.int_,
        cv.Optional(CONF_EYE_WIDTH,     default=62):  cv.int_range(min=10, max=120),
        cv.Optional(CONF_EYE_HEIGHT,    default=46):  cv.int_range(min=10, max=120),
        cv.Optional(CONF_BORDER_RADIUS, default=16):  cv.int_range(min=0,  max=60),
        cv.Optional(CONF_EYE_GAP,       default=28):  cv.int_range(min=0,  max=80),
        cv.Optional(CONF_SPACE_BETWEEN, default=0):   cv.int_range(min=-30, max=30),
        cv.Optional(CONF_PUPIL_SIZE,    default=10):  cv.int_range(min=0,  max=30),
        cv.Optional(CONF_SHOW_PUPILS,   default=True):  cv.boolean,
        cv.Optional(CONF_CYCLOPS,       default=False): cv.boolean,
        # Timing — positive_time_period_milliseconds returns int (ms)
        cv.Optional(CONF_BLINK_INTERVAL,  default="4s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_BLINK_VARIATION, default="2s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_WANDER_INTERVAL, default="5s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_FPS,             default=25):   cv.int_range(min=5, max=60),
        # Display
        cv.Optional(CONF_DISPLAY_WIDTH,  default=240): cv.int_range(min=64, max=800),
        cv.Optional(CONF_DISPLAY_HEIGHT, default=240): cv.int_range(min=64, max=800),
        # VA phase IDs — defaults match va_intercom substitutions
        cv.Optional(CONF_PHASE_IDLE,           default=1):  cv.int_,
        cv.Optional(CONF_PHASE_LISTENING,      default=2):  cv.int_,
        cv.Optional(CONF_PHASE_THINKING,       default=3):  cv.int_,
        cv.Optional(CONF_PHASE_REPLYING,       default=4):  cv.int_,
        cv.Optional(CONF_PHASE_NOT_READY,      default=10): cv.int_,
        cv.Optional(CONF_PHASE_ERROR,          default=11): cv.int_,
        cv.Optional(CONF_PHASE_MUTED,          default=12): cv.int_,
        cv.Optional(CONF_PHASE_TIMER_FINISHED, default=20): cv.int_,
    }
).extend(cv.COMPONENT_SCHEMA)

# ── Action class references ───────────────────────────────────────────────────
ShowAction     = robo_eyes_ns.class_("ShowAction",     automation.Action)
HideAction     = robo_eyes_ns.class_("HideAction",     automation.Action)
SetPhaseAction = robo_eyes_ns.class_("SetPhaseAction", automation.Action)
SetMoodAction  = robo_eyes_ns.class_("SetMoodAction",  automation.Action)

# ── robo_eyes.show ────────────────────────────────────────────────────────────
@automation.register_action(
    "robo_eyes.show",
    ShowAction,
    maybe_simple_id({cv.GenerateID(CONF_ID): cv.use_id(RoboEyes)}),
)
async def show_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var

# ── robo_eyes.hide ────────────────────────────────────────────────────────────
@automation.register_action(
    "robo_eyes.hide",
    HideAction,
    maybe_simple_id({cv.GenerateID(CONF_ID): cv.use_id(RoboEyes)}),
)
async def hide_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var

# ── robo_eyes.set_phase ───────────────────────────────────────────────────────
@automation.register_action(
    "robo_eyes.set_phase",
    SetPhaseAction,
    cv.Schema(
        {
            cv.GenerateID(CONF_ID): cv.use_id(RoboEyes),
            cv.Required("phase"): cv.templatable(cv.int_),
        }
    ),
)
async def set_phase_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config["phase"], args, int)
    cg.add(var.set_phase(templ))
    return var

# ── robo_eyes.set_mood ────────────────────────────────────────────────────────
@automation.register_action(
    "robo_eyes.set_mood",
    SetMoodAction,
    cv.Schema(
        {
            cv.GenerateID(CONF_ID): cv.use_id(RoboEyes),
            cv.Required("mood"): cv.enum(MOODS, upper=True),
        }
    ),
)
async def set_mood_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_mood(config["mood"]))
    return var

# ── Code generation ───────────────────────────────────────────────────────────
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_eye_color(config[CONF_EYE_COLOR]))
    cg.add(var.set_bg_color(config[CONF_BG_COLOR]))
    cg.add(var.set_eye_width(config[CONF_EYE_WIDTH]))
    cg.add(var.set_eye_height(config[CONF_EYE_HEIGHT]))
    cg.add(var.set_border_radius(config[CONF_BORDER_RADIUS]))
    cg.add(var.set_eye_gap(config[CONF_EYE_GAP]))
    cg.add(var.set_space_between(config[CONF_SPACE_BETWEEN]))
    cg.add(var.set_pupil_size(config[CONF_PUPIL_SIZE]))
    cg.add(var.set_show_pupils(config[CONF_SHOW_PUPILS]))
    cg.add(var.set_cyclops(config[CONF_CYCLOPS]))
    cg.add(var.set_blink_interval(config[CONF_BLINK_INTERVAL]))
    cg.add(var.set_blink_variation(config[CONF_BLINK_VARIATION]))
    cg.add(var.set_wander_interval(config[CONF_WANDER_INTERVAL]))
    cg.add(var.set_fps(config[CONF_FPS]))
    cg.add(var.set_display_width(config[CONF_DISPLAY_WIDTH]))
    cg.add(var.set_display_height(config[CONF_DISPLAY_HEIGHT]))
    cg.add(var.set_phase_idle(config[CONF_PHASE_IDLE]))
    cg.add(var.set_phase_listening(config[CONF_PHASE_LISTENING]))
    cg.add(var.set_phase_thinking(config[CONF_PHASE_THINKING]))
    cg.add(var.set_phase_replying(config[CONF_PHASE_REPLYING]))
    cg.add(var.set_phase_not_ready(config[CONF_PHASE_NOT_READY]))
    cg.add(var.set_phase_error(config[CONF_PHASE_ERROR]))
    cg.add(var.set_phase_muted(config[CONF_PHASE_MUTED]))
    cg.add(var.set_phase_timer_finished(config[CONF_PHASE_TIMER_FINISHED]))
