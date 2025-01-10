@module game_basic

@ctype vec3 HMM_Vec3
@ctype mat4 HMM_Mat4

@vs vs
layout(binding=0) uniform vs_params {
  mat4 u_world_to_clip_transform;
};

in vec3 a_obj_position;
in mat4 inst_obj_to_world_transform;
in vec3 inst_color;

out vec3 color;

void main() {
  color = inst_color;
  gl_Position = u_world_to_clip_transform * inst_obj_to_world_transform * vec4(a_obj_position, 1.0);
}
@end

@fs fs
in vec3 color;

layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 bright_color;

void main() {
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
