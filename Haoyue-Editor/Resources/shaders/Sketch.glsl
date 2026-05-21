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
	float Exposure;           // Exposure compensation
	float HatchDensity;       // Line spacing in pixels (4-30, default 10)
	float HatchIntensity;     // Hatching darkness (0-2, default 1.0)
	float EdgeStrength;       // Edge detection strength (0-2, default 0.8)
	vec3 InkColor;            // Pen/ink color (default: dark blue-black)
	vec3 PaperColor;          // Paper background color (default: warm off-white)
} u_Uniforms;

// ==================== Helper: luminance ====================
float Luminance(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// ==================== Hatching: generates parallel lines at a given angle ====================
// Returns 1.0 on a line, 0.0 between lines
float HatchLine(vec2 pixelUV, float spacing, float angle)
{
	float c = cos(angle);
	float s = sin(angle);
	vec2 rotated = vec2(pixelUV.x * c - pixelUV.y * s, pixelUV.x * s + pixelUV.y * c);

	float linePos = fract(rotated.x / max(spacing, 0.5));

	// Smooth line profile (thin dark line, mostly white between)
	float lineWidth = 0.35; // fraction of spacing occupied by line
	return 1.0 - smoothstep(0.0, lineWidth, min(linePos, 1.0 - linePos));
}

// ==================== Sobel edge detection on luminance ====================
float SobelEdge(vec2 uv, vec2 texelSize)
{
	float tl = Luminance(texture(u_Texture, uv + vec2(-1, -1) * texelSize).rgb);
	float t  = Luminance(texture(u_Texture, uv + vec2( 0, -1) * texelSize).rgb);
	float tr = Luminance(texture(u_Texture, uv + vec2( 1, -1) * texelSize).rgb);
	float l  = Luminance(texture(u_Texture, uv + vec2(-1,  0) * texelSize).rgb);
	float r  = Luminance(texture(u_Texture, uv + vec2( 1,  0) * texelSize).rgb);
	float bl = Luminance(texture(u_Texture, uv + vec2(-1,  1) * texelSize).rgb);
	float b  = Luminance(texture(u_Texture, uv + vec2( 0,  1) * texelSize).rgb);
	float br = Luminance(texture(u_Texture, uv + vec2( 1,  1) * texelSize).rgb);

	float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
	float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

	return sqrt(gx * gx + gy * gy);
}

// ==================== Main ====================
void main()
{
	vec2 uv = Input.TexCoord;
	vec2 texelSize = 1.0 / vec2(textureSize(u_Texture, 0));

	// Sample scene color
	vec3 sceneColor = texture(u_Texture, uv).rgb * u_Uniforms.Exposure;

	// Compute luminance (0 = dark/black, 1 = bright/white)
	float lum = Luminance(sceneColor);
	float invLum = 1.0 - lum; // dark areas need more hatching

	// Pixel-level coordinates for consistent hatching
	vec2 pixelUV = uv * vec2(textureSize(u_Texture, 0));
	float spacing = u_Uniforms.HatchDensity;

	// === Step 1: Multi-angle cross-hatching ===
	// Three hatching layers at 0, 60, and 120 degrees
	// Layer thresholds: darker areas get more layers
	float hatch0 = HatchLine(pixelUV, spacing, 0.0);
	float hatch60 = HatchLine(pixelUV, spacing, 1.0472);   // 60 degrees
	float hatch120 = HatchLine(pixelUV, spacing, 2.0944);  // 120 degrees

	// Combine hatching layers based on luminance:
	//   Light (lum > 0.6):  no hatching
	//   Mid    (0.3-0.6):   1 layer
	//   Dark   (0.15-0.3):  2 layers
	//   Shadow (< 0.15):    3 layers
	float hatchMask = 0.0;

	// First layer activates for non-bright areas
	float t1 = smoothstep(0.55, 0.3, lum);
	hatchMask += t1 * hatch0;

	// Second layer activates for darker areas
	float t2 = smoothstep(0.3, 0.12, lum);
	hatchMask += t2 * hatch60;

	// Third layer activates for very dark areas
	float t3 = smoothstep(0.12, 0.0, lum);
	hatchMask += t3 * hatch120;

	// Clamp and apply intensity
	hatchMask = clamp(hatchMask * u_Uniforms.HatchIntensity, 0.0, 1.0);

	// === Step 2: Edge detection for pen outlines ===
	float edgeIntensity = SobelEdge(uv, texelSize);
	float edgeMask = smoothstep(0.06, 0.18, edgeIntensity);
	edgeMask = clamp(edgeMask * u_Uniforms.EdgeStrength, 0.0, 1.0);

	// === Step 3: Compose final image ===
	// Start with paper color
	vec3 result = u_Uniforms.PaperColor;

	// Blend hatching ink lines over paper
	result = mix(result, u_Uniforms.InkColor, hatchMask);

	// Overlay edge lines (always ink color, on top of everything)
	result = mix(result, u_Uniforms.InkColor, edgeMask);

	// Subtle hint of original color for artistic feel (watercolor wash)
	float colorHint = 0.12;
	result = mix(result, sceneColor * 0.5 + result * 0.5, colorHint);

	o_Color = vec4(result, 1.0);
}
