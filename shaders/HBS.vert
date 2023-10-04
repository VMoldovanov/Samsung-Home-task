#version 320 es
precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTCoord;

uniform mat4 u_matMVP;
uniform vec2 u_screenResolution;
uniform vec2 u_sizeImage;

out vec2 v_TCoord;

void main()
{
	v_TCoord = aTCoord;

	// центрируем картинку на экране
	vec2 newPos = aPos.xy * u_sizeImage;
	vec2 centerOffset = (u_screenResolution - u_sizeImage) * 0.5;
	gl_Position = u_matMVP * vec4(newPos + centerOffset, aPos.z, 1.0);
}