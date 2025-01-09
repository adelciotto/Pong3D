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

static constexpr zpl_vec3 colors[] = {
    (zpl_vec3){1.0f, 1.0f, 1.0f},     // COLOR_WHITE
    (zpl_vec3){0.6f, 0.599f, 0.598f}, // COLOR_GRAY
    (zpl_vec3){1.0f, 0.01f, 0.008f},  // COLOR_RED
    (zpl_vec3){1.0f, 0.85f, 0.3f},    // COLOR_WARM_GOLD
    (zpl_vec3){0.2f, 0.4f, 1.0f},     // COLOR_COOL_BLUE
    (zpl_vec3){1.0f, 0.2f, 0.2f},     // COLOR_VIBRANT_RED
    (zpl_vec3){0.6f, 0.2f, 0.8f},     // COLOR_SOFT_PURPLE
    (zpl_vec3){0.2f, 1.0f, 0.2f},     // COLOR_GREEN
    (zpl_vec3){0.2f, 1.0f, 1.0f},     // COLOR_CYAN
    (zpl_vec3){1.0f, 0.0f, 1.0f},     // COLOR_MAGENTA
    (zpl_vec3){1.0f, 0.5f, 0.0f},     // COLOR_ORANGE
    (zpl_vec3){1.0f, 0.5f, 1.0f},     // COLOR_BRIGHT_PINK
    (zpl_vec3){0.4f, 0.7f, 1.0f},     // COLOR_LIGHT_BLUE
};

static constexpr f32 ball_speed = 50.0f;
static constexpr f32 ball_max_speed = 100.0f;
static constexpr f32 paddle_speed = 30.0f;

// General functions.
static f32 pulsate(f32 time, f32 min, f32 max, f32 speed);
static f32 ease_in_out(f32 time);
static Bounding_Box bounding_box_to_world_space(zpl_vec3 position,
                                                zpl_vec3 scale);
static Bounding_Box bounding_box_for_viewport(Camera *c, f32 z_dist);
static b8 bounding_box_colliding(Bounding_Box a, Bounding_Box b);

// Background star functions.
static void background_star_reset(Game *g, Background_Star *s);
static void background_star_update(Game *g, Background_Star *s, f32 total_time,
                                   f32 delta_time);

static void menu_state_init(Game *g)
{
    Menu_State *menu = &g->menu;

    menu->ball.position = zpl_vec3f(0.0f, 0.0f, -10.0f);
    menu->ball.scale = zpl_vec3f(0.66f, 0.66f, 0.66f);
    menu->ball.color = colors[COLOR_LIGHT_BLUE];
    menu->ball.glow = 10.0f;
    menu->ball.bounds =
        bounding_box_to_world_space(menu->ball.position, menu->ball.scale);
    menu->ball.velocity.x = zpl_random_range_f64(
        &g->rand, -ball_max_speed * 0.8f, ball_max_speed * 0.8f);
    menu->ball.velocity.y = zpl_random_range_f64(
        &g->rand, -ball_max_speed * 0.5f, ball_max_speed * 0.5f);

    for (isize i = 0; i < menu_background_stars_count; i += 1)
    {
        background_star_reset(g, &menu->background_stars[i]);
    }

    Point_Light *point_light = &g->renderer->game_pass.point_lights[0];
    point_light->position = menu->ball.position;
    point_light->diffuse_color = menu->ball.color * menu->ball.glow * 0.5f;
    point_light->ambient_color = zpl_vec3f_zero();
    point_light->falloff = 0.125f;
    point_light->radius = 10.0f;

    Directional_Light *dir_light = &g->renderer->game_pass.dir_light;
    dir_light->direction = zpl_vec3f(-0.2f, 0.0f, -1.0f);
    dir_light->diffuse_color = zpl_vec3f(0.52f, 0.487f, 0.489f);
    dir_light->ambient_color = zpl_vec3f(0.005f, 0.004f, 0.004f);
}

static void gameplay_state_init(Game *g)
{
    Gameplay_State *gameplay = &g->gameplay;

    Bounding_Box view_bounds =
        bounding_box_for_viewport(&g->camera, g->camera.eye.z);

    gameplay->boundary_left.position =
        zpl_vec3f(view_bounds.aabb.min.x + 6.0f, 0.0f, 0.0f);
    gameplay->boundary_left.scale =
        zpl_vec3f(1.0f, g->camera.eye.z * 0.25f + 5.0f, 1.0f);
    gameplay->boundary_left.color = colors[COLOR_RED];
    gameplay->boundary_left.glow = 5.0f;
    gameplay->boundary_left.bounds = bounding_box_to_world_space(
        gameplay->boundary_left.position, gameplay->boundary_left.scale);

    gameplay->boundary_right.position =
        zpl_vec3f(-gameplay->boundary_left.position.x, 0.0f, 0.0f);
    gameplay->boundary_right.scale = gameplay->boundary_left.scale;
    gameplay->boundary_right.color = colors[COLOR_RED];
    gameplay->boundary_right.glow = 5.0f;
    gameplay->boundary_right.bounds = bounding_box_to_world_space(
        gameplay->boundary_right.position, gameplay->boundary_right.scale);

    gameplay->boundary_top.position =
        zpl_vec3f(0.0f, gameplay->boundary_left.scale.y + 1.0f, 0.0f);
    gameplay->boundary_top.scale =
        zpl_vec3f(view_bounds.aabb.max.x - 5.0f, 1.0f, 1.0f);
    gameplay->boundary_top.color = colors[COLOR_GREY];
    gameplay->boundary_top.bounds = bounding_box_to_world_space(
        gameplay->boundary_top.position, gameplay->boundary_top.scale);

    gameplay->boundary_bottom.position =
        zpl_vec3f(0.0f, -gameplay->boundary_top.position.y, 0.0f);
    gameplay->boundary_bottom.scale = gameplay->boundary_top.scale;
    gameplay->boundary_bottom.color = colors[COLOR_GREY];
    gameplay->boundary_bottom.bounds = bounding_box_to_world_space(
        gameplay->boundary_bottom.position, gameplay->boundary_bottom.scale);

    gameplay->ball.position = zpl_vec3f(0.0f, 0.0f, 0.0f);
    gameplay->ball.scale = zpl_vec3f(0.66f, 0.66f, 0.66f);
    gameplay->ball.color = colors[COLOR_LIGHT_BLUE];
    gameplay->ball.glow = 10.0f;
    gameplay->ball.bounds = bounding_box_to_world_space(gameplay->ball.position,
                                                        gameplay->ball.scale);
    // TODO: Get rid of this.
    gameplay->ball.velocity.x = -50.0f;
    gameplay->ball.velocity.y = -4.0f;

    gameplay->paddle_left.position =
        gameplay->boundary_left.position +
        zpl_vec3f(gameplay->boundary_left.bounds.half_extent.x + 2.0f, 0.0f,
                  0.0f);
    gameplay->paddle_left.scale = zpl_vec3f(0.5f, 3.0f, 1.0f);
    gameplay->paddle_left.color = colors[COLOR_COOL_BLUE];
    gameplay->paddle_left.glow = 5.0f;
    gameplay->paddle_left.bounds = bounding_box_to_world_space(
        gameplay->paddle_left.position, gameplay->paddle_left.scale);

    gameplay->paddle_right.position =
        gameplay->boundary_right.position -
        zpl_vec3f(gameplay->boundary_left.bounds.half_extent.x + 2.0f, 0.0f,
                  0.0f);
    gameplay->paddle_right.scale = zpl_vec3f(0.5f, 3.0f, 1.0f);
    gameplay->paddle_right.color = colors[COLOR_COOL_BLUE];
    gameplay->paddle_right.glow = 5.0f;
    gameplay->paddle_right.bounds = bounding_box_to_world_space(
        gameplay->paddle_right.position, gameplay->paddle_right.scale);

    for (isize i = 0; i < gameplay_background_stars_count; i += 1)
    {
        background_star_reset(g, &gameplay->background_stars[i]);
    }

    Point_Light *point_light = &g->renderer->game_pass.point_lights[0];
    point_light->position = gameplay->ball.position;
    point_light->diffuse_color =
        gameplay->ball.color * gameplay->ball.glow * 0.5f;
    point_light->ambient_color = zpl_vec3f_zero();
    point_light->falloff = 0.125f;
    point_light->radius = 10.0f;

    Directional_Light *dir_light = &g->renderer->game_pass.dir_light;
    dir_light->direction = zpl_vec3f(-0.03f, 0.3f, -1.0f);
    dir_light->diffuse_color = zpl_vec3f(1.0f, 1.0f, 1.0f);
    dir_light->ambient_color = zpl_vec3f(0.005f, 0.004f, 0.004f);
}

void game_init(Game *g, Input *inp, Renderer *r)
{
    ZPL_ASSERT(inp != nullptr);
    ZPL_ASSERT(r != nullptr);

    g->renderer = r;
    g->input = inp;

    zpl_random_init(&g->rand);

    g->camera.eye = zpl_vec3f(0.0f, 0.0f, 100.0f);
    g->camera.center = zpl_vec3f(0.0f, 0.0f, 0.0f);
    g->camera.up = zpl_vec3f(0.0f, 1.0f, 0.0f);
    g->camera.fov_rad = zpl_to_radians(40.0f);
    g->camera.aspect = (f32)g->renderer->framebuffer_width /
                       (f32)g->renderer->framebuffer_height;
    g->camera.z_min = 0.1f;
    g->camera.z_max = 1000.0f;

    g->current_state = GAME_STATE_GAMEPLAY;
    gameplay_state_init(g);
}

static void intro_state_input(Game *g)
{
}

static void gameplay_state_input(Game *g)
{
    Gameplay_State *gameplay = &g->gameplay;

    Input_Controller *c0 = &g->input->controllers[INPUT_CONTROLLER_INDEX_0];
    if (c0->enabled)
    {
        if (input_controller_button_pressed(c0, INPUT_CONTROLLER_BUTTON_UP))
        {
            gameplay->paddle_left.y_target = 30.0f;
        }
        if (input_controller_button_pressed(c0, INPUT_CONTROLLER_BUTTON_DOWN))
        {
            gameplay->paddle_left.y_target = -30.0f;
        }
        if (input_controller_button_up(c0, INPUT_CONTROLLER_BUTTON_UP) &&
            input_controller_button_up(c0, INPUT_CONTROLLER_BUTTON_DOWN))
        {
            gameplay->paddle_left.y_target = 0.0f;
        }
    }
}

void game_input(Game *g)
{
    switch (g->current_state)
    {
    case GAME_STATE_MENU:
        intro_state_input(g);
        break;
    case GAME_STATE_GAMEPLAY:
        gameplay_state_input(g);
        break;
    }
}

static void ball_paddle_bounce(Ball *b, Paddle *p)
{
    f32 angle = (b->position.y - p->position.y) / p->bounds.half_extent.y;
    f32 abs_angle = zpl_abs(angle);

    b->velocity.y = ball_speed * zpl_sin(angle);
    b->velocity.x = ball_speed * zpl_cos(angle);

    if (abs_angle > 0.6)
    {
        b->velocity.y *= (1.0f + abs_angle * 0.5f);
        b->velocity.x *= (1.0f + abs_angle * 0.5f);
    }

    if (b->position.x < p->position.x)
    {
        b->velocity.x *= -1.0f;
    }
}

static void ball_move(Ball *b, f32 delta_time)
{
    f32 ball_mag = zpl_vec2_mag(b->velocity);
    if (ball_mag > ball_max_speed)
    {
        zpl_vec2 n;
        zpl_vec2_norm(&n, b->velocity);
        b->velocity = n * ball_max_speed;
    }

    b->position.xy += b->velocity * delta_time;
}

static void paddle_move(Paddle *p, f32 delta_time)
{
    f32 abs_target = zpl_abs(p->y_target);
    f32 dir = abs_target / p->y_target;
    f32 movement = (abs_target > paddle_speed * delta_time) ? paddle_speed * dir
                                                            : p->y_target;
    p->position.y += movement * delta_time;
}

static void menu_state_sim(Game *g, f32 total_time, f32 delta_time)
{
    Menu_State *menu = &g->menu;

    // Update.
    {
        ball_move(&menu->ball, delta_time);

        zpl_vec3_lerp(&g->camera.center, g->camera.center,
                      menu->ball.position * 0.1f, delta_time * 0.8f);

        for (isize i = 0; i < menu_background_stars_count; i += 1)
        {
            background_star_update(g, &menu->background_stars[i], total_time,
                                   delta_time);
        }

        menu->ball.bounds =
            bounding_box_to_world_space(menu->ball.position, menu->ball.scale);
    }

    // Collision.
    {
        Bounding_Box view_bounds = bounding_box_for_viewport(
            &g->camera, g->camera.eye.z + zpl_abs(menu->ball.position.z));

        if (menu->ball.bounds.aabb.min.x <= view_bounds.aabb.min.x ||
            menu->ball.bounds.aabb.max.x >= view_bounds.aabb.max.x)
        {
            menu->ball.velocity.x *= -1.0f;
        }
        if (menu->ball.bounds.aabb.min.y <= view_bounds.aabb.min.y ||
            menu->ball.bounds.aabb.max.y >= view_bounds.aabb.max.y)
        {
            menu->ball.velocity.y *= -1.0f;
        }
    }
}

static void gameplay_state_sim(Game *g, f32 total_time, f32 delta_time)
{
    Gameplay_State *gameplay = &g->gameplay;

    // Update.
    {
        ball_move(&gameplay->ball, delta_time);
        paddle_move(&gameplay->paddle_left, delta_time);
        paddle_move(&gameplay->paddle_right, delta_time);

        zpl_vec3_lerp(&g->camera.center, g->camera.center,
                      gameplay->ball.position * 0.1f, delta_time * 0.8f);

        for (isize i = 0; i < gameplay_background_stars_count; i += 1)
        {
            background_star_update(g, &gameplay->background_stars[i],
                                   total_time, delta_time);
        }

        gameplay->ball.bounds = bounding_box_to_world_space(
            gameplay->ball.position, gameplay->ball.scale);
        gameplay->paddle_left.bounds = bounding_box_to_world_space(
            gameplay->paddle_left.position, gameplay->paddle_left.scale);
        gameplay->paddle_right.bounds = bounding_box_to_world_space(
            gameplay->paddle_right.position, gameplay->paddle_right.scale);
    }

    // Collision.
    {
        // Ball to left paddle.
        if (gameplay->ball.velocity.x < 0.0f &&
            bounding_box_colliding(gameplay->ball.bounds,
                                   gameplay->paddle_left.bounds))
        {
            gameplay->ball.position.x =
                gameplay->paddle_left.bounds.aabb.max.x +
                gameplay->ball.bounds.half_extent.x;
            ball_paddle_bounce(&gameplay->ball, &gameplay->paddle_left);
        }
        // Ball to right paddle.
        if (gameplay->ball.velocity.x > 0.0f &&
            bounding_box_colliding(gameplay->ball.bounds,
                                   gameplay->paddle_right.bounds))
        {
            gameplay->ball.position.x =
                gameplay->paddle_right.bounds.aabb.min.x -
                gameplay->ball.bounds.half_extent.x;
            ball_paddle_bounce(&gameplay->ball, &gameplay->paddle_right);
        }

        // Ball to left boundary.
        if (bounding_box_colliding(gameplay->ball.bounds,
                                   gameplay->boundary_left.bounds))
        {
            gameplay->ball.position.x =
                gameplay->boundary_left.bounds.aabb.max.x +
                gameplay->ball.bounds.half_extent.x;
            gameplay->ball.velocity.x *= -1.0f;
        }
        // Ball to right boundary.
        if (bounding_box_colliding(gameplay->ball.bounds,
                                   gameplay->boundary_right.bounds))
        {
            gameplay->ball.position.x =
                gameplay->boundary_right.bounds.aabb.min.x -
                gameplay->ball.bounds.half_extent.x;
            gameplay->ball.velocity.x *= -1.0f;
        }
        // Ball to top boundary.
        if (bounding_box_colliding(gameplay->ball.bounds,
                                   gameplay->boundary_top.bounds))
        {
            gameplay->ball.position.y =
                gameplay->boundary_top.bounds.aabb.min.y -
                gameplay->ball.bounds.half_extent.y;
            gameplay->ball.velocity.y *= -1.0f;
        }
        // Ball to bottom boundary.
        if (bounding_box_colliding(gameplay->ball.bounds,
                                   gameplay->boundary_bottom.bounds))
        {
            gameplay->ball.position.y =
                gameplay->boundary_bottom.bounds.aabb.max.y +
                gameplay->ball.bounds.half_extent.y;
            gameplay->ball.velocity.y *= -1.0f;
        }

        // Left paddle to top boundary.
        if (gameplay->paddle_left.y_target > 0.0f &&
            bounding_box_colliding(gameplay->paddle_left.bounds,
                                   gameplay->boundary_top.bounds))
        {
            gameplay->paddle_left.position.y =
                gameplay->boundary_top.bounds.aabb.min.y -
                gameplay->paddle_left.bounds.half_extent.y;
            gameplay->paddle_left.y_target = 0.0f;
        }
        // Left paddle to bottom boundary.
        if (gameplay->paddle_left.y_target < 0.0f &&
            bounding_box_colliding(gameplay->paddle_left.bounds,
                                   gameplay->boundary_bottom.bounds))
        {
            gameplay->paddle_left.position.y =
                gameplay->boundary_bottom.bounds.aabb.max.y +
                gameplay->paddle_left.bounds.half_extent.y;
            gameplay->paddle_left.y_target = 0.0f;
        }
    }
}

void game_sim(Game *g, f64 total_duration, f64 sims_per_second)
{
    f32 total_time = (f32)total_duration;
    f32 delta_time = (f32)sims_per_second;

    g->camera.aspect = (f32)g->renderer->framebuffer_width /
                       (f32)g->renderer->framebuffer_height;

    switch (g->current_state)
    {
    case GAME_STATE_MENU:
        menu_state_sim(g, total_time, delta_time);
        break;
    case GAME_STATE_GAMEPLAY:
        gameplay_state_sim(g, total_time, delta_time);
        break;
    }
}

static void menu_state_draw(Game *g)
{
    Menu_State *menu = &g->menu;

    Renderer *r = g->renderer;
    Point_Light *pl = &r->game_pass.point_lights[0];
    pl->position = menu->ball.position;
    pl->diffuse_color = menu->ball.color * menu->ball.glow * 0.5f;

    zpl_mat4_perspective(&r->game_pass.view_to_clip_transform,
                         g->camera.fov_rad, g->camera.aspect, g->camera.z_min,
                         g->camera.z_max);
    zpl_mat4_look_at(&r->game_pass.world_to_view_transform, g->camera.eye,
                     g->camera.center, g->camera.up);

    renderer_draw_basic_box_instance(r, menu->ball.position, zpl_vec3f_zero(),
                                     menu->ball.scale,
                                     menu->ball.color * menu->ball.glow);

    for (isize i = 0; i < menu_background_stars_count; i += 1)
    {
        Background_Star *s = &menu->background_stars[i];
        renderer_draw_basic_box_instance(
            r, s->position, s->rotation,
            zpl_vec3f(s->scale, s->scale, s->scale), s->color * s->glow);
    }
}

static void gameplay_state_draw(Game *g)
{
    Gameplay_State *gameplay = &g->gameplay;

    // Update direct renderer state.
    Renderer *r = g->renderer;
    r->game_pass.point_lights[0].position = gameplay->ball.position;
    r->game_pass.point_lights[0].diffuse_color =
        gameplay->ball.color * gameplay->ball.glow * 0.5f;

    zpl_mat4_perspective(&r->game_pass.view_to_clip_transform,
                         g->camera.fov_rad, g->camera.aspect, g->camera.z_min,
                         g->camera.z_max);
    zpl_mat4_look_at(&r->game_pass.world_to_view_transform, g->camera.eye,
                     g->camera.center, g->camera.up);

    renderer_draw_basic_box_instance(
        r, gameplay->boundary_left.position, zpl_vec3f_zero(),
        gameplay->boundary_left.scale,
        gameplay->boundary_left.color * gameplay->boundary_left.glow);
    renderer_draw_basic_box_instance(
        r, gameplay->boundary_right.position, zpl_vec3f_zero(),
        gameplay->boundary_right.scale,
        gameplay->boundary_right.color * gameplay->boundary_right.glow);

    renderer_draw_phong_box(r, gameplay->boundary_top.position,
                            zpl_vec3f_zero(), gameplay->boundary_top.scale,
                            gameplay->boundary_top.color);
    renderer_draw_phong_box(r, gameplay->boundary_bottom.position,
                            zpl_vec3f_zero(), gameplay->boundary_bottom.scale,
                            gameplay->boundary_bottom.color);

    renderer_draw_phong_box(r, gameplay->paddle_left.position, zpl_vec3f_zero(),
                            gameplay->paddle_left.scale,
                            gameplay->paddle_left.color *
                                gameplay->paddle_left.glow);
    renderer_draw_phong_box(r, gameplay->paddle_right.position,
                            zpl_vec3f_zero(), gameplay->paddle_right.scale,
                            gameplay->paddle_right.color *
                                gameplay->paddle_right.glow);

    renderer_draw_basic_box_instance(
        r, gameplay->ball.position, zpl_vec3f_zero(), gameplay->ball.scale,
        gameplay->ball.color * gameplay->ball.glow);

    for (isize i = 0; i < gameplay_background_stars_count; i += 1)
    {
        Background_Star *s = &gameplay->background_stars[i];
        renderer_draw_basic_box_instance(
            r, s->position, s->rotation,
            zpl_vec3f(s->scale, s->scale, s->scale), s->color * s->glow);
    }
}

void game_draw(Game *g)
{
    switch (g->current_state)
    {
    case GAME_STATE_MENU:
        menu_state_draw(g);
        break;
    case GAME_STATE_GAMEPLAY:
        gameplay_state_draw(g);
        break;
    }
}

static f32 pulsate(f32 time, f32 min, f32 max, f32 speed)
{
    f32 pulse = (zpl_sin(time * speed) + 1.0f) * 0.5f;
    return pulse * (max - min) + min;
}

static f32 ease_in_out(f32 time)
{
    if (time < 0.5f)
    {
        return 2.0f * time * time; // Ease in
    }
    return -1.0f + (4.0f - 2.0f * time) * time; // Ease out
}

static Bounding_Box bounding_box_to_world_space(zpl_vec3 position,
                                                zpl_vec3 scale)
{
    Bounding_Box bb;
    bb.aabb.min = position - scale;
    bb.aabb.max = position + scale;
    bb.half_extent = (bb.aabb.max - bb.aabb.min) * 0.5f;
    return bb;
}

static Bounding_Box bounding_box_for_viewport(Camera *c, f32 z_dist)
{
    f32 visible_height = 2.0f * z_dist * zpl_tan(c->fov_rad * 0.5f);
    f32 visible_width = visible_height * c->aspect;
    Bounding_Box bb;
    bb.aabb.min.x = -visible_width * 0.5f;
    bb.aabb.max.x = visible_width * 0.5f;
    bb.aabb.max.y = visible_height * 0.5f;
    bb.aabb.min.y = -visible_height * 0.5f;
    bb.half_extent = (bb.aabb.max - bb.aabb.min) * 0.5f;
    return bb;
}

static b8 bounding_box_colliding(Bounding_Box a, Bounding_Box b)
{
    return (a.aabb.max.x >= b.aabb.min.x && a.aabb.min.x <= b.aabb.max.x) &&
           (a.aabb.max.y >= b.aabb.min.y && a.aabb.min.y <= b.aabb.max.y) &&
           (a.aabb.max.z >= b.aabb.min.z && a.aabb.min.z <= b.aabb.max.z);
}

static void background_star_reset(Game *g, Background_Star *s)
{
    f32 z = zpl_random_range_f64(&g->rand, g->camera.eye.z * 2.0f,
                                 g->camera.z_max * 0.9f);

    Bounding_Box view_bounds =
        bounding_box_for_viewport(&g->camera, g->camera.eye.z + z);
    s->position =
        zpl_vec3f((f32)zpl_random_range_f64(&g->rand, view_bounds.aabb.min.x,
                                            view_bounds.aabb.max.x),
                  (f32)zpl_random_range_f64(&g->rand, view_bounds.aabb.min.y,
                                            view_bounds.aabb.max.y),
                  -z);
    s->rotation_speed =
        zpl_vec3f((f32)zpl_random_range_f64(&g->rand, 0.0f, 2.5f),
                  (f32)zpl_random_range_f64(&g->rand, 0.0f, 2.5f),
                  (f32)zpl_random_range_f64(&g->rand, 0.0f, 2.5f));
    s->scale = 0.0f;
    s->color = colors[zpl_random_range_i64(&g->rand, 0, COLOR_COUNT - 1)];
    s->glow_min = (f32)zpl_random_range_f64(&g->rand, 5.0f, 10.0f);
    s->glow_max = (f32)zpl_random_range_f64(&g->rand, s->glow_min, 15.0f);
    s->glow_speed = (f32)zpl_random_range_f64(&g->rand, 0.25f, 5.0f);
    s->glow = s->glow_min;
    s->max_lifetime = (f32)zpl_random_range_f64(&g->rand, 2.0f, 10.0f);
    s->lifetime = 0.0f;
}

void background_star_update(Game *g, Background_Star *s, f32 total_time,
                            f32 delta_time)
{
    f32 progress = s->lifetime / s->max_lifetime;
    if (progress < 1.0f)
    {
        s->scale = ease_in_out(progress);
    }
    else
    {
        s->scale = ease_in_out(2.0f - progress);
    }

    s->lifetime += delta_time;
    if (s->lifetime >= s->max_lifetime * 2.0f)
    {
        background_star_reset(g, s);
    }

    s->rotation += s->rotation_speed * delta_time;
    s->glow = pulsate(total_time, s->glow_min, s->glow_max, s->glow_speed);
}
