// Grid Shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;
	mat4 u_InverseViewProjection;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} u_Renderer;

layout (location = 0) out vec3 v_WorldPosition;

void main()
{
	// Calculate world position
	vec4 worldPos = u_Renderer.Transform * vec4(a_Position, 1.0);
	v_WorldPosition = worldPos.xyz;
	
	vec4 position = u_ViewProjectionMatrix * worldPos;
	gl_Position = position;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 unused;

layout (std140, binding = 2) uniform SceneData
{
	vec3 u_CameraPosition;
	float padding;
} u_SceneData;

layout (push_constant) uniform Settings
{
	layout (offset = 64) float Scale;
	float Size;
} u_Settings;

layout (location = 0) in vec3 v_WorldPosition;

float grid(vec2 st, float res)
{
	vec2 grid = abs(fract(st - 0.5) - 0.5) / fwidth(st);
	float line = min(grid.x, grid.y);
	return 1.0 - min(line, 1.0);
}

void main()
{
	// Use world space XZ coordinates for infinite grid
	vec2 worldCoords = v_WorldPosition.xz;
	
	// Calculate grid with anti-aliased lines
	float gridLine = grid(worldCoords * u_Settings.Scale, u_Settings.Size);
	
	// Fade grid based on distance from camera (increased range for larger grid)
	float distanceToCamera = length(v_WorldPosition - u_SceneData.u_CameraPosition);
	float fadeFactor = 1.0 - smoothstep(100.0, 300.0, distanceToCamera);
	
	// Create axis lines with different colors (even thinner lines)
	float xAxisLine = abs(v_WorldPosition.z) < 0.01 ? 1.0 : 0.0;  // Reduced from 0.02 to 0.01
	float zAxisLine = abs(v_WorldPosition.x) < 0.01 ? 1.0 : 0.0;  // Reduced from 0.02 to 0.01
	
	// Base grid color
	vec3 gridColor = vec3(0.2, 0.2, 0.2);
	
	// Highlight axes
	if (xAxisLine > 0.5)
		gridColor = vec3(1.0, 0.3, 0.3); // Red for X axis
	else if (zAxisLine > 0.5)
		gridColor = vec3(0.3, 0.3, 1.0); // Blue for Z axis
	
	// Apply grid pattern and fade
	float alpha = gridLine * 0.5 * fadeFactor;
	if (xAxisLine > 0.5 || zAxisLine > 0.5)
		alpha = max(alpha, fadeFactor);
	
	color = vec4(gridColor, alpha);
	unused = vec4(0.0);
}