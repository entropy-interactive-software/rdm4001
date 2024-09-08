#version 330 core
layout (location = 0) in vec2 v_pos;
layout (location = 1) in vec2 v_uv;

out vec2 f_uv;

void main() {
  f_uv = v_uv;

  gl_Position = vec4(v_pos, 0.0, 1.0);
}