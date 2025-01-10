#include "game.h"
#include "input.h"
#include "renderer.h"

enum Color
{
    COLOR_WHITE,
    COLOR_GREY,
    COLOR_RED,
    COLOR_WARM_GOLD,
    COLOR_COOL_BLUE,
    COLOR_VIBRANT_RED,
    COLOR_SOFT_PURPLE,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_ORANGE,
    COLOR_BRIGHT_PINK,
    COLOR_LIGHT_BLUE,
    COLOR_COUNT,
};

static constexpr HMM_Vec3 colors[COLOR_COUNT] = {
    {1.0f, 1.0f, 1.0f},     // COLOR_WHITE
    {0.6f, 0.599f, 0.598f}, // COLOR_GRAY
    {1.0f, 0.01f, 0.008f},  // COLOR_RED
    {1.0f, 0.85f, 0.3f},    // COLOR_WARM_GOLD
    {0.2f, 0.4f, 1.0f},     // COLOR_COOL_BLUE
    {1.0f, 0.2f, 0.2f},     // COLOR_VIBRANT_RED
    {0.6f, 0.2f, 0.8f},     // COLOR_SOFT_PURPLE
    {0.2f, 1.0f, 0.2f},     // COLOR_GREEN
    {0.2f, 1.0f, 1.0f},     // COLOR_CYAN
    {1.0f, 0.0f, 1.0f},     // COLOR_MAGENTA
    {1.0f, 0.5f, 0.0f},     // COLOR_ORANGE
    {1.0f, 0.5f, 1.0f},     // COLOR_BRIGHT_PINK
    {0.4f, 0.7f, 1.0f},     // COLOR_LIGHT_BLUE
};

static constexpr float ball_speed = 50.0f;
static constexpr float ball_max_speed = 100.0f;
static constexpr float paddle_speed = 30.0f;

// General functions.
static float pulsate(float time, float min, float max, float speed);
static float ease_in_out(float time);
static float rand_float(rnd_gamerand_t &rand);
static float rand_float(rnd_gamerand_t &rand, float min, float max);
static int rand_int(rnd_gamerand_t &rand, int min, int max);

// Bounding box functions.
static Bounding_Box bounding_box_entity_bounds(HMM_Vec3 position,
                                               HMM_Vec3 scale);
static Bounding_Box bounding_box_view_bounds_at_z(const Camera &c,
                                                  float z_dist);
static bool bounding_box_colliding(Bounding_Box a, Bounding_Box b);

// Background star functions.
static void background_star_reset(Background_Star &star, Game &g);
static void background_star_update(Background_Star &star, Game &g,
                                   float total_time, float delta_time);

static void menu_state_init(Game &g)
{
    auto &ball = g.menu.ball;
    ball.position = HMM_V3(0.0f, 0.0f, -10.0f);
    ball.scale = HMM_V3(0.66f, 0.66f, 0.66f);
    ball.color = colors[COLOR_LIGHT_BLUE];
    ball.glow = 10.0f;
    ball.bounds = bounding_box_entity_bounds(ball.position, ball.scale);
    ball.velocity.X =
        rand_float(g.rand, -ball_max_speed * 0.8f, ball_max_speed * 0.8f);
    ball.velocity.Y =
        rand_float(g.rand, -ball_max_speed * 0.5f, ball_max_speed * 0.5f);

    for (int i = 0; i < menu_background_stars_count; i += 1)
    {
        background_star_reset(g.menu.background_stars[i], g);
    }

    auto &point_light = g.renderer->game_pass.point_lights[0];
    point_light.position = g.menu.ball.position;
    point_light.diffuse_color = g.menu.ball.color * g.menu.ball.glow * 0.5f;
    point_light.ambient_color = HMM_V3(0.0f, 0.0f, 0.0f);
    point_light.falloff = 0.125f;
    point_light.radius = 10.0f;

    auto &dir_light = g.renderer->game_pass.dir_light;
    dir_light.direction = HMM_V3(-0.2f, 0.0f, -1.0f);
    dir_light.diffuse_color = HMM_V3(0.52f, 0.487f, 0.489f);
    dir_light.ambient_color = HMM_V3(0.005f, 0.004f, 0.004f);
}

static void gameplay_state_init(Game &g)
{
    auto view_bounds = bounding_box_view_bounds_at_z(g.camera, g.camera.eye.Z);

    auto &boundary_left = g.gameplay.boundary_left;
    boundary_left.position = HMM_V3(view_bounds.min.X + 6.0f, 0.0f, 0.0f);
    boundary_left.scale = HMM_V3(1.0f, g.camera.eye.Z * 0.25f + 5.0f, 1.0f);
    boundary_left.color = colors[COLOR_COOL_BLUE];
    boundary_left.bounds =
        bounding_box_entity_bounds(boundary_left.position, boundary_left.scale);

    auto &boundary_right = g.gameplay.boundary_right;
    boundary_right.position = HMM_V3(-boundary_left.position.X, 0.0f, 0.0f);
    boundary_right.scale = boundary_left.scale;
    boundary_right.color = colors[COLOR_WARM_GOLD];
    boundary_right.bounds = bounding_box_entity_bounds(boundary_right.position,
                                                       boundary_right.scale);

    auto &boundary_top = g.gameplay.boundary_top;
    boundary_top.position = HMM_V3(0.0f, boundary_left.scale.Y + 1.0f, 0.0f);
    boundary_top.scale = HMM_V3(view_bounds.max.X - 5.0f, 1.0f, 1.0f);
    boundary_top.color = colors[COLOR_GREY];
    boundary_top.bounds =
        bounding_box_entity_bounds(boundary_top.position, boundary_top.scale);

    auto &boundary_bottom = g.gameplay.boundary_bottom;
    boundary_bottom.position = HMM_V3(0.0f, -boundary_top.position.Y, 0.0f);
    boundary_bottom.scale = boundary_top.scale;
    boundary_bottom.color = colors[COLOR_GREY];
    boundary_bottom.bounds = bounding_box_entity_bounds(
        boundary_bottom.position, boundary_bottom.scale);

    auto &ball = g.gameplay.ball;
    ball.position = HMM_V3(0.0f, 0.0f, 0.0f);
    ball.scale = HMM_V3(0.66f, 0.66f, 0.66f);
    ball.color = colors[COLOR_LIGHT_BLUE];
    ball.glow = 10.0f;
    ball.bounds = bounding_box_entity_bounds(ball.position, ball.scale);
    // TODO: Get rid of this.
    ball.velocity.X = -50.0f;
    ball.velocity.Y = -4.0f;

    auto &paddle_left = g.gameplay.paddle_left;
    paddle_left.position =
        boundary_left.position +
        HMM_V3(boundary_left.bounds.half_extent.X + 2.0f, 0.0f, 0.0f);
    paddle_left.scale = HMM_V3(0.5f, 3.0f, 1.0f);
    paddle_left.color = colors[COLOR_COOL_BLUE];
    paddle_left.glow = 5.0f;
    paddle_left.bounds =
        bounding_box_entity_bounds(paddle_left.position, paddle_left.scale);

    auto &paddle_right = g.gameplay.paddle_right;
    paddle_right.position =
        boundary_right.position -
        HMM_V3(boundary_left.bounds.half_extent.X + 2.0f, 0.0f, 0.0f);
    paddle_right.scale = HMM_V3(0.5f, 3.0f, 1.0f);
    paddle_right.color = colors[COLOR_WARM_GOLD];
    paddle_right.glow = 5.0f;
    paddle_right.bounds =
        bounding_box_entity_bounds(paddle_right.position, paddle_right.scale);

    for (int i = 0; i < gameplay_background_stars_count; i += 1)
    {
        background_star_reset(g.gameplay.background_stars[i], g);
    }

    auto &point_light = g.renderer->game_pass.point_lights[0];
    point_light.position = ball.position;
    point_light.diffuse_color = ball.color * ball.glow * 0.5f;
    point_light.ambient_color = HMM_V3(0.0f, 0.0f, 0.0f);
    point_light.falloff = 0.125f;
    point_light.radius = 10.0f;

    auto &dir_light = g.renderer->game_pass.dir_light;
    dir_light.direction = HMM_V3(-0.03f, 0.3f, -1.0f);
    dir_light.diffuse_color = HMM_V3(1.0f, 1.0f, 1.0f);
    dir_light.ambient_color = HMM_V3(0.005f, 0.004f, 0.004f);
}

void game_init(Game &g, Input &input, Renderer &renderer, uint32_t rand_seed)
{
    g.input = &input;
    g.renderer = &renderer;

    rnd_gamerand_seed(&g.rand, rand_seed);

    g.camera.eye = HMM_V3(0.0f, 0.0f, 100.0f);
    g.camera.center = HMM_V3(0.0f, 0.0f, 0.0f);
    g.camera.up = HMM_V3(0.0f, 1.0f, 0.0f);
    g.camera.fov_rad = HMM_DegToRad * 40.0f;
    g.camera.aspect = static_cast<float>(g.renderer->framebuffer_width) /
                      static_cast<float>(g.renderer->framebuffer_height);
    g.camera.z_min = 0.1f;
    g.camera.z_max = 1000.0f;

    g.current_state = GAME_STATE_GAMEPLAY;
    gameplay_state_init(g);
}

static void intro_state_input(Game &g)
{
}

static void gameplay_state_input(Game &g)
{
    const auto &controller = g.input->controllers[0];
    if (controller.enabled)
    {
        if (input_controller_button_pressed(controller,
                                            INPUT_CONTROLLER_BUTTON_UP))
        {
            g.gameplay.paddle_left.y_target = 30.0f;
        }
        if (input_controller_button_pressed(controller,
                                            INPUT_CONTROLLER_BUTTON_DOWN))
        {
            g.gameplay.paddle_left.y_target = -30.0f;
        }
        if (input_controller_button_up(controller,
                                       INPUT_CONTROLLER_BUTTON_UP) &&
            input_controller_button_up(controller,
                                       INPUT_CONTROLLER_BUTTON_DOWN))
        {
            g.gameplay.paddle_left.y_target = 0.0f;
        }
    }
}

void game_input(Game &g)
{
    switch (g.current_state)
    {
    case GAME_STATE_MENU:
        intro_state_input(g);
        break;
    case GAME_STATE_GAMEPLAY:
        gameplay_state_input(g);
        break;
    }
}

static void ball_paddle_bounce(Ball &ball, const Paddle &paddle)
{
    float angle =
        (ball.position.Y - paddle.position.Y) / paddle.bounds.half_extent.Y;
    float abs_angle = HMM_ABS(angle);

    ball.velocity.Y = ball_speed * HMM_SinF(angle);
    ball.velocity.X = ball_speed * HMM_CosF(angle);

    if (abs_angle > 0.6f)
    {
        ball.velocity.Y *= (1.0f + abs_angle * 0.5f);
        ball.velocity.X *= (1.0f + abs_angle * 0.5f);
    }

    if (ball.position.X < paddle.position.X)
    {
        ball.velocity.X *= -1.0f;
    }
}

static void ball_move(Ball &ball, float delta_time)
{
    float ball_mag = HMM_LenV2(ball.velocity);
    if (ball_mag > ball_max_speed)
    {
        ball.velocity = HMM_NormV2(ball.velocity) * ball_max_speed;
    }
    ball.position.XY += ball.velocity * delta_time;
}

static void paddle_move(Paddle &paddle, float delta_time)
{
    float abs_target = HMM_ABS(paddle.y_target);
    float dir = abs_target / paddle.y_target;
    float movement = (abs_target > paddle_speed * delta_time)
                         ? paddle_speed * dir
                         : paddle.y_target;
    paddle.position.Y += movement * delta_time;
}

static void menu_state_sim(Game &g, float total_time, float delta_time)
{
    auto &ball = g.menu.ball;

    // Update.
    {
        ball_move(ball, delta_time);
        ball.bounds = bounding_box_entity_bounds(ball.position, ball.scale);

        g.camera.center = HMM_LerpV3(g.camera.center, delta_time * 0.8f,
                                     ball.position * 0.1f);

        for (int i = 0; i < menu_background_stars_count; i += 1)
        {
            background_star_update(g.menu.background_stars[i], g, total_time,
                                   delta_time);
        }
    }

    // Collision.
    {
        auto view_bounds = bounding_box_view_bounds_at_z(
            g.camera, g.camera.eye.Z + HMM_ABS(ball.position.Z));

        if (ball.bounds.min.X <= view_bounds.min.X ||
            ball.bounds.max.X >= view_bounds.max.X)
        {
            ball.velocity.X *= -1.0f;
        }
        if (ball.bounds.min.Y <= view_bounds.min.Y ||
            ball.bounds.max.Y >= view_bounds.max.Y)
        {
            ball.velocity.Y *= -1.0f;
        }
    }
}

static void gameplay_state_sim(Game &g, float total_time, float delta_time)
{
    auto &ball = g.gameplay.ball;
    auto &paddle_left = g.gameplay.paddle_left;
    auto &paddle_right = g.gameplay.paddle_right;

    // Update.
    {
        ball_move(ball, delta_time);
        ball.bounds = bounding_box_entity_bounds(ball.position, ball.scale);

        paddle_move(paddle_left, delta_time);
        paddle_left.bounds =
            bounding_box_entity_bounds(paddle_left.position, paddle_left.scale);

        paddle_move(paddle_right, delta_time);
        paddle_right.bounds = bounding_box_entity_bounds(paddle_right.position,
                                                         paddle_right.scale);

        g.camera.center = HMM_LerpV3(g.camera.center, delta_time * 0.8f,
                                     ball.position * 0.1f);

        for (int i = 0; i < gameplay_background_stars_count; i += 1)
        {
            background_star_update(g.gameplay.background_stars[i], g,
                                   total_time, delta_time);
        }
    }

    // Collision.
    {
        auto &boundary_left = g.gameplay.boundary_left;
        auto &boundary_right = g.gameplay.boundary_right;
        auto &boundary_top = g.gameplay.boundary_top;
        auto &boundary_bottom = g.gameplay.boundary_bottom;

        // Ball to left paddle.
        if (ball.velocity.X < 0.0f &&
            bounding_box_colliding(ball.bounds, paddle_left.bounds))
        {
            ball.position.X =
                paddle_left.bounds.max.X + ball.bounds.half_extent.X;
            ball_paddle_bounce(ball, paddle_left);
        }
        // Ball to right paddle.
        if (ball.velocity.X > 0.0f &&
            bounding_box_colliding(ball.bounds, paddle_right.bounds))
        {
            ball.position.X =
                paddle_right.bounds.min.X - ball.bounds.half_extent.X;
            ball_paddle_bounce(ball, paddle_right);
        }

        // Ball to left boundary.
        if (bounding_box_colliding(ball.bounds, boundary_left.bounds))
        {
            ball.position.X =
                boundary_left.bounds.max.X + ball.bounds.half_extent.X;
            ball.velocity.X *= -1.0f;
        }
        // Ball to right boundary.
        if (bounding_box_colliding(ball.bounds, boundary_right.bounds))
        {
            ball.position.X =
                boundary_right.bounds.min.X - ball.bounds.half_extent.X;
            ball.velocity.X *= -1.0f;
        }
        // Ball to top boundary.
        if (bounding_box_colliding(ball.bounds, boundary_top.bounds))
        {
            ball.position.Y =
                boundary_top.bounds.min.Y - ball.bounds.half_extent.Y;
            ball.velocity.Y *= -1.0f;
        }
        // Ball to bottom boundary.
        if (bounding_box_colliding(ball.bounds, boundary_bottom.bounds))
        {
            ball.position.Y =
                boundary_bottom.bounds.max.Y + ball.bounds.half_extent.Y;
            ball.velocity.Y *= -1.0f;
        }

        // Left paddle to top boundary.
        if (paddle_left.y_target > 0.0f &&
            bounding_box_colliding(paddle_left.bounds, boundary_top.bounds))
        {
            paddle_left.position.Y =
                boundary_top.bounds.min.Y - paddle_left.bounds.half_extent.Y;
            paddle_left.y_target = 0.0f;
        }
        // Left paddle to bottom boundary.
        if (paddle_left.y_target < 0.0f &&
            bounding_box_colliding(paddle_left.bounds, boundary_bottom.bounds))
        {
            paddle_left.position.Y =
                boundary_bottom.bounds.max.Y + paddle_left.bounds.half_extent.Y;
            paddle_left.y_target = 0.0f;
        }
    }
}

void game_sim(Game &g, float total_time_secs, float delta_time_secs)
{
    g.camera.aspect = static_cast<float>(g.renderer->framebuffer_width) /
                      static_cast<float>(g.renderer->framebuffer_height);

    switch (g.current_state)
    {
    case GAME_STATE_MENU:
        menu_state_sim(g, total_time_secs, delta_time_secs);
        break;
    case GAME_STATE_GAMEPLAY:
        gameplay_state_sim(g, total_time_secs, delta_time_secs);
        break;
    }
}

static void menu_state_draw(const Game &g)
{
    const auto &ball = g.menu.ball;

    auto &point_light = g.renderer->game_pass.point_lights[0];
    point_light.position = ball.position;
    point_light.diffuse_color = ball.color * ball.glow * 0.5f;

    g.renderer->game_pass.world_to_view_transform =
        HMM_LookAt_RH(g.camera.eye, g.camera.center, g.camera.up);
    g.renderer->game_pass.view_to_clip_transform = HMM_Perspective_RH_ZO(
        g.camera.fov_rad, g.camera.aspect, g.camera.z_min, g.camera.z_max);

    renderer_draw_basic_box_instance(*g.renderer, ball.position,
                                     HMM_V3(0.0f, 0.0f, 0.0f), ball.scale,
                                     ball.color * ball.glow);

    for (int i = 0; i < menu_background_stars_count; i += 1)
    {
        const auto &star = g.menu.background_stars[i];
        renderer_draw_basic_box_instance(
            *g.renderer, star.position, star.rotation,
            HMM_V3(star.scale, star.scale, star.scale), star.color * star.glow);
    }
}

static void gameplay_state_draw(const Game &g)
{
    const auto &ball = g.gameplay.ball;

    auto &point_light = g.renderer->game_pass.point_lights[0];
    point_light.position = ball.position;
    point_light.diffuse_color = ball.color * ball.glow * 0.5f;

    g.renderer->game_pass.world_to_view_transform =
        HMM_LookAt_RH(g.camera.eye, g.camera.center, g.camera.up);
    g.renderer->game_pass.view_to_clip_transform = HMM_Perspective_RH_ZO(
        g.camera.fov_rad, g.camera.aspect, g.camera.z_min, g.camera.z_max);

    auto draw_boundary = [&g](const Boundary &boundary)
    {
        renderer_draw_phong_box(*g.renderer, boundary.position, {},
                                boundary.scale, boundary.color);
    };
    auto draw_paddle = [&g](const Paddle &paddle)
    {
        renderer_draw_phong_box(*g.renderer, paddle.position, {}, paddle.scale,
                                paddle.color * paddle.glow);
    };

    draw_boundary(g.gameplay.boundary_left);
    draw_boundary(g.gameplay.boundary_right);
    draw_boundary(g.gameplay.boundary_bottom);
    draw_boundary(g.gameplay.boundary_top);

    draw_paddle(g.gameplay.paddle_left);
    draw_paddle(g.gameplay.paddle_right);

    renderer_draw_basic_box_instance(*g.renderer, ball.position, {}, ball.scale,
                                     ball.color * ball.glow);

    for (int i = 0; i < gameplay_background_stars_count; i += 1)
    {
        const auto &star = g.gameplay.background_stars[i];
        renderer_draw_basic_box_instance(
            *g.renderer, star.position, star.rotation,
            HMM_V3(star.scale, star.scale, star.scale), star.color * star.glow);
    }
}

void game_draw(const Game &g)
{
    switch (g.current_state)
    {
    case GAME_STATE_MENU:
        menu_state_draw(g);
        break;
    case GAME_STATE_GAMEPLAY:
        gameplay_state_draw(g);
        break;
    }
}

static float pulsate(float time, float min, float max, float speed)
{
    float pulse = (HMM_SinF(time * speed) + 1.0f) * 0.5f;
    return pulse * (max - min) + min;
}

static float ease_in_out(float time)
{
    if (time < 0.5f)
    {
        return 2.0f * time * time; // Ease in
    }
    return -1.0f + (4.0f - 2.0f * time) * time; // Ease out
}

static float rand_float(rnd_gamerand_t &rand)
{
    return rnd_gamerand_nextf(&rand);
}

static float rand_float(rnd_gamerand_t &rand, float min, float max)
{
    return min + rnd_gamerand_nextf(&rand) * (max - min);
}

static int rand_int(rnd_gamerand_t &rand, int min, int max)
{
    return rnd_gamerand_range(&rand, min, max);
}

static Bounding_Box bounding_box_entity_bounds(HMM_Vec3 position,
                                               HMM_Vec3 scale)
{
    Bounding_Box bounds;
    bounds.min = position - scale;
    bounds.max = position + scale;
    bounds.half_extent = (bounds.max - bounds.min) * 0.5f;
    return bounds;
}

static Bounding_Box bounding_box_view_bounds_at_z(const Camera &c, float z_dist)
{
    float visible_height = 2.0f * z_dist * HMM_TanF(c.fov_rad * 0.5f);
    float visible_width = visible_height * c.aspect;
    Bounding_Box bounds;
    bounds.min.X = -visible_width * 0.5f;
    bounds.max.X = visible_width * 0.5f;
    bounds.max.Y = visible_height * 0.5f;
    bounds.min.Y = -visible_height * 0.5f;
    bounds.half_extent = (bounds.max - bounds.min) * 0.5f;
    return bounds;
}

static bool bounding_box_colliding(Bounding_Box a, Bounding_Box b)
{
    return (a.max.X >= b.min.X && a.min.X <= b.max.X) &&
           (a.max.Y >= b.min.Y && a.min.Y <= b.max.Y) &&
           (a.max.Z >= b.min.Z && a.min.Z <= b.max.Z);
}

static void background_star_reset(Background_Star &star, Game &g)
{
    float z = rand_float(g.rand, g.camera.eye.Z * 2.0f, g.camera.z_max * 0.9f);
    auto view_bounds =
        bounding_box_view_bounds_at_z(g.camera, g.camera.eye.Z + z);

    star.position =
        HMM_V3(rand_float(g.rand, view_bounds.min.X, view_bounds.max.X),
               rand_float(g.rand, view_bounds.min.Y, view_bounds.max.Y), -z);

    star.rotation_speed =
        HMM_V3(rand_float(g.rand, 0.0f, 2.5f), rand_float(g.rand, 0.0f, 2.5f),
               rand_float(g.rand, 0.0f, 2.5f));

    star.scale = 0.0f;
    star.color = colors[rand_int(g.rand, 0, COLOR_COUNT - 1)];
    star.glow_min = rand_float(g.rand, 5.0f, 10.0f);
    star.glow_max = rand_float(g.rand, star.glow_min, 15.0f);
    star.glow_speed = rand_float(g.rand, 0.25f, 5.0f);
    star.glow = star.glow_min;
    star.max_lifetime = rand_float(g.rand, 2.0f, 10.0f);
    star.lifetime = 0.0f;
}

void background_star_update(Background_Star &star, Game &g, float total_time,
                            float delta_time)
{
    float progress = star.lifetime / star.max_lifetime;
    if (progress < 1.0f)
    {
        star.scale = ease_in_out(progress);
    }
    else
    {
        star.scale = ease_in_out(2.0f - progress);
    }

    star.lifetime += delta_time;
    if (star.lifetime >= star.max_lifetime * 2.0f)
    {
        background_star_reset(star, g);
    }

    star.rotation += star.rotation_speed * delta_time;
    star.glow =
        pulsate(total_time, star.glow_min, star.glow_max, star.glow_speed);
}
