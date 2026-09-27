// Leafwind -- gameplay constants (docs/DESIGN.md section 2).
#pragma once

namespace cfg {

// Units & scale (2.1)
constexpr float TILE = 0.5f;          // metres per tile
constexpr float PPM = 80.f;           // pixels per metre
constexpr float TILE_PX = TILE * PPM; // 40 px per tile
constexpr float DT = 1.f / 60.f;      // fixed timestep
constexpr int VEL_ITERS = 8;
constexpr int POS_ITERS = 3;
constexpr int MAX_STEPS_PER_FRAME = 4;
constexpr float VIEW_W = 800.f;
constexpr float VIEW_H = 600.f;

// Player collider
constexpr float PLAYER_W = 0.40f;
constexpr float PLAYER_H = 0.60f;
constexpr float PLAYER_R = 0.20f;
constexpr float PLAYER_MASS = 1.0f;

// Movement (2.2)
constexpr float RUN_SPEED = 4.5f;
constexpr float GROUND_ACCEL = 60.f;
constexpr float GROUND_DECEL = 80.f;
constexpr float TURN_BRAKE = 100.f;
constexpr float AIR_ACCEL = 30.f;
constexpr float AIR_DECEL = 10.f;
constexpr float GRAVITY = 25.f;
constexpr float FALL_GRAVITY_EXTRA = 8.75f;
constexpr float MAX_FALL = 14.f;
constexpr float JUMP_VY = -9.f;
constexpr float JUMP_CUT = 0.4f;
constexpr float JUMP_CUT_MIN_T = 0.06f;
constexpr float COYOTE = 0.10f;
constexpr float JUMP_BUFFER = 0.12f;
constexpr float HEAD_BUMP_VY = 0.5f;
constexpr float CORNER_CORRECT = 0.12f;
constexpr float HARD_LAND_VY = 8.f;

// Vines (2.4)
constexpr float ROPE_SEG_LEN = 0.25f;
constexpr float ROPE_SEG_W = 0.04f;
constexpr float ROPE_SEG_MASS = 0.03f;
constexpr float ROPE_PUMP_FORCE = 10.f;
constexpr float ROPE_CLIMB_INTERVAL = 0.12f;
constexpr float ROPE_COOLDOWN = 0.25f;
constexpr float ROPE_RELEASE_VX = 1.15f;
constexpr float ROPE_RELEASE_VY = -7.5f;

// Water (3.7)
constexpr float SWIM_GRAVITY_SCALE = 0.25f;
constexpr float SWIM_VY = 2.5f;
constexpr float SWIM_MAX_VX = 2.2f;
constexpr float SWIM_ACCEL = 20.f;
constexpr float SWIM_HOP_VY = -6.f;
constexpr float CURRENT_FORCE = 3.f;

// Mushroom (3.9)
constexpr float MUSHROOM_VY = -10.f;
constexpr float MUSHROOM_BIG_VY = -13.f;
constexpr float MUSHROOM_WINDOW = 0.08f;

// Wind (3.13)
constexpr float WIND_AIR = 6.f;
constexpr float WIND_GROUND = 2.f;
constexpr float UPDRAFT_ACCEL = 45.f;
constexpr float UPDRAFT_MAX_RISE = -4.f;

// Death (2.5)
constexpr float DEATH_ANIM = 0.5f;
constexpr float DEATH_FADE_START = 0.8f;
constexpr float DEATH_RESPAWN = 1.0f;

// Box2D integrates semi-implicitly (v += g*dt before x += v*dt), which shaves v*dt/2 off every
// apex. Launch velocities get half a gravity step added so heights match the design's analytic
// numbers (3.24-tile jump, 4 / 6.76-tile mushroom bounces...).
constexpr float launch(float vy) { return vy - GRAVITY * DT * 0.5f; }

} // namespace cfg
