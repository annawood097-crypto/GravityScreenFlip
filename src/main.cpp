// Gravity Screen Flip
// Rotates the whole gameplay view by 180 degrees while player 1 is upside down.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    constexpr float kPi = 3.14159265358979323846f;

    float easeInOutCubic(float t) {
        return t < 0.5f ? 4.f * t * t * t
                        : 1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f;
    }
}

class $modify(GSFPlayLayer, PlayLayer) {
    struct Fields {
        float angle = 0.f;
        float startAngle = 0.f;
        float targetAngle = 0.f;
        float elapsed = 0.f;
        bool  flipped = false;
        bool  dirty = false;
        bool  anchorWarned = false;
        bool  loggedFirst = false;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        // Our own per-frame callback, independent of any update hook.
        this->schedule(schedule_selector(GSFPlayLayer::gsfFrame));
        log::info("Gravity Screen Flip: attached to PlayLayer");
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        this->gsfSnap();
    }

    void gsfFrame(float) {
        this->gsfTick();
    }

    void gsfSnap() {
        auto f = m_fields.self();
        if (!m_player1) return;
        bool enabled = Mod::get()->getSettingValue<bool>("enabled");
        f->flipped = m_player1->m_isUpsideDown;
        float a = (enabled && f->flipped) ? 180.f : 0.f;
        f->angle = f->startAngle = f->targetAngle = a;
        f->elapsed = 0.f;
        this->gsfApply(a);
    }

    void gsfTick() {
        auto f = m_fields.self();

        if (!f->loggedFirst) {
            f->loggedFirst = true;
            log::info("Gravity Screen Flip: first frame tick running");
        }

        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            if (f->dirty) {
                f->angle = f->startAngle = f->targetAngle = 0.f;
                this->gsfApply(0.f);
            }
            if (m_player1) f->flipped = m_player1->m_isUpsideDown;
            return;
        }
        if (!m_player1) return;

        float duration = static_cast<float>(Mod::get()->getSettingValue<double>("duration"));
        bool continuous = Mod::get()->getSettingValue<bool>("continuous-spin");

        bool flipped = m_player1->m_isUpsideDown;
        if (flipped != f->flipped) {
            f->flipped = flipped;
            f->startAngle = f->angle;
            f->targetAngle = continuous ? f->targetAngle + 180.f
                                        : (flipped ? 180.f : 0.f);
            f->elapsed = 0.f;
            log::info("Gravity Screen Flip: gravity changed, upside down = {}", flipped);
        }

        if (f->angle != f->targetAngle) {
            if (duration <= 0.001f) {
                f->angle = f->targetAngle;
            } else {
                f->elapsed += CCDirector::get()->getDeltaTime();
                float t = std::min(f->elapsed / duration, 1.f);
                f->angle = f->startAngle + (f->targetAngle - f->startAngle) * easeInOutCubic(t);
                if (t >= 1.f) f->angle = f->targetAngle;
            }
        }

        if (f->angle != 0.f || f->dirty) {
            this->gsfApply(f->angle);
        }
    }

    void gsfApply(float angle) {
        auto f = m_fields.self();

        if (!this->isIgnoreAnchorPointForPosition()) {
            if (!f->anchorWarned) {
                f->anchorWarned = true;
                log::warn("Gravity Screen Flip: PlayLayer does not ignore its anchor point; skipping rotation");
            }
            return;
        }

        UILayer* ui = m_uiLayer;
        bool hud = Mod::get()->getSettingValue<bool>("upright-hud")
                   && ui && ui->getParent() == this;

        bool identity = std::fabs(std::remainder(angle, 360.f)) < 0.001f;
        if (identity) {
            this->setRotation(0.f);
            this->setScale(1.f);
            this->setPosition({0.f, 0.f});
            if (hud) {
                ui->setRotation(0.f);
                ui->setScale(1.f);
                ui->setPosition({0.f, 0.f});
            }
            f->dirty = false;
            return;
        }

        auto win = CCDirector::get()->getWinSize();
        float W = win.width, H = win.height;
        float cx = W / 2.f, cy = H / 2.f;

        float rad = angle * kPi / 180.f;
        float c = std::cos(rad), s = std::sin(rad);

        float scale = 1.f;
        if (Mod::get()->getSettingValue<bool>("cover-edges")) {
            float a = std::fmod(std::fabs(angle), 180.f) * kPi / 180.f;
            scale = std::fabs(std::cos(a)) + (W / H) * std::fabs(std::sin(a));
        }

        float px = cx - scale * (cx * c + cy * s);
        float py = cy - scale * (-cx * s + cy * c);

        this->setRotation(angle);
        this->setScale(scale);
        this->setPosition({px, py});

        if (hud) {
            float inv = 1.f / scale;
            ui->setRotation(-angle);
            ui->setScale(inv);
            ui->setPosition({-inv * (px * c - py * s), -inv * (px * s + py * c)});
        }
        f->dirty = true;
    }
};
