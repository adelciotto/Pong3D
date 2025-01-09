@module combine_display

@ctype vec2 zpl_vec2

@vs vs
in vec2 a_obj_position;
in vec2 a_obj_uv;

out vec2 v_uv;

void main() {
  v_uv = a_obj_uv;
  gl_Position = vec4(a_obj_position.xy, 0.0, 1.0);
}
@end

@fs fs
layout(binding=0) uniform texture2D u_tex_0;
layout(binding=1) uniform texture2D u_tex_1;
layout(binding=0) uniform sampler u_smp;

layout(binding=0) uniform fs_params {
  float u_exposure;
  float u_bloom_strength;
};

in vec2 v_uv;

out vec4 frag_color;

void main() {
  vec3 hdr_color = texture(sampler2D(u_tex_0, u_smp), v_uv).rgb;
  vec3 bloom_color = texture(sampler2D(u_tex_1, u_smp), v_uv).rgb;
  vec3 result = mix(hdr_color, bloom_color, u_bloom_strength);
  result = vec3(1.0) - exp(-result * u_exposure);
  result = pow(abs(result), vec3(1.0 / 2.2));
  frag_color = vec4(result, 1.0);

}
@end

@program program vs fs
