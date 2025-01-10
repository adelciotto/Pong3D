/*------------------------------------------------------------------------------
  pong3d

  A 3D Pong clone.

  Created using the excellent sokol libraries and tools.
  Written by Anthony Del Ciotto.

  TODO:
    - Add 3D text drawing.
    - Add paddles, implement basic gameplay.
    - Add A.I player.
    - Add sound effects.
    - Add options to menu. Audio, graphics etc.
    - Add cool intro with demo gameplay if user is inactive.
    - Maintain correct aspect ratio when resizing window.
*/

#include "game.h"
#include "input.h"
#include "renderer.h"
#include "sokol_debugtext.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"
#include <cstdlib>
#include <cstring>
#include <ctime>

static constexpr double sims_per_sec = 1.0 / 60.0;

struct App_State
{
    Input input;
    Renderer renderer;
    Game game;
    Game game_temp;
    uint64_t start_time;
    uint64_t last_sim_time;
    double accumulated_time_secs;
};

static App_State *as = nullptr;

static void init()
{
    {
        // TODO: Reduce pool allocation size to what's actually needed.
        sg_desc desc = {};
        desc.environment = sglue_environment();
        desc.logger.func = slog_func;
        sg_setup(desc);
    }
    {
        sdtx_desc_t desc = {};
        desc.fonts[0] = sdtx_font_c64();
        sdtx_setup(desc);
    }

    as = static_cast<App_State *>(calloc(1, sizeof(App_State)));

    stm_setup();
    as->start_time = stm_now();

    input_init(as->input);
    renderer_init(as->renderer, sapp_width(), sapp_height());

    time_t seconds;
    time(&seconds);
    game_init(as->game, as->input, as->renderer,
              static_cast<uint32_t>(seconds));
}

static void frame()
{
    game_input(as->game);

    // At the moment the game uses the sokol provided sapp_frame_duration() for
    // it's fixed timestep simulation. This is a smoothed value over N frames.
    // Let's revisit if we should use this or a non-smoothed one we calculate
    // manually.
    double frame_time_secs = sapp_frame_duration();
    as->accumulated_time_secs += frame_time_secs;
    while (as->accumulated_time_secs >= sims_per_sec)
    {
        double total_time_secs = stm_sec(stm_since(as->start_time));
        game_sim(as->game, total_time_secs, sims_per_sec);
        as->last_sim_time = stm_now();
        as->accumulated_time_secs -= sims_per_sec;
    }
    {
        double total_time_secs = stm_sec(stm_since(as->start_time));
        double delta_time_secs = stm_sec(stm_since(as->last_sim_time));
        memcpy(&as->game_temp, &as->game, sizeof(Game));
        game_sim(as->game_temp, total_time_secs, delta_time_secs);
        game_draw(as->game_temp);
    }

    input_update(as->input);

    renderer_render(as->renderer, sglue_swapchain());

    // Draw some debug info.
    sdtx_canvas(sapp_widthf() * 0.5f, sapp_heightf() * 0.5f);
    sdtx_origin(2.0f, 2.0f);
    sdtx_font(0);

    double ms_per_frame = frame_time_secs * 1000.0;
    double fps = 1000.0 / ms_per_frame;
    sdtx_printf("%.3fms/f\n", ms_per_frame);
    sdtx_crlf();
    sdtx_printf("%.1f FPS", fps);

    sg_pass pass = {};
    pass.action.colors[0].load_action = SG_LOADACTION_DONTCARE;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(pass);
    sdtx_draw();
    sg_end_pass();

    sg_commit();
}

static void cleanup()
{
    free(as);

    sdtx_shutdown();
    sg_shutdown();
}

static void event(const sapp_event *ev)
{
    if (ev->type == SAPP_EVENTTYPE_RESIZED)
    {
        renderer_resize(as->renderer, ev->framebuffer_width,
                        ev->framebuffer_height);
    }

    input_handle_event(as->input, ev);

    // Allow user to quickly toggle fullscreen with alt-enter.
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN)
    {
        if (ev->key_code == SAPP_KEYCODE_ENTER &&
            ev->modifiers & SAPP_MODIFIER_ALT)
        {
            sapp_toggle_fullscreen();
        }
    }
}

sapp_desc sokol_main(int argc, char *argv[])
{
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.event_cb = event;
    desc.width = 1280 * 2;
    desc.height = 720 * 2;
    desc.window_title = "Pong3D";
    desc.logger.func = slog_func;
    desc.high_dpi = true;
    return desc;
}
