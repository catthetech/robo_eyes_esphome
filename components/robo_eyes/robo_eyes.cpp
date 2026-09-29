// =============================================================================
// robo_eyes.cpp — Animated robot eyes for ESPHome LVGL displays
//
// Drawing via LV_EVENT_DRAW_MAIN on a full-size lv_obj_t.
// No lv_canvas required. Fixes: lv_draw_triangle_dsc_t uses .color/.opa.
// =============================================================================
#include "robo_eyes.h"

namespace esphome {
namespace robo_eyes {

static const char *const TAG = "robo_eyes";

// =============================================================================
// setup()
// =============================================================================
void RoboEyes::setup() {
    ESP_LOGI(TAG, "RoboEyes setup: %dx%d fps=%d pupils=%s cyclops=%s",
             disp_w_, disp_h_, fps_,
             show_pupils_ ? "yes" : "no",
             cyclops_     ? "yes" : "no");

    // ── Dedicated LVGL screen ─────────────────────────────────────────────────
    screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(screen_, disp_w_, disp_h_);
    lv_obj_set_style_bg_color(screen_, lv_color_hex(bg_color_), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen_, 0, LV_PART_MAIN);

    // ── Full-size drawing object with custom draw callback ────────────────────
    // Using lv_obj instead of lv_canvas — works with ESPHome's default LVGL
    // build which does not enable LV_USE_CANVAS.
    draw_obj_ = lv_obj_create(screen_);
    lv_obj_set_size(draw_obj_, disp_w_, disp_h_);
    lv_obj_set_pos(draw_obj_, 0, 0);
    lv_obj_set_style_bg_color(draw_obj_, lv_color_hex(bg_color_), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(draw_obj_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(draw_obj_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(draw_obj_, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(draw_obj_, 0, LV_PART_MAIN);
    lv_obj_clear_flag(draw_obj_, LV_OBJ_FLAG_SCROLLABLE);

    // Register draw callback — called by LVGL whenever draw_obj_ is invalidated
    lv_obj_add_event_cb(draw_obj_, s_draw_cb, LV_EVENT_DRAW_MAIN, this);

    // ── Seed animation timers ─────────────────────────────────────────────────
    uint32_t now     = millis_();
    next_blink_ms_   = now + blink_interval_ms_;
    next_wander_ms_  = now + wander_interval_ms_;

    // ── LVGL animation timer ──────────────────────────────────────────────────
    uint32_t period = (fps_ > 0) ? (1000u / (uint32_t) fps_) : 40u;
    timer_ = lv_timer_create(s_tick, period, this);

    ESP_LOGI(TAG, "RoboEyes ready — period %u ms", (unsigned) period);
}

// =============================================================================
// Public API
// =============================================================================
void RoboEyes::show() {
    if (screen_) lv_scr_load(screen_);
}

void RoboEyes::set_mood(Mood m) {
    if (mood_ == m) return;
    ESP_LOGD(TAG, "mood %d -> %d", (int) mood_, (int) m);
    mood_ = m;
    tpx_  = 0.0f;
    tpy_  = 0.0f;
    if (m != Mood::CLOSED && m != Mood::ANGRY)
        blink_phase_ = 1;
}

void RoboEyes::set_phase(int phase_id) {
    Mood m;
    if      (phase_id == phase_idle_)           m = Mood::DEFAULT;
    else if (phase_id == phase_listening_)       m = Mood::CURIOUS;
    else if (phase_id == phase_thinking_)        m = Mood::TIRED;
    else if (phase_id == phase_replying_)        m = Mood::HAPPY;
    else if (phase_id == phase_not_ready_)       m = Mood::TIRED;
    else if (phase_id == phase_error_)           m = Mood::ANGRY;
    else if (phase_id == phase_muted_)           m = Mood::CLOSED;
    else if (phase_id == phase_timer_finished_)  m = Mood::CONFUSED;
    else                                         m = Mood::DEFAULT;
    set_mood(m);
}

// =============================================================================
// LVGL timer callback
// =============================================================================
void RoboEyes::s_tick(lv_timer_t *t) {
    auto *self = static_cast<RoboEyes *>(lv_timer_get_user_data(t));
    if (self) self->tick();
}

// =============================================================================
// LVGL draw callback — called during LVGL render cycle
// =============================================================================
void RoboEyes::s_draw_cb(lv_event_t *e) {
    auto *self  = static_cast<RoboEyes *>(lv_event_get_user_data(e));
    lv_layer_t *layer = lv_event_get_layer(e);
    if (self && layer) self->draw_frame_(layer);
}

// =============================================================================
// tick() — update animation state, then invalidate to trigger draw callback
// =============================================================================
void RoboEyes::tick() {
    if (!draw_obj_ || !screen_) return;
    if (lv_scr_act() != screen_) return;
    if (!enabled_) return;

    uint32_t now = millis_();
    frame_++;

    // ── Blink state machine ───────────────────────────────────────────────────
    if (mood_ == Mood::CLOSED) {
        openness_    = lerp_(openness_, 0.0f, 0.07f);
        blink_phase_ = 0;
    } else if (mood_ == Mood::ANGRY) {
        openness_    = lerp_(openness_, 1.0f, 0.12f);
        blink_phase_ = 0;
        next_blink_ms_ = now + blink_interval_ms_;
    } else if (blink_phase_ == 0) {
        openness_ = lerp_(openness_, 1.0f, 0.14f);
        if (now >= next_blink_ms_) {
            blink_phase_ = 1;
            uint32_t var = blink_variation_ms_ > 0 ? rng_() % blink_variation_ms_ : 0u;
            next_blink_ms_ = now + blink_interval_ms_ + var;
        }
    } else if (blink_phase_ == 1) {
        openness_ -= 0.22f;
        if (openness_ <= 0.01f) { openness_ = 0.0f; blink_phase_ = 2; }
    } else {
        openness_ += 0.16f;
        if (openness_ >= 0.98f) { openness_ = 1.0f; blink_phase_ = 0; }
    }

    // ── Pupil wander ──────────────────────────────────────────────────────────
    float wander_speed = 0.03f;
    switch (mood_) {
        case Mood::CURIOUS:
            wander_speed = 0.08f;
            if (now >= next_wander_ms_) {
                tpx_ = (float)((int)(rng_() % 200) - 100) / 100.0f * 0.75f;
                tpy_ = (float)((int)(rng_() % 200) - 100) / 100.0f * 0.50f;
                next_wander_ms_ = now + 400u + rng_() % 400u;
            }
            break;
        case Mood::TIRED:
            tpx_ = lerp_(tpx_, -0.55f, 0.025f);
            tpy_ = lerp_(tpy_, -0.45f, 0.025f);
            break;
        case Mood::HAPPY:
            tpx_ = sinf((float) frame_ * 0.07f) * 0.35f;
            tpy_ = 0.0f;
            break;
        case Mood::CONFUSED:
            wander_speed = 0.12f;
            if (now >= next_wander_ms_) {
                tpx_ = (float)((int)(rng_() % 200) - 100) / 100.0f * 0.60f;
                tpy_ = (float)((int)(rng_() % 200) - 100) / 100.0f * 0.30f;
                next_wander_ms_ = now + 120u + rng_() % 150u;
            }
            break;
        default:
            if (now >= next_wander_ms_) {
                tpx_ = (float)((int)(rng_() % 200) - 100) / 100.0f * 0.45f;
                tpy_ = (float)((int)(rng_() % 200) - 100) / 100.0f * 0.28f;
                next_wander_ms_ = now + wander_interval_ms_ + rng_() % 2000u;
            }
            break;
    }
    px_ = lerp_(px_, tpx_, wander_speed);
    py_ = lerp_(py_, tpy_, wander_speed);

    // ── Bounce (HAPPY) ────────────────────────────────────────────────────────
    bounce_y_ = (mood_ == Mood::HAPPY)
                ? sinf((float) frame_ * 0.17f) * 4.0f
                : lerp_(bounce_y_, 0.0f, 0.1f);

    // ── Flicker (ANGRY / CONFUSED) ────────────────────────────────────────────
    if (mood_ == Mood::ANGRY || mood_ == Mood::CONFUSED) {
        float amp = (mood_ == Mood::CONFUSED) ? 7.0f : 3.5f;
        flicker_x_ = (frame_ % 2 == 0)
                     ? ((float)((int)(rng_() % 100) - 50) / 50.0f) * amp
                     : 0.0f;
    } else {
        flicker_x_ = 0.0f;
    }

    // Update background colour and request redraw
    lv_obj_set_style_bg_color(draw_obj_, lv_color_hex(bg_color_), LV_PART_MAIN);
    lv_obj_invalidate(draw_obj_);
}

// =============================================================================
// draw_frame_() — called from s_draw_cb during LVGL render
// =============================================================================
void RoboEyes::draw_frame_(lv_layer_t *layer) {
    // Background fill
    {
        lv_draw_rect_dsc_t bg;
        lv_draw_rect_dsc_init(&bg);
        bg.bg_color     = lv_color_hex(bg_color_);
        bg.bg_opa       = LV_OPA_COVER;
        bg.radius       = 0;
        bg.border_width = 0;
        lv_area_t a = {0, 0, (lv_coord_t)(disp_w_ - 1), (lv_coord_t)(disp_h_ - 1)};
        lv_draw_rect(layer, &bg, &a);
    }

    const float cx    = (float) disp_w_ * 0.5f;
    const float cy    = (float) disp_h_ * 0.5f + bounce_y_;
    const float vis_h = (float) eye_height_ * clamp_(openness_, 0.0f, 1.0f);

    if (cyclops_) {
        draw_eye_(layer, cx + flicker_x_, cy, (float) eye_width_, vis_h, true);
    } else {
        const float total_w = (float)(eye_width_ * 2 + eye_gap_ + space_between_);
        const float lx = cx - total_w * 0.5f + (float) eye_width_ * 0.5f + flicker_x_;
        const float rx = cx + total_w * 0.5f - (float) eye_width_ * 0.5f + flicker_x_;
        draw_eye_(layer, lx, cy, (float) eye_width_, vis_h, true);
        draw_eye_(layer, rx, cy, (float) eye_width_, vis_h, false);
    }
}

// =============================================================================
// draw_eye_() — one eye body + mood overlay + optional pupil
// =============================================================================
void RoboEyes::draw_eye_(lv_layer_t *layer, float cx, float cy,
                          float ew, float vis_h, bool is_left) {
    if (vis_h < 1.0f) return;

    const float x = cx - ew * 0.5f;
    const float y = cy - vis_h * 0.5f;
    const float r_raw = (float) border_radius_ * (vis_h / (float) eye_height_);
    const float r = clamp_(r_raw, 0.0f, std::min(ew * 0.5f, vis_h * 0.5f));

    const lv_color_t eye_col = lv_color_hex(eye_color_);
    const lv_color_t bg_col  = lv_color_hex(bg_color_);

    // ── 1. Eye body ───────────────────────────────────────────────────────────
    {
        lv_draw_rect_dsc_t d;
        lv_draw_rect_dsc_init(&d);
        d.bg_color     = eye_col;
        d.bg_opa       = LV_OPA_COVER;
        d.radius       = (lv_coord_t) r;
        d.border_width = 0;
        lv_area_t a = {
            (lv_coord_t) x,
            (lv_coord_t) y,
            (lv_coord_t)(x + ew    - 1.0f),
            (lv_coord_t)(y + vis_h - 1.0f),
        };
        lv_draw_rect(layer, &d, &a);
    }

    // ── 2. Mood eyelid overlay ─────────────────────────────────────────────────
    if (openness_ > 0.32f) {
        switch (mood_) {

            case Mood::ANGRY: {
                // Inner top corner cut → angry brow
                // NOTE: lv_draw_triangle_dsc_t in LVGL 9 uses .color and .opa
                lv_draw_triangle_dsc_t td;
                lv_draw_triangle_dsc_init(&td);
                td.color = bg_col;
                td.opa   = LV_OPA_COVER;
                if (is_left) {
                    td.p[0] = {(lv_value_precise_t) cx,      (lv_value_precise_t) y};
                    td.p[1] = {(lv_value_precise_t)(x + ew), (lv_value_precise_t) y};
                    td.p[2] = {(lv_value_precise_t)(x + ew), (lv_value_precise_t)(y + vis_h * 0.44f)};
                } else {
                    td.p[0] = {(lv_value_precise_t) x,       (lv_value_precise_t) y};
                    td.p[1] = {(lv_value_precise_t) cx,      (lv_value_precise_t) y};
                    td.p[2] = {(lv_value_precise_t) x,       (lv_value_precise_t)(y + vis_h * 0.44f)};
                }
                lv_draw_triangle(layer, &td);
                break;
            }

            case Mood::TIRED: {
                // Outer top corner cut → droopy brow
                lv_draw_triangle_dsc_t td;
                lv_draw_triangle_dsc_init(&td);
                td.color = bg_col;
                td.opa   = LV_OPA_COVER;
                if (is_left) {
                    td.p[0] = {(lv_value_precise_t) x,       (lv_value_precise_t) y};
                    td.p[1] = {(lv_value_precise_t) cx,      (lv_value_precise_t) y};
                    td.p[2] = {(lv_value_precise_t) x,       (lv_value_precise_t)(y + vis_h * 0.44f)};
                } else {
                    td.p[0] = {(lv_value_precise_t) cx,      (lv_value_precise_t) y};
                    td.p[1] = {(lv_value_precise_t)(x + ew), (lv_value_precise_t) y};
                    td.p[2] = {(lv_value_precise_t)(x + ew), (lv_value_precise_t)(y + vis_h * 0.44f)};
                }
                lv_draw_triangle(layer, &td);
                break;
            }

            case Mood::HAPPY: {
                // Bg rect covers bottom 40% → smile arch
                lv_draw_rect_dsc_t rd;
                lv_draw_rect_dsc_init(&rd);
                rd.bg_color     = bg_col;
                rd.bg_opa       = LV_OPA_COVER;
                rd.radius       = 0;
                rd.border_width = 0;
                lv_area_t ra = {
                    (lv_coord_t) x,
                    (lv_coord_t)(y + vis_h * 0.60f),
                    (lv_coord_t)(x + ew - 1.0f),
                    (lv_coord_t)(y + vis_h + 6.0f),
                };
                lv_draw_rect(layer, &rd, &ra);
                break;
            }

            default:
                break;
        }
    }

    // ── 3. Pupil ───────────────────────────────────────────────────────────────
    if (show_pupils_ && pupil_size_ > 0 && openness_ > 0.14f) {
        const float pr          = (float) pupil_size_ * (vis_h / (float) eye_height_);
        const float max_px_off  = ew    * 0.5f - pr - 1.0f;
        const float max_py_off  = vis_h * 0.5f - pr - 1.0f;
        const float ox  = clamp_(px_ * (ew    * 0.22f), -max_px_off, max_px_off);
        const float oy  = clamp_(py_ * (vis_h * 0.22f), -max_py_off, max_py_off);
        const float pu_x = cx + ox;
        const float pu_y = clamp_(cy + oy, y + pr, y + vis_h - pr);

        lv_draw_rect_dsc_t pd;
        lv_draw_rect_dsc_init(&pd);
        pd.bg_color     = bg_col;
        pd.bg_opa       = LV_OPA_COVER;
        pd.radius       = LV_RADIUS_CIRCLE;
        pd.border_width = 0;
        lv_area_t pa = {
            (lv_coord_t)(pu_x - pr),
            (lv_coord_t)(pu_y - pr),
            (lv_coord_t)(pu_x + pr),
            (lv_coord_t)(pu_y + pr),
        };
        lv_draw_rect(layer, &pd, &pa);
    }
}

}  // namespace robo_eyes
}  // namespace esphome
