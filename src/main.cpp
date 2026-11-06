// Gravity Screen Flip
// Adds 180 degrees to the game's own camera angle while player 1 is upside down.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    struct State {
        float angle = 0.f;       // extra camera angle currently applied (degrees)
        float startAngle = 0.f;
        float targetAngle = 0.f;
        float elapsed = 0.f;
        bool  flipped = false;
        bool  loggedTick = false;
        bool  loggedCamera = false;
    };
    State g;

    float easeInOutCubic(float t) {
        return t < 0.5f ? 4.f * t * t * t
                        : 1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f;
    }
}

class $modify(GSFPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        g = State{};
        this->schedule(schedule_selector(GSFPlayLayer::gsfFrame));
        log::info("Gravity Screen Flip: attached to PlayLayer");
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        // Respawn: jump straight to the right orientation, no spin.
        if (m_player1) {
            bool enabled = Mod::get()->getSettingValue<bool>("enabled");
            g.flipped = m_player1->m_isUpsideDown;
            float a = (enabled && g.flipped) ? 180.f : 0.f;
            g.angle = g.startAngle = g.targetAngle = a;
            g.elapsed = 0.f;
        }
    }

    void gsfFrame(float) {
        if (!g.loggedTick) {
            g.loggedTick = true;
            log::info("Gravity Screen Flip: first frame tick running");
        }

        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            g.angle = g.startAngle = g.targetAngle = 0.f;
            if (m_player1) g.flipped = m_player1->m_isUpsideDown;
            return;
        }
        if (!m_player1) return;

        float duration = static_cast<float>(Mod::get()->getSettingValue<double>("duration"));
        bool continuous = Mod::get()->getSettingValue<bool>("continuous-spin");

        bool flipped = m_player1->m_isUpsideDown;
        if (flipped != g.flipped) {
            g.flipped = flipped;
            g.startAngle = g.angle;
            g.targetAngle = continuous ? g.targetAngle + 180.f
                                       : (flipped ? 180.f : 0.f);
            g.elapsed = 0.f;
            log::info("Gravity Screen Flip: gravity changed, upside down = {}", flipped);
        }

        if (g.angle != g.targetAngle) {
            if (duration <= 0.001f) {
                g.angle = g.targetAngle;
            } else {
                g.elapsed += CCDirector::get()->getDeltaTime();
                float t = std::min(g.elapsed / duration, 1.f);
                g.angle = g.startAngle + (g.targetAngle - g.startAngle) * easeInOutCubic(t);
                if (t >= 1.f) g.angle = g.targetAngle;
            }
        }
    }
};

class $modify(GSFBaseGameLayer, GJBaseGameLayer) {
    void updateCamera(float dt) {
        // Only act inside a real level, never in the editor.
        bool inLevel = static_cast<GJBaseGameLayer*>(PlayLayer::get())
                       == static_cast<GJBaseGameLayer*>(this);

        if (!inLevel || g.angle == 0.f) {
            GJBaseGameLayer::updateCamera(dt);
            return;
        }

        if (!g.loggedCamera) {
            g.loggedCamera = true;
            log::info("Gravity Screen Flip: applying camera angle {}", g.angle);
        }

        float original = m_gameState.m_cameraAngle;
        m_gameState.m_cameraAngle = original + g.angle;
        GJBaseGameLayer::updateCamera(dt);
        m_gameState.m_cameraAngle = original;
    }
};
