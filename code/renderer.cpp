#include "renderer.h"
#include "bloom.glsl.h"
#include "combine_display.glsl.h"
#include "game_basic.glsl.h"
#include "game_phong.glsl.h"

static constexpr int msaa_sample_count = 4;
static constexpr f32 bloom_filter_radius = 0.003f;

static void renderer_init_quad_geometry(Renderer *r)
{
    // clang-format off
    static constexpr f32 vertices[] = {
        -1.0f, +1.0f, 0.0f, 1.0f,
        +1.0f, +1.0f, 1.0f, 1.0f,
        +1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
    };
    // clang-format on
    static constexpr u16 indices[] = {0, 1, 2, 0, 2, 3};
    {
        sg_buffer_desc desc = {};
        desc.data = SG_RANGE(vertices);
        r->quad.vbuf = sg_make_buffer(desc);
    }
    {
        sg_buffer_desc desc = {};
        desc.type = SG_BUFFERTYPE_INDEXBUFFER;
        desc.data = SG_RANGE(indices);
        r->quad.ibuf = sg_make_buffer(desc);
    }
    r->quad.elements_count = zpl_count_of(indices);
}

static void renderer_init_box_geometry(Renderer *r)
{
    static constexpr f32 vertices[] = {
        -1.0f, -1.0f, -1.0f, 0.0f,  0.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 0.0f,
        0.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, -1.0f, 1.0f,
        -1.0f, 0.0f,  0.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
        1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,
        0.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  -1.0f, -1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 0.0f,  0.0f,
        -1.0f, 1.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
        0.0f,  0.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
        1.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  -1.0f, -1.0f, -1.0f, 0.0f,
        -1.0f, 0.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  -1.0f, 0.0f,  1.0f,  -1.0f,
        1.0f,  0.0f,  -1.0f, 0.0f,  1.0f,  -1.0f, -1.0f, 0.0f,  -1.0f, 0.0f,
        -1.0f, 1.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  -1.0f, 1.0f,  1.0f,  0.0f,
        1.0f,  0.0f,  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        -1.0f, 0.0f,  1.0f,  0.0f,
    };
    static constexpr u16 indices[] = {
        0,  1,  2,  0,  2,  3,  6,  5,  4,  7,  6,  4,  8,  9,  10, 8,  10, 11,
        14, 13, 12, 15, 14, 12, 16, 17, 18, 16, 18, 19, 22, 21, 20, 23, 22, 20};
    {
        sg_buffer_desc desc = {};
        desc.data = SG_RANGE(vertices);
        r->box.vbuf = sg_make_buffer(desc);
    }
    {
        sg_buffer_desc desc = {};
        desc.type = SG_BUFFERTYPE_INDEXBUFFER;
        desc.data = SG_RANGE(indices);
        r->box.ibuf = sg_make_buffer(desc);
    }
    r->box.elements_count = zpl_count_of(indices);
}

static void renderer_init_game_pass(Renderer *r)
{
    r->game_pass.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    r->game_pass.pass_action.colors[0].store_action = SG_STOREACTION_DONTCARE;
    r->game_pass.pass_action.colors[0].clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
    r->game_pass.pass_action.colors[1].load_action = SG_LOADACTION_CLEAR;
    r->game_pass.pass_action.colors[1].store_action = SG_STOREACTION_DONTCARE;
    r->game_pass.pass_action.colors[1].clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
    {
        sg_pipeline_desc desc = {};
        desc.layout.attrs[ATTR_game_phong_program_a_obj_position] = {
            0, 0, SG_VERTEXFORMAT_FLOAT3};
        desc.layout.attrs[ATTR_game_phong_program_a_obj_normal] = {
            0, sizeof(float) * 3, SG_VERTEXFORMAT_FLOAT3};
        desc.shader =
            sg_make_shader(game_phong_program_shader_desc(sg_query_backend()));
        desc.index_type = SG_INDEXTYPE_UINT16;
        desc.cull_mode = SG_CULLMODE_BACK,
        desc.sample_count = msaa_sample_count, desc.depth.write_enabled = true;
        desc.depth.pixel_format = SG_PIXELFORMAT_DEPTH;
        desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
        desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA16F;
        desc.colors[1].pixel_format = SG_PIXELFORMAT_RGBA16F;
        desc.color_count = 2;
        r->game_pass.phong_pip = sg_make_pipeline(desc);
    }
    {
        sg_pipeline_desc desc = {};
        desc.layout.buffers[0].stride = sizeof(f32) * 6;
        desc.layout.buffers[1].stride = sizeof(Basic_Box_Instance);
        desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE;
        desc.layout.attrs[ATTR_game_basic_program_a_obj_position] = {
            0, 0, SG_VERTEXFORMAT_FLOAT3};
        desc.layout.attrs[ATTR_game_basic_program_inst_obj_to_world_transform] =
            {1, 0, SG_VERTEXFORMAT_FLOAT4};
        desc.layout.attrs[ATTR_game_basic_program_inst_obj_to_world_transform +
                          1] = {1, sizeof(float) * 4, SG_VERTEXFORMAT_FLOAT4};
        desc.layout
            .attrs[ATTR_game_basic_program_inst_obj_to_world_transform + 2] = {
            1, sizeof(float) * 4 * 2, SG_VERTEXFORMAT_FLOAT4};
        desc.layout
            .attrs[ATTR_game_basic_program_inst_obj_to_world_transform + 3] = {
            1, sizeof(float) * 4 * 3, SG_VERTEXFORMAT_FLOAT4};
        desc.layout.attrs[ATTR_game_basic_program_inst_color] = {
            1, sizeof(float) * 4 * 4, SG_VERTEXFORMAT_FLOAT3};
        desc.shader =
            sg_make_shader(game_basic_program_shader_desc(sg_query_backend()));
        desc.index_type = SG_INDEXTYPE_UINT16;
        desc.cull_mode = SG_CULLMODE_BACK;
        desc.sample_count = msaa_sample_count;
        desc.depth.write_enabled = true;
        desc.depth.pixel_format = SG_PIXELFORMAT_DEPTH;
        desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
        desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA16F;
        desc.colors[1].pixel_format = SG_PIXELFORMAT_RGBA16F;
        desc.color_count = 2;
        r->game_pass.basic_pip = sg_make_pipeline(desc);
    }
    {
        sg_buffer_desc desc = {};
        desc.usage = SG_USAGE_STREAM;
        desc.size = sizeof(Basic_Box_Instance) * basic_box_instances_max_count;
        r->game_pass.basic_instances_buffer = sg_make_buffer(desc);
    }
}

static void renderer_init_bloom_pass(Renderer *r)
{
    r->bloom_pass.down_sample_pass_action.colors[0].load_action =
        SG_LOADACTION_DONTCARE;
    r->bloom_pass.up_sample_pass_action.colors[0].load_action =
        SG_LOADACTION_DONTCARE;
    {
        sg_pipeline_desc desc = {};
        desc.layout.attrs[ATTR_bloom_program_down_sample_a_obj_position]
            .format = SG_VERTEXFORMAT_FLOAT2;
        desc.layout.attrs[ATTR_bloom_program_down_sample_a_obj_uv].format =
            SG_VERTEXFORMAT_FLOAT2;
        desc.shader = sg_make_shader(
            bloom_program_down_sample_shader_desc(sg_query_backend()));
        desc.index_type = SG_INDEXTYPE_UINT16;
        desc.cull_mode = SG_CULLMODE_BACK;
        desc.colors[0].pixel_format = SG_PIXELFORMAT_RG11B10F;
        desc.depth.pixel_format = SG_PIXELFORMAT_NONE;
        r->bloom_pass.down_sample_pip = sg_make_pipeline(desc);
    }
    {
        sg_pipeline_desc desc = {};
        desc.layout.attrs[ATTR_bloom_program_up_sample_a_obj_position].format =
            SG_VERTEXFORMAT_FLOAT2;
        desc.layout.attrs[ATTR_bloom_program_up_sample_a_obj_uv].format =
            SG_VERTEXFORMAT_FLOAT2;
        desc.shader = sg_make_shader(
            bloom_program_up_sample_shader_desc(sg_query_backend()));
        desc.index_type = SG_INDEXTYPE_UINT16;
        desc.cull_mode = SG_CULLMODE_BACK;
        desc.colors[0].pixel_format = SG_PIXELFORMAT_RG11B10F;
        desc.colors[0].blend.enabled = true;
        desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
        desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
        desc.colors[0].blend.op_rgb = SG_BLENDOP_ADD;
        desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
        desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
        desc.colors[0].blend.op_alpha = SG_BLENDOP_ADD;
        desc.depth.pixel_format = SG_PIXELFORMAT_NONE;
        r->bloom_pass.up_sample_pip = sg_make_pipeline(desc);
    }
}

static void renderer_init_combine_display_pass(Renderer *r)
{
    r->combine_display_pass.pass_action.colors[0].load_action =
        SG_LOADACTION_DONTCARE;
    {
        sg_pipeline_desc desc = {};
        desc.layout.attrs[ATTR_combine_display_program_a_obj_position].format =
            SG_VERTEXFORMAT_FLOAT2;
        desc.layout.attrs[ATTR_combine_display_program_a_obj_uv].format =
            SG_VERTEXFORMAT_FLOAT2;
        desc.shader = sg_make_shader(
            combine_display_program_shader_desc(sg_query_backend()));
        desc.index_type = SG_INDEXTYPE_UINT16;
        desc.cull_mode = SG_CULLMODE_BACK;
        r->combine_display_pass.pip = sg_make_pipeline(desc);
    }
}

void renderer_init(Renderer *r, int framebuffer_width, int framebuffer_height)
{
    renderer_init_quad_geometry(r);
    renderer_init_box_geometry(r);

    {
        sg_sampler_desc desc = {};
        desc.min_filter = SG_FILTER_LINEAR;
        desc.mag_filter = SG_FILTER_LINEAR;
        desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
        r->smp = sg_make_sampler(desc);
    }

    renderer_init_game_pass(r);
    renderer_init_bloom_pass(r);
    renderer_init_combine_display_pass(r);

    // Call resize manually for the first time to ensure render targets are
    // initialized.
    renderer_resize(r, framebuffer_width, framebuffer_height);
}

static void renderer_resize_game_pass(Renderer *r)
{
    for (isize i = 0; i < 2; ++i)
    {
        sg_destroy_image(r->game_pass.resolve_images[i]);
        sg_destroy_image(r->game_pass.msaa_images[i]);
    }
    sg_destroy_image(r->game_pass.depth_image);
    sg_destroy_attachments(r->game_pass.atts);

    for (isize i = 0; i < 2; ++i)
    {
        {
            sg_image_desc desc = {};
            desc.render_target = true;
            desc.width = r->framebuffer_width;
            desc.height = r->framebuffer_height;
            desc.pixel_format = SG_PIXELFORMAT_RGBA16F;
            desc.sample_count = msaa_sample_count;
            r->game_pass.msaa_images[i] = sg_make_image(desc);
        }
        {
            sg_image_desc desc = {};
            desc.render_target = true;
            desc.width = r->framebuffer_width;
            desc.height = r->framebuffer_height;
            desc.pixel_format = SG_PIXELFORMAT_RGBA16F;
            desc.sample_count = 1;
            r->game_pass.resolve_images[i] = sg_make_image(desc);
        }
    }
    {
        sg_image_desc desc = {};
        desc.render_target = true;
        desc.width = r->framebuffer_width;
        desc.height = r->framebuffer_height;
        desc.pixel_format = SG_PIXELFORMAT_DEPTH;
        desc.sample_count = msaa_sample_count;
        r->game_pass.depth_image = sg_make_image(desc);
    }
    {
        sg_attachments_desc desc = {};
        desc.colors[0].image = r->game_pass.msaa_images[0];
        desc.colors[1].image = r->game_pass.msaa_images[1];
        desc.resolves[0].image = r->game_pass.resolve_images[0];
        desc.resolves[1].image = r->game_pass.resolve_images[1];
        desc.depth_stencil.image = r->game_pass.depth_image;
        r->game_pass.atts = sg_make_attachments(desc);
    }
}

static void renderer_resize_bloom_pass(Renderer *r)
{
    int width_i = r->framebuffer_width;
    int height_i = r->framebuffer_height;
    f32 width_f = (f32)r->framebuffer_width;
    f32 height_f = (f32)r->framebuffer_height;
    zpl_vec2 texel_size = zpl_vec2f(1.0f / width_f, 1.0f / height_f);
    for (isize i = 0; i < bloom_mips_count; i += 1)
    {
        sg_destroy_image(r->bloom_pass.mips[i].img);
        sg_destroy_attachments(r->bloom_pass.mips[i].atts);

        width_i /= 2;
        height_i /= 2;
        width_f *= 0.5f;
        height_f *= 0.5f;
        r->bloom_pass.mips[i].texel_size = texel_size;
        r->bloom_pass.mips[i].width = width_i;
        r->bloom_pass.mips[i].height = height_i;
        {
            sg_image_desc desc = {};
            desc.render_target = true;
            desc.width = width_i;
            desc.height = height_i;
            desc.pixel_format = SG_PIXELFORMAT_RG11B10F;
            desc.sample_count = 1;
            r->bloom_pass.mips[i].img = sg_make_image(desc);
        }
        {
            sg_attachments_desc desc = {};
            desc.colors[0].image = r->bloom_pass.mips[i].img;
            r->bloom_pass.mips[i].atts = sg_make_attachments(desc);
        }
        texel_size *= 2.0f;
    }
}

void renderer_resize(Renderer *r, int framebuffer_width, int framebuffer_height)
{
    r->framebuffer_width = framebuffer_width;
    r->framebuffer_height = framebuffer_height;
    renderer_resize_game_pass(r);
    renderer_resize_bloom_pass(r);
}

static zpl_mat4 compute_obj_to_world_transform(zpl_vec3 pos, zpl_vec3 rot,
                                               zpl_vec3 scale)
{
    zpl_quat rq = zpl_quatf(1.0f, 0.0f, 0.0f, 0.0f);
    if (rot.x != 0.0f)
    {
        rq *= zpl_quat_axis_angle(zpl_vec3f(1.0f, 0.0f, 0.0f), rot.x);
    }
    if (rot.y != 0.0f)
    {
        rq *= zpl_quat_axis_angle(zpl_vec3f(0.0f, 1.0f, 0.0f), rot.y);
    }
    if (rot.z != 0.0f)
    {
        rq *= zpl_quat_axis_angle(zpl_vec3f(0.0f, 0.0f, 1.0f), rot.z);
    }
    zpl_mat4 rm;
    zpl_mat4_from_quat(&rm, rq);

    zpl_mat4 result;
    zpl_mat4_identity(&result);
    zpl_mat4_translate(&result, pos);
    result *= rm;
    zpl_mat4_scale(&result, scale);
    return result;
}

void renderer_draw_basic_box_instance(Renderer *r, zpl_vec3 position,
                                      zpl_vec3 rotation, zpl_vec3 scale,
                                      zpl_vec3 color)
{
    ZPL_ASSERT(r->game_pass.basic_instances_count <
               basic_box_instances_max_count);

    Basic_Box_Instance *instance =
        &r->game_pass.basic_instances[r->game_pass.basic_instances_count];
    instance->obj_to_world_transform =
        compute_obj_to_world_transform(position, rotation, scale);
    instance->color = color;
    r->game_pass.basic_instances_count += 1;
}

void renderer_draw_phong_box(Renderer *r, zpl_vec3 position, zpl_vec3 rotation,
                             zpl_vec3 scale, zpl_vec3 color)
{
    ZPL_ASSERT(r->game_pass.draw_calls_count < draw_calls_max_count);

    Draw_Call *draw_call =
        &r->game_pass.draw_calls[r->game_pass.draw_calls_count];

    draw_call->pip = r->game_pass.phong_pip;
    draw_call->bind.vertex_buffers[0] = r->box.vbuf,
    draw_call->bind.index_buffer = r->box.ibuf;
    draw_call->base_element = 0;
    draw_call->elements_count = r->box.elements_count;
    draw_call->instances_count = 1;
    draw_call->obj_to_world_transform =
        compute_obj_to_world_transform(position, rotation, scale);
    draw_call->color = color;

    r->game_pass.draw_calls_count += 1;
}

static void renderer_render_game_pass(Renderer *r)
{
    game_phong_fs_dir_light_t fs_dir_light = {};
    fs_dir_light.direction = r->game_pass.dir_light.direction;
    fs_dir_light.color = r->game_pass.dir_light.diffuse_color;
    fs_dir_light.ambient = r->game_pass.dir_light.ambient_color;

    Point_Light point_light = r->game_pass.point_lights[0];
    game_phong_fs_point_light_0_t fs_point_light = {};
    fs_point_light.color = point_light.diffuse_color;
    fs_point_light.ambient = point_light.ambient_color;
    fs_point_light.falloff = point_light.falloff;
    fs_point_light.radius = point_light.radius;

    zpl_vec4 view_pos;
    zpl_mat4_mul_vec4(&view_pos, &r->game_pass.world_to_view_transform,
                      zpl_vec4f(point_light.position.x, point_light.position.y,
                                point_light.position.z, 1.0f));
    fs_point_light.view_position = view_pos.xyz;

    sg_pass pass = {};
    pass.action = r->game_pass.pass_action;
    pass.attachments = r->game_pass.atts;
    sg_begin_pass(pass);

    // Add draw call for instanced geometry if required.
    if (r->game_pass.basic_instances_count > 0)
    {
        ZPL_ASSERT(r->game_pass.draw_calls_count < draw_calls_max_count);

        sg_range range = {};
        range.ptr = r->game_pass.basic_instances;
        range.size =
            sizeof(Basic_Box_Instance) * r->game_pass.basic_instances_count;
        sg_update_buffer(r->game_pass.basic_instances_buffer, range);

        Draw_Call *draw_call =
            &r->game_pass.draw_calls[r->game_pass.draw_calls_count];

        draw_call->pip = r->game_pass.basic_pip;
        draw_call->bind.vertex_buffers[0] = r->box.vbuf,
        draw_call->bind.vertex_buffers[1] = r->game_pass.basic_instances_buffer,
        draw_call->bind.index_buffer = r->box.ibuf, draw_call->base_element = 0;
        draw_call->elements_count = r->box.elements_count;
        draw_call->instances_count = (int)r->game_pass.basic_instances_count;

        r->game_pass.draw_calls_count += 1;
        r->game_pass.basic_instances_count = 0;
    }

    if (r->game_pass.draw_calls_count > 0)
    {
        for (isize i = 0; i < r->game_pass.draw_calls_count; i += 1)
        {
            Draw_Call *draw_call = &r->game_pass.draw_calls[i];

            if (draw_call->pip.id == r->game_pass.phong_pip.id)
            {
                sg_apply_pipeline(r->game_pass.phong_pip);

                game_phong_vs_params_t vs_params = (game_phong_vs_params_t){
                    .u_view_to_clip_transform =
                        r->game_pass.view_to_clip_transform,
                };
                vs_params.u_obj_to_view_transform =
                    r->game_pass.world_to_view_transform *
                    draw_call->obj_to_world_transform;
                zpl_mat4_inverse(&vs_params.u_obj_to_view_normal_transform,
                                 &vs_params.u_obj_to_view_transform);
                zpl_mat4_transpose(&vs_params.u_obj_to_view_normal_transform);
                sg_apply_uniforms(UB_game_phong_vs_params, SG_RANGE(vs_params));

                game_phong_fs_material_t material = {};
                material.color = draw_call->color;
                // TODO: Provide option for this in draw call.
                material.shininess = 20.0f;
                sg_apply_uniforms(UB_game_phong_fs_material,
                                  SG_RANGE(material));

                sg_apply_uniforms(UB_game_phong_fs_dir_light,
                                  SG_RANGE(fs_dir_light));
                sg_apply_uniforms(UB_game_phong_fs_point_light_0,
                                  SG_RANGE(fs_point_light));
            }

            else if (draw_call->pip.id == r->game_pass.basic_pip.id)
            {
                sg_apply_pipeline(r->game_pass.basic_pip);

                game_basic_vs_params_t vs_params = {};
                vs_params.u_world_to_clip_transform =
                    r->game_pass.view_to_clip_transform *
                    r->game_pass.world_to_view_transform;
                sg_apply_uniforms(UB_game_basic_vs_params, SG_RANGE(vs_params));
            }

            sg_apply_bindings(&draw_call->bind);
            sg_draw(draw_call->base_element, draw_call->elements_count,
                    draw_call->instances_count);
        }

        r->game_pass.draw_calls_count = 0;
    }

    sg_end_pass();
}

static void renderer_render_bloom_pass(Renderer *r)
{
    sg_image current_image = r->game_pass.resolve_images[1];
    for (isize i = 0; i < bloom_mips_count; i += 1)
    {
        sg_pass pass = {};
        pass.action = r->bloom_pass.down_sample_pass_action;
        pass.attachments = r->bloom_pass.mips[i].atts;
        sg_begin_pass(pass);

        sg_apply_viewport(0, 0, r->bloom_pass.mips[i].width,
                          r->bloom_pass.mips[i].height, true);
        sg_apply_pipeline(r->bloom_pass.down_sample_pip);

        bloom_fs_down_sample_uniforms_t fs_params = {};
        fs_params.u_texel_size = r->bloom_pass.mips[i].texel_size;
        sg_apply_uniforms(UB_bloom_fs_down_sample_uniforms,
                          SG_RANGE(fs_params));

        sg_bindings bind = {};
        bind.vertex_buffers[0] = r->quad.vbuf;
        bind.index_buffer = r->quad.ibuf;
        bind.images[IMG_bloom_u_down_sample_tex] = current_image;
        bind.samplers[SMP_bloom_u_down_sample_smp] = r->smp;
        sg_apply_bindings(bind);
        sg_draw(0, r->quad.elements_count, 1);

        sg_end_pass();

        current_image = r->bloom_pass.mips[i].img;
    }

    for (isize i = bloom_mips_count - 1; i > 0; i -= 1)
    {
        sg_pass pass = {};
        pass.action = r->bloom_pass.up_sample_pass_action;
        pass.attachments = r->bloom_pass.mips[i - 1].atts;
        sg_begin_pass(pass);
        sg_apply_viewport(0, 0, r->bloom_pass.mips[i - 1].width,
                          r->bloom_pass.mips[i - 1].height, true);

        sg_apply_pipeline(r->bloom_pass.up_sample_pip);

        bloom_fs_up_sample_uniforms_t fs_params = {};
        fs_params.u_filter_radius = bloom_filter_radius;
        sg_apply_uniforms(UB_bloom_fs_up_sample_uniforms, SG_RANGE(fs_params));

        sg_bindings bind = {};
        bind.vertex_buffers[0] = r->quad.vbuf;
        bind.index_buffer = r->quad.ibuf;
        bind.images[IMG_bloom_u_up_sample_tex] = r->bloom_pass.mips[i].img;
        bind.samplers[SMP_bloom_u_up_sample_smp] = r->smp;
        sg_apply_bindings(bind);
        sg_draw(0, r->quad.elements_count, 1);

        sg_end_pass();
    }
}

static void renderer_render_combine_display_pass(Renderer *r,
                                                 sg_swapchain swapchain)
{
    sg_pass pass = {};
    pass.action = r->combine_display_pass.pass_action;
    pass.swapchain = swapchain;
    sg_begin_pass(pass);

    sg_apply_pipeline(r->combine_display_pass.pip);

    combine_display_fs_params_t fs_params = {};
    fs_params.u_exposure = 1.0f;
    fs_params.u_bloom_strength = 0.04f;
    sg_apply_uniforms(UB_combine_display_fs_params, SG_RANGE(fs_params));

    sg_bindings bind = {};
    bind.vertex_buffers[0] = r->quad.vbuf;
    bind.index_buffer = r->quad.ibuf;
    bind.images[IMG_combine_display_u_tex_0] = r->game_pass.resolve_images[0];
    bind.images[IMG_combine_display_u_tex_1] = r->bloom_pass.mips[0].img;
    bind.samplers[SMP_combine_display_u_smp] = r->smp;
    sg_apply_bindings(bind);
    sg_draw(0, r->quad.elements_count, 1);

    sg_end_pass();
}

void renderer_render(Renderer *r, sg_swapchain swapchain)
{
    renderer_render_game_pass(r);
    renderer_render_bloom_pass(r);
    renderer_render_combine_display_pass(r, swapchain);
}
