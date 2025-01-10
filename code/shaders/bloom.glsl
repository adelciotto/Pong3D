@module bloom

@ctype vec2 HMM_Vec2

@vs vs
in vec2 a_obj_position;
in vec2 a_obj_uv;

out vec2 v_uv;

void main() {
  v_uv = a_obj_uv;
  gl_Position = vec4(a_obj_position.xy, 0.0, 1.0);
}
@end

@fs fs_down_sample
in vec2 v_uv;

out vec3 frag_color;

layout(binding=0) uniform texture2D u_down_sample_tex;
layout(binding=0) uniform sampler u_down_sample_smp;

layout(binding=0) uniform fs_down_sample_uniforms {
  vec2 u_texel_size;
};

void main() {
  float x = u_texel_size.x;
  float y = u_texel_size.y;
  vec3 a = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x - 2*x, v_uv.y + 2*y)).rgb;
  vec3 b = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x,       v_uv.y + 2*y)).rgb;
  vec3 c = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x + 2*x, v_uv.y + 2*y)).rgb;
  vec3 d = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x - 2*x, v_uv.y)).rgb;
  vec3 e = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x,       v_uv.y)).rgb;
  vec3 f = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x + 2*x, v_uv.y)).rgb;
  vec3 g = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x - 2*x, v_uv.y - 2*y)).rgb;
  vec3 h = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x,       v_uv.y - 2*y)).rgb;
  vec3 i = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x + 2*x, v_uv.y - 2*y)).rgb;
  vec3 j = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x - x,   v_uv.y + y)).rgb;
  vec3 k = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x + x,   v_uv.y + y)).rgb;
  vec3 l = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x - x,   v_uv.y - y)).rgb;
  vec3 m = texture(sampler2D(u_down_sample_tex, u_down_sample_smp), vec2(v_uv.x + x,   v_uv.y - y)).rgb;
  frag_color = e*0.125;
  frag_color += (a+c+g+i)*0.03125;
  frag_color += (b+d+f+h)*0.0625;
  frag_color += (j+k+l+m)*0.125;
  frag_color = max(frag_color, 0.0001f);
}
@end

@fs fs_up_sample
in vec2 v_uv;

out vec3 frag_color;

layout(binding=0) uniform texture2D u_up_sample_tex;
layout(binding=0) uniform sampler u_up_sample_smp;

layout(binding=0) uniform fs_up_sample_uniforms {
  float u_filter_radius;
};

void main() {
  float x = u_filter_radius;
  float y = u_filter_radius;
  vec3 a = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x - x, v_uv.y + y)).rgb;
  vec3 b = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x,     v_uv.y + y)).rgb;
  vec3 c = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x + x, v_uv.y + y)).rgb;
  vec3 d = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x - x, v_uv.y)).rgb;
  vec3 e = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x,     v_uv.y)).rgb;
  vec3 f = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x + x, v_uv.y)).rgb;
  vec3 g = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x - x, v_uv.y - y)).rgb;
  vec3 h = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x,     v_uv.y - y)).rgb;
  vec3 i = texture(sampler2D(u_up_sample_tex, u_up_sample_smp), vec2(v_uv.x + x, v_uv.y - y)).rgb;
  frag_color = e*4.0;
  frag_color += (b+d+f+h)*2.0;
  frag_color += (a+c+g+i);
  frag_color *= 1.0 / 16.0;
}
@end

@program program_down_sample vs fs_down_sample
@program program_up_sample vs fs_up_sample
