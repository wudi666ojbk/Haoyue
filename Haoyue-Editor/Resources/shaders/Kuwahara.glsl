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
	float KernelRadius;      // Brush size (1–10, default 3)
	float ColorLevels;       // Optional posterization (0 = disabled)
	float Padding;
} u_Uniforms;

// ==================== Luminance ====================
float Luminance(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// ==================== Main ====================
void main()
{
	vec2 uv = Input.TexCoord;
	ivec2 texSize = textureSize(u_Texture, 0);
	vec2 texelSize = 1.0 / vec2(texSize);

	int radius = int(round(clamp(u_Uniforms.KernelRadius, 1.0, 15.0)));

	// Accumulators for the 4 quadrants:
	//   [0] = NE (dx >= 0, dy <= 0)   [1] = NW (dx <= 0, dy <= 0)
	//   [2] = SW (dx <= 0, dy >= 0)   [3] = SE (dx >= 0, dy >= 0)
	vec3 mean[4] = vec3[4](vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));
	vec3 sqMean[4] = vec3[4](vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));
	float cnt[4] = float[4](0.0, 0.0, 0.0, 0.0);

	for (int dy = -radius; dy <= radius; dy++)
	{
		for (int dx = -radius; dx <= radius; dx++)
		{
			vec2 sampleUV = uv + vec2(dx, dy) * texelSize;
			vec3 color = texture(u_Texture, sampleUV).rgb;

			// NE quadrant (top-right)
			if (dx >= 0 && dy <= 0)
			{
				mean[0] += color;
				sqMean[0] += color * color;
				cnt[0]++;
			}
			// NW quadrant (top-left)
			if (dx <= 0 && dy <= 0)
			{
				mean[1] += color;
				sqMean[1] += color * color;
				cnt[1]++;
			}
			// SW quadrant (bottom-left)
			if (dx <= 0 && dy >= 0)
			{
				mean[2] += color;
				sqMean[2] += color * color;
				cnt[2]++;
			}
			// SE quadrant (bottom-right)
			if (dx >= 0 && dy >= 0)
			{
				mean[3] += color;
				sqMean[3] += color * color;
				cnt[3]++;
			}
		}
	}

	// Compute variance = E[X²] - E[X]² for each quadrant, then pick best
	int bestIdx = 0;
	float minVariance = 1e10;

	for (int i = 0; i < 4; i++)
	{
		mean[i] /= cnt[i];
		sqMean[i] /= cnt[i];
		vec3 var = abs(sqMean[i] - mean[i] * mean[i]);
		float luminanceVariance = var.r + var.g + var.b;

		if (luminanceVariance < minVariance)
		{
			minVariance = luminanceVariance;
			bestIdx = i;
		}
	}

	vec3 color = mean[bestIdx] * u_Uniforms.Exposure;

	// Optional posterization (adds to the painterly feel)
	if (u_Uniforms.ColorLevels > 1.0)
	{
		color = floor(color * u_Uniforms.ColorLevels) / u_Uniforms.ColorLevels;
		color = clamp(color, 0.0, 1.0);
	}

	// Tonemapping
	const float pureWhite = 1.0;
	float lum = Luminance(color);
	float mappedLum = (lum * (1.0 + lum / (pureWhite * pureWhite))) / (1.0 + lum);
	color = (mappedLum / max(lum, 0.0001)) * color;

	// Gamma correction
	const float gamma = 2.2;
	o_Color = vec4(pow(color, vec3(1.0 / gamma)), 1.0);
}
