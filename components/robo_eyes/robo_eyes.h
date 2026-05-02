#pragma once
// =============================================================================
// robo_eyes.h — Animated robot eyes for ESPHome LVGL displays
//
// Draws two (or one, cyclops-mode) animated eyes directly on an LVGL canvas
// using LVGL 9 layer-based drawing APIs.  No Adafruit GFX dependency.
//
// Requires: ESPHome >= 2024.11 (LVGL 9 built-in), ESP32 (PSRAM recommended)
// =============================================================================

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include "lvgl.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#ifdef USE_ESP32
#include "esp_heap_caps.h"
#include "esp_random.h"
#endif

namespace esphome {
namespace robo_eyes {

// ── Mood: drives eyelid shape, blink speed, and pupil movement ───────────────
enum class Mood : uint8_t {
    DEFAULT  = 0,   // idle: slow wander, normal autoblink
    HAPPY    = 1,   // replying: bounce + fast blink, smile-cut bottom
    ANGRY    = 2,   // error: inner raised corner, no blink, H-flicker
    TIRED    = 3,   // thinking/not_ready: outer droopy corner, look up-left
    CURIOUS  = 4,   // listening: taller eyes, fast restless wander
    CLOSED   = 5,   // muted: eyes gradually close
    CONFUSED = 6,   // timer_finished: fast jitter + shake
};

// =============================================================================
class RoboEyes : public Component {
   public:
    // ── Setters (called by ESPHome codegen) ───────────────────────────────
    void set_eye_color(uint32_t c)        { eye_color_     = c; }
    void set_bg_color(uint32_t c)         { bg_color_      = c; }
    void set_eye_width(int v)             { eye_width_     = v; }
    void set_eye_height(int v)            { eye_height_    = v; }
    void set_border_radius(int v)         { border_radius_ = v; }
    void set_eye_gap(int v)               { eye_gap_       = v; }
    void set_space_between(int v)         { space_between_ = v; }
    void set_pupil_size(int v)            { pupil_size_    = v; }
    void set_show_pupils(bool v)          { show_pupils_   = v; }
    void set_cyclops(bool v)              { cyclops_       = v; }
    void set_blink_interval(uint32_t ms)  { blink_interval_ms_  = ms; }
    void set_blink_variation(uint32_t ms) { blink_variation_ms_ = ms; }
    void set_wander_interval(uint32_t ms) { wander_interval_ms_ = ms; }
    void set_fps(int v)                   { fps_ = v; }
    void set_display_width(int v)         { disp_w_ = v; }
    void set_display_height(int v)        { disp_h_ = v; }

    // Phase IDs — must match voice_assist_*_phase_id substitutions in YAML
    void set_phase_idle(int v)            { phase_idle_           = v; }
    void set_phase_listening(int v)       { phase_listening_      = v; }
    void set_phase_thinking(int v)        { phase_thinking_       = v; }
    void set_phase_replying(int v)        { phase_replying_       = v; }
    void set_phase_not_ready(int v)       { phase_not_ready_      = v; }
    void set_phase_error(int v)           { phase_error_          = v; }
    void set_phase_muted(int v)           { phase_muted_          = v; }
    void set_phase_timer_finished(int v)  { phase_timer_finished_ = v; }

    // ── Public API ────────────────────────────────────────────────────────
    /** Load the eyes LVGL screen. Safe to call from any script/automation. */
    void show();

    /** No-op; caller uses lvgl.page.show to navigate away. */
    void hide() {}

    /** Set mood directly (bypasses phase mapping). */
    void set_mood(Mood m);

    /**
     * Map a voice_assistant_phase integer → Mood and update animation.
     * Phase IDs are configured in YAML (defaults match va_intercom project).
     */
    void set_phase(int phase_id);

    // ── ESPHome lifecycle ─────────────────────────────────────────────────
    void setup() override;
    void loop() override {}

    // Run after LVGL is fully initialised
    float get_setup_priority() const override {
        return esphome::setup_priority::LATE - 1.0f;
    }

    // ── LVGL timer callback ───────────────────────────────────────────────
    static void s_tick(lv_timer_t *t);
    void tick();

   protected:
    // ── Configuration ─────────────────────────────────────────────────────
    uint32_t eye_color_         = 0x0099FF;
    uint32_t bg_color_          = 0x000000;
    int      eye_width_         = 62;
    int      eye_height_        = 46;
    int      border_radius_     = 16;
    int      eye_gap_           = 28;
    int      space_between_     = 0;
    int      pupil_size_        = 10;
    bool     show_pupils_       = true;
    bool     cyclops_           = false;
    uint32_t blink_interval_ms_ = 4000;
    uint32_t blink_variation_ms_= 2000;
    uint32_t wander_interval_ms_= 5000;
    int      fps_               = 25;
    int      disp_w_            = 240;
    int      disp_h_            = 240;

    // Phase ID mapping (configurable via YAML)
    int phase_idle_           = 1;
    int phase_listening_      = 2;
    int phase_thinking_       = 3;
    int phase_replying_       = 4;
    int phase_not_ready_      = 10;
    int phase_error_          = 11;
    int phase_muted_          = 12;
    int phase_timer_finished_ = 20;

    // ── LVGL objects ──────────────────────────────────────────────────────
    lv_obj_t   *screen_  = nullptr;
    lv_obj_t   *canvas_  = nullptr;
    lv_timer_t *timer_   = nullptr;
    void       *buf_     = nullptr;
    size_t      buf_size_= 0;

    // ── Animation state ───────────────────────────────────────────────────
    Mood     mood_          = Mood::DEFAULT;
    float    openness_      = 1.0f;    // 1.0 = fully open, 0.0 = closed
    uint8_t  blink_phase_   = 0;       // 0=waiting, 1=closing, 2=opening
    uint32_t next_blink_ms_ = 0;
    uint32_t next_wander_ms_= 0;

    float px_  = 0.0f, py_  = 0.0f;   // current pupil offset (normalised −1..1)
    float tpx_ = 0.0f, tpy_ = 0.0f;   // target pupil offset

    float    bounce_y_  = 0.0f;
    float    flicker_x_ = 0.0f;
    uint32_t frame_     = 0;

    // ── Helpers ───────────────────────────────────────────────────────────
    void  draw_frame_();
    void  draw_eye_(lv_layer_t *layer, float cx, float cy,
                    float ew, float vis_h, bool is_left);

    static inline float lerp_(float a, float b, float t) { return a + (b - a) * t; }
    static inline float clamp_(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
    static uint32_t rng_() {
#ifdef USE_ESP32
        return (uint32_t) esp_random();
#else
        return (uint32_t) rand();
#endif
    }
    inline uint32_t millis_() const { return (uint32_t) lv_tick_get(); }
};

// =============================================================================
// Action templates
// =============================================================================
template<typename... Ts>
class ShowAction : public Action<Ts...>, public Parented<RoboEyes> {
   public:
    void play(Ts... x) override { this->parent_->show(); }
};

template<typename... Ts>
class HideAction : public Action<Ts...>, public Parented<RoboEyes> {
   public:
    void play(Ts... x) override { this->parent_->hide(); }
};

template<typename... Ts>
class SetPhaseAction : public Action<Ts...>, public Parented<RoboEyes> {
   public:
    TEMPLATABLE_VALUE(int, phase)
    void play(Ts... x) override {
        this->parent_->set_phase(this->phase_.value(x...));
    }
};

template<typename... Ts>
class SetMoodAction : public Action<Ts...>, public Parented<RoboEyes> {
   public:
    void set_mood(Mood m) { mood_ = m; }
    void play(Ts... x)   override { this->parent_->set_mood(mood_); }
   protected:
    Mood mood_ = Mood::DEFAULT;
};

}  // namespace robo_eyes
}  // namespace esphome
