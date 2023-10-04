#version 320 es
precision mediump float;

in vec2 v_TCoord;

uniform sampler2D u_texture0;
uniform mat3 u_matHSB;

out vec4 FragColor;

void main()
{
   // применяем цветовые трансформации к полученому сэмплу
   vec3 texelRGB = texture(u_texture0, v_TCoord).xyz;
   FragColor = vec4(u_matHSB * texelRGB, 1.0);
}