#pragma once

#include "zpl_wrap.h"
#include "sokol_gfx.h"

inline constexpr isize point_lights_count = 1;
inline constexpr isize draw_calls_max_count = 16;
inline constexpr isize basic_box_instances_max_count = 1024;
inline constexpr isize bloom_mips_count = 6;

struct Quad_Geometry
{
    sg_buffer vbuf;
    sg_buffer ibuf;
    int elements_count;
};

struct Box_Geometry
{
    sg_buffer vbuf;
    sg_buffer ibuf;
    int elements_count;
};

struct Text_Geometry
{
};

struct Directional_Light
{
    zpl_vec3 direction;
    zpl_vec3 diffuse_color;
    zpl_vec3 ambient_color;
};

struct Point_Light
{
    // TODO: Add a enabled property.
    zpl_vec3 position;
    zpl_vec3 diffuse_color;
    zpl_vec3 ambient_color;
    f32 falloff;
    f32 radius;
};

struct Basic_Box_Instance
{
    zpl_mat4 obj_to_world_transform;
    zpl_vec3 color;
};

struct Draw_Call
{
    sg_pipeline pip;
    sg_bindings bind;
    int base_element;
    int elements_count;
    int instances_count;
    zpl_mat4 obj_to_world_transform;
    zpl_vec3 color;
};

struct Bloom_Mip
{
    sg_image img;
    sg_attachments atts;
    int width;
    int height;
    zpl_vec2 texel_size;
};

struct Game_Pass
{
    // These members should be set directly by the game. I didn't feel a
    // need to add an abstraction.
    Directional_Light dir_light;
    Point_Light point_lights[point_lights_count];
    zpl_mat4 world_to_view_transform;
    zpl_mat4 view_to_clip_transform;

    sg_pass_action pass_action;
    sg_image msaa_images[2];
    sg_image resolve_images[2];
    sg_image depth_image;
    sg_attachments atts;
    Draw_Call draw_calls[draw_calls_max_count];
    isize draw_calls_count;
    sg_pipeline phong_pip;
    Basic_Box_Instance basic_instances[basic_box_instances_max_count];
    isize basic_instances_count;
    sg_buffer basic_instances_buffer;
    sg_pipeline basic_pip;
};

struct Bloom_Pass
{
    Bloom_Mip mips[bloom_mips_count];
    sg_pass_action down_sample_pass_action;
    sg_pass_action up_sample_pass_action;
    sg_pipeline down_sample_pip;
    sg_pipeline up_sample_pip;
};

struct Combine_Display_Pass
{
    sg_pass_action pass_action;
    sg_pipeline pip;
};

struct Renderer
{
    Quad_Geometry quad;
    Box_Geometry box;
    Text_Geometry text;
    sg_sampler smp;
    int framebuffer_width;
    int framebuffer_height;
    Game_Pass game_pass;
    Bloom_Pass bloom_pass;
    Combine_Display_Pass combine_display_pass;
};

void renderer_init(Renderer *r, int framebuffer_width, int framebuffer_height);
void renderer_resize(Renderer *r, int framebuffer_width,
                     int framebuffer_height);

void renderer_draw_basic_box_instance(Renderer *r, zpl_vec3 position,
                                      zpl_vec3 rotation, zpl_vec3 scale,
                                      zpl_vec3 color);
void renderer_draw_phong_box(Renderer *r, zpl_vec3 position, zpl_vec3 rotation,
                             zpl_vec3 scale, zpl_vec3 color);

void renderer_render(Renderer *r, sg_swapchain swapchain);
