#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) out OutputBlock Output;

void main()
{
	vec4 position = vec4(a_Position.xy, 0.0, 1.0);
	Output.TexCoord = a_TexCoord;
	gl_Position = position;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) in OutputBlock Input;

layout (binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform Uniforms
{
	float Exposure;
	float PixelDensity;     // Number of virtual pixels horizontally (e.g. 80, 120, 160, 256)
	float ColorLevels;       // Number of discrete color levels per channel (0 = disabled)
	float PixelAspectRatio;  // Pixel aspect ratio (1.0 = square, 0.5 = wide retro pixels)
} u_Uniforms;

void main()
{
	const float gamma     = 2.2;
	const float pureWhite = 1.0;

	// === Step 1: Pixelation via UV coordinate quantization ===

	// Compute effective grid resolution based on PixelDensity and aspect ratio
	float density = max(u_Uniforms.PixelDensity, 4.0);
	float gridX = density;
	float gridY = density;

	// Quantize UV coordinates: snap to the nearest low-resolution grid cell
	vec2 pixelCoord = floor(Input.TexCoord * vec2(gridX, gridY)) / vec2(gridX, gridY);

	// Sample from the center of the chosen cell (nearest-neighbor feel)
	vec2 sampleCoord = pixelCoord + 0.5 / vec2(gridX, gridY);

	// Sample the scene texture at the quantized coordinate
	vec3 color = texture(u_Texture, sampleCoord).rgb * u_Uniforms.Exposure;

	// === Step 2: Optional color quantization (posterization) ===

	if (u_Uniforms.ColorLevels > 1.0)
	{
		float levels = u_Uniforms.ColorLevels;
		color = floor(color * levels) / levels;

		// Clamp to prevent overshoot
		color = clamp(color, 0.0, 1.0);
	}

	// === Step 3: Reinhard tonemapping ===

	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances
	vec3 mappedColor = (mappedLuminance / max(luminance, 0.0001)) * color;

	// === Step 4: Gamma correction ===

	o_Color = vec4(pow(mappedColor, vec3(1.0 / gamma)), 1.0);
}
