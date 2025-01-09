#pragma once

#include "zpl_wrap.h"

inline constexpr isize menu_background_stars_count = 256;
inline constexpr isize gameplay_background_stars_count = 128;

struct Input;
struct Renderer;

enum Game_State
{
    GAME_STATE_MENU,
    GAME_STATE_GAMEPLAY,
};

struct Bounding_Box
{
    zpl_aabb3 aabb;
    zpl_vec3 half_extent;
};

struct Boundary
{
    zpl_vec3 position;
    zpl_vec3 scale;
    Bounding_Box bounds;
    zpl_vec3 color;
    f32 glow;
};

struct Ball
{
    zpl_vec3 position;
    zpl_vec3 scale;
    zpl_vec2 velocity;
    Bounding_Box bounds;
    zpl_vec3 color;
    f32 glow;
};

struct Paddle
{
    zpl_vec3 position;
    zpl_vec3 scale;
    f32 y_target;
    Bounding_Box bounds;
    zpl_vec3 color;
    f32 glow;
};

struct Background_Star
{
    zpl_vec3 position;
    zpl_vec3 rotation;
    zpl_vec3 rotation_speed;
    f32 scale;
    zpl_vec3 color;
    f32 glow_min;
    f32 glow_max;
    f32 glow_speed;
    f32 glow;
    f32 max_lifetime;
    f32 lifetime;
};

struct Camera
{
    zpl_vec3 eye;
    zpl_vec3 center;
    zpl_vec3 up;
    f32 fov_rad;
    f32 aspect;
    f32 z_min;
    f32 z_max;
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
    zpl_random rand;
    Camera camera;
    Game_State current_state;
    Menu_State menu;
    Gameplay_State gameplay;
};

void game_init(Game *g, Input *inp, Renderer *r);
void game_input(Game *g);
void game_sim(Game *g, f64 total_duration, f64 sims_per_second);
void game_draw(Game *g);
