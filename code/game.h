#pragma once

#include "HandmadeMath.h"
#include "rnd.h"
#include <cstdint>

inline constexpr int menu_background_stars_count = 256;
inline constexpr int gameplay_background_stars_count = 128;

struct Input;
struct Renderer;

enum Game_State
{
    GAME_STATE_MENU,
    GAME_STATE_GAMEPLAY,
};

struct Bounding_Box
{
    HMM_Vec3 min;
    HMM_Vec3 max;
    HMM_Vec3 half_extent;
};

struct Boundary
{
    HMM_Vec3 position;
    HMM_Vec3 scale;
    Bounding_Box bounds;
    HMM_Vec3 color;
};

struct Ball
{
    HMM_Vec3 position;
    HMM_Vec3 scale;
    HMM_Vec2 velocity;
    Bounding_Box bounds;
    HMM_Vec3 color;
    float glow;
};

struct Paddle
{
    HMM_Vec3 position;
    HMM_Vec3 scale;
    float y_target;
    Bounding_Box bounds;
    HMM_Vec3 color;
    float glow;
};

struct Background_Star
{
    HMM_Vec3 position;
    HMM_Vec3 rotation;
    HMM_Vec3 rotation_speed;
    float scale;
    HMM_Vec3 color;
    float glow_min;
    float glow_max;
    float glow_speed;
    float glow;
    float max_lifetime;
    float lifetime;
};

struct Camera
{
    HMM_Vec3 eye;
    HMM_Vec3 center;
    HMM_Vec3 up;
    float fov_rad;
    float aspect;
    float z_min;
    float z_max;
};

struct Menu_State
{
    Ball ball;
    Background_Star background_stars[menu_background_stars_count];
};

struct Gameplay_State
{
    Boundary boundary_left;
    Boundary boundary_right;
    Boundary boundary_top;
    Boundary boundary_bottom;
    Ball ball;
    Paddle paddle_left;
    Paddle paddle_right;
    Background_Star background_stars[gameplay_background_stars_count];
};

struct Game
{
    Input *input;
    Renderer *renderer;
    rnd_gamerand_t rand;
    Camera camera;
    Game_State current_state;
    Menu_State menu;
    Gameplay_State gameplay;
};

void game_init(Game &g, Input &input, Renderer &renderer, uint32_t rand_seed);
void game_input(Game &g);
void game_sim(Game &g, float total_time_secs, float delta_time_secs);
void game_draw(const Game &g);
