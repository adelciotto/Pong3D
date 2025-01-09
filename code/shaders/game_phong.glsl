@module game_phong

@ctype vec3 zpl_vec3
@ctype mat4 zpl_mat4

@vs vs
layout(binding=0) uniform vs_params {
  mat4 u_obj_to_view_transform;
  mat4 u_obj_to_view_normal_transform;
  mat4 u_view_to_clip_transform;
};

in vec3 a_obj_position;
in vec3 a_obj_normal;

out vec3 v_view_position;
out vec3 v_view_normal;

void main() {
  vec4 view_position = u_obj_to_view_transform * vec4(a_obj_position, 1.0);
  v_view_position = view_position.xyz;
  v_view_normal = normalize(mat3(u_obj_to_view_normal_transform) * a_obj_normal);
  gl_Position = u_view_to_clip_transform * view_position;
}
@end

@fs fs
layout(binding=1) uniform fs_dir_light {
  vec3 direction;
  vec3 color;
  vec3 ambient;
} u_dir_light;

layout(binding=2) uniform fs_point_light_0 {
  vec3 view_position;
  vec3 color;
  vec3 ambient;
  float falloff;
  float radius;
} u_point_light_0;

layout(binding=3) uniform fs_material {
  vec3 color;
  float shininess;
} u_material;

in vec3 v_view_position;
in vec3 v_view_normal;

layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 bright_color;

float attenuation(float r, float f, float d) {
  float denom = d / r + 1.0;
  float attenuation = 1.0 / (denom*denom);
  float t = (attenuation - f) / (1.0 - f);
  return max(t, 0.0);
}

float compute_diffuse(vec3 L, vec3 N) {
  return max(0.0, dot(L, N));
}

float compute_specular(vec3 L, vec3 V, vec3 N, float shininess) {
  vec3 R = -reflect(L, N);
  return pow(max(0.0, dot(V, R)), shininess);
}

vec3 point_light(vec3 light_view_pos, vec3 light_color, vec3 light_ambient,
                      float light_falloff, float light_radius, vec3 V, vec3 N) {
  vec3 light_vector = light_view_pos - v_view_position;
  float light_dist = length(light_vector);
  float falloff = attenuation(light_radius, light_falloff, light_dist);

  vec3 L = normalize(light_vector);
  float specular = 1.0 * compute_specular(L, V, N, u_material.shininess) * 1.0 * falloff;
  vec3 diffuse = light_color * compute_diffuse(L, N) * falloff;
  return u_material.color * (diffuse * 3.0 + light_ambient) + specular;
}

vec3 directional_light(vec3 light_dir, vec3 light_color, vec3 light_ambient, vec3 V, vec3 N) {
  vec3 L = normalize(-light_dir);
  float specular = 1.0 * compute_specular(L, V, N, u_material.shininess) * 1.0;
  vec3 diffuse = light_color * compute_diffuse(L, N);
  return u_material.color * (diffuse + light_ambient) + specular;
}

void main() {
  vec3 V = normalize(v_view_position);
  vec3 N = normalize(v_view_normal);

  vec3 color = vec3(0.0);
  color += directional_light(u_dir_light.direction, u_dir_light.color, u_dir_light.ambient, V, N);
  color += point_light(u_point_light_0.view_position, u_point_light_0.color,
                            u_point_light_0.ambient, u_point_light_0.falloff,
                            u_point_light_0.radius, V, N);

  float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
  if (brightness > 1.0) {
    bright_color = vec4(color, 1.0);
  } else {
    bright_color = vec4(0.0, 0.0, 0.0, 1.0);
  }
  frag_color = vec4(color, 1.0);
}
@end

@program program vs fs
