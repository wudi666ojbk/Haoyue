#type vertex
#version 450 core

// ==================== 顶点着色器输入属性 ====================
layout(location = 0) in vec3 a_Position;    // 顶点位置
layout(location = 1) in vec3 a_Normal;      // 顶点法线
layout(location = 2) in vec3 a_Tangent;     // 顶点切线（用于法线贴图）
layout(location = 3) in vec3 a_Binormal;    // 顶点副法线（用于法线贴图）
layout(location = 4) in vec2 a_TexCoord;    // 纹理坐标

// ==================== 相机Uniform缓冲（binding = 0）====================
layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;           // 视图投影矩阵（View * Projection）
	mat4 u_InverseViewProjectionMatrix;    // 逆视图投影矩阵
	mat4 u_ViewMatrix;                     // 视图矩阵
};

// ==================== 阴影数据Uniform缓冲（binding = 1）====================
layout (std140, binding = 1) uniform ShadowData
{
	mat4 u_LightMatrix[4];                 // 4个级联阴影的光空间变换矩阵（LightViewProjection）
};

// ==================== 推送常量：模型变换矩阵 ====================
layout (push_constant) uniform Transform
{
	mat4 Transform;                        // 模型的世界变换矩阵
} u_Renderer;

// ==================== 顶点着色器输出结构 ====================
struct VertexOutput
{
	vec3 WorldPosition;                    // 世界空间位置
    vec3 Normal;                           // 世界空间法线
	vec2 TexCoord;                         // 纹理坐标
	mat3 WorldNormals;                     // 世界空间TBN矩阵（切线-副法线-法线，用于法线贴图转换）
	mat3 WorldTransform;                   // 世界变换矩阵（3x3，仅旋转部分）
	vec3 Binormal;                         // 副法线

	vec4 ShadowMapCoords[4];               // 4个级联的阴影贴图坐标（齐次坐标）
	vec3 ViewPosition;                     // 视图空间位置
};

layout (location = 0) out VertexOutput Output;

void main()
{
	// 计算世界空间位置
	Output.WorldPosition = vec3(u_Renderer.Transform * vec4(a_Position, 1.0));
	
	// 转换法线到世界空间（只使用旋转部分，避免缩放影响）
    Output.Normal = mat3(u_Renderer.Transform) * a_Normal;
	
	// 翻转Y轴纹理坐标（OpenGL坐标系转换：Y轴向上 vs Vulkan/DirectX Y轴向下）
	Output.TexCoord = vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	
	// 构建世界空间的TBN矩阵（用于将法线贴图的切线空间法线转换到世界空间）
	Output.WorldNormals = mat3(u_Renderer.Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	
	// 存储世界变换矩阵（3x3）
	Output.WorldTransform = mat3(u_Renderer.Transform);
	Output.Binormal = a_Binormal;

	// 计算4个级联阴影贴图的坐标（将世界位置变换到每个级联的光空间）
	Output.ShadowMapCoords[0] = u_LightMatrix[0] * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoords[1] = u_LightMatrix[1] * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoords[2] = u_LightMatrix[2] * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoords[3] = u_LightMatrix[3] * vec4(Output.WorldPosition, 1.0);
	
	// 计算视图空间位置（用于距离计算和级联选择）
	Output.ViewPosition = vec3(u_ViewMatrix * vec4(Output.WorldPosition, 1.0));

	// 计算最终的裁剪空间位置（用于光栅化）
	gl_Position = u_ViewProjectionMatrix * u_Renderer.Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

// ==================== 数学常量 ====================
const float PI = 3.14159265358979;         // 圆周率
const float Epsilon = 0.00001;             // 防止除零的小值

const int LightCount = 1;                  // 光源数量（当前仅支持1个方向光）

// 电介质（非金属）的固定菲涅尔反射率（F0）
const vec3 Fdielectric = vec3(0.04);

// ==================== 方向光结构体 ====================
struct DirectionalLight
{
	vec3 Direction;                        // 光线方向（从表面指向光源）
	vec3 Radiance;                         // 辐射度（光的颜色/强度）
	float Multiplier;                      // 强度乘数
};

// ==================== 片元着色器输入结构（与顶点输出对应）====================
struct VertexOutput
{
	vec3 WorldPosition;                    // 世界空间位置
    vec3 Normal;                           // 世界空间法线
	vec2 TexCoord;                         // 纹理坐标
	mat3 WorldNormals;                     // 世界空间TBN矩阵
	mat3 WorldTransform;                   // 世界变换矩阵
	vec3 Binormal;                         // 副法线

	vec4 ShadowMapCoords[4];               // 4个级联的阴影贴图坐标
	vec3 ViewPosition;                     // 视图空间位置
};

layout (location = 0) in VertexOutput Input;

// ==================== 片元着色器输出 ====================
layout(location = 0) out vec4 color;       // 最终颜色输出
layout(location = 1) out vec4 o_BloomColor; // Bloom效果颜色输出

// ==================== 场景数据Uniform缓冲（binding = 2）====================
layout (std140, binding = 2) uniform SceneData
{
	DirectionalLight u_DirectionalLights;  // 方向光数据
	vec3 u_CameraPosition;                 // 相机世界位置（偏移量32字节）
	bool u_HasEnvironmentMap;              // 是否有环境贴图
};

// ==================== 渲染器数据Uniform缓冲（binding = 3）====================
layout (std140, binding = 3) uniform RendererData
{
	vec4 u_CascadeSplits;                  // 4个级联的分界距离（视图空间Z值）
	bool u_ShowCascades;                   // 是否显示级联调试颜色
	bool u_SoftShadows;                    // 是否启用软阴影（PCSS）
	float u_LightSize;                     // 光源大小（影响半影宽度，值越大阴影越柔和）
	float u_MaxShadowDistance;             // 最大阴影距离
	float u_ShadowFade;                    // 阴影淡出距离
	bool u_CascadeFading;                  // 是否启用级联淡入淡出
	float u_CascadeTransitionFade;         // 级联过渡淡入淡出强度
};

// ==================== PBR纹理采样器（set = 0）====================
layout (set = 0, binding = 4) uniform sampler2D u_AlbedoTexture;    // 反照率/基础色贴图
layout (set = 0, binding = 5) uniform sampler2D u_NormalTexture;    // 法线贴图
layout (set = 0, binding = 6) uniform sampler2D u_MetalnessTexture; // 金属度贴图
layout (set = 0, binding = 7) uniform sampler2D u_RoughnessTexture; // 粗糙度贴图

// ==================== 环境贴图采样器（set = 1）====================
layout (set = 1, binding = 8) uniform samplerCube u_EnvRadianceTex;   // 预过滤的环境辐射贴图（不同粗糙度的mipmap级别）
layout (set = 1, binding = 9) uniform samplerCube u_EnvIrradianceTex; // 环境辐照度贴图（用于漫反射IBL）

// ==================== BRDF查找表 ====================
layout (set = 1, binding = 10) uniform sampler2D u_BRDFLUTTexture;    // BRDF积分查找表（用于IBL specular）

// ==================== 阴影贴图 ====================
layout (set = 1, binding = 11) uniform sampler2DArray u_ShadowMapTexture; // 阴影贴图数组（4个级联层）

// ==================== 材质推送常量 ====================
layout (push_constant) uniform Material
{
	layout (offset = 64) vec3 AlbedoColor;  // 反照率颜色（与贴图相乘）
	float Metalness;                        // 金属度（0.0 = 非金属，1.0 = 金属）
	float Roughness;                        // 粗糙度（0.0 = 光滑，1.0 = 粗糙）

	float EnvMapRotation;                   // 环境贴图旋转角度（度）

	bool UseNormalMap;                      // 是否使用法线贴图
	
	// Cartoon rendering parameters
	int ToonLevels;                         // 卡通色调分离级别数（通常3-5）
	float SpecularIntensity;                // 高光强度（0.0-2.0）
	float RimLightIntensity;                // 边缘光强度（0.0-2.0）
	vec3 OutlineColor;                      // 描边颜色
	float RimPower;                         // 边缘光幂次（控制边缘光的集中度，默认3.0）
	float SpecularThreshold;                // 高光阈值（控制高光区域的锐利程度，默认0.5）
	float AmbientIntensity;                 // 环境光强度（0.0-1.0，默认0.3）
} u_MaterialUniforms;

// ==================== PBR参数结构 ====================
struct PBRParameters
{
	vec3 Albedo;                            // 反照率（表面颜色）
	float Roughness;                        // 粗糙度
	float Metalness;                        // 金属度

	vec3 Normal;                            // 表面法线（世界空间）
	vec3 View;                              // 视线方向（从表面指向相机）
	float NdotV;                            // 法线与视线的点积（N·V）
};

PBRParameters m_Params;                     // 全局PBR参数实例

// ============================================================================
// 卡通渲染核心函数
// ============================================================================

// ==================== 色调分离函数（Cartoon Quantization）====================
// 将连续的光照值离散化为几个级别，产生卡通风格的色块效果
float ToonShade(float intensity, int levels)
{
	if (levels <= 1) return intensity;
	// 将[0,1]范围的值分成levels个离散级别
	return floor(intensity * float(levels)) / float(levels);
}

// ==================== 向量色调分离 ====================
vec3 ToonShadeVec3(vec3 color, int levels)
{
	if (levels <= 1) return color;
	return floor(color * float(levels)) / float(levels);
}

// ==================== 平滑色调分离（带抗锯齿）====================
float SmoothToonShade(float intensity, int levels, float smoothness)
{
	if (levels <= 1) return intensity;
	float quantized = floor(intensity * float(levels)) / float(levels);
	float nextLevel = quantized + 1.0 / float(levels);
	float t = smoothstep(quantized - smoothness, quantized + smoothness, intensity);
	return mix(quantized, nextLevel, t);
}

// ==================== Schlick菲涅尔近似 ====================
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ==================== 考虑粗糙度的Schlick菲涅尔近似 ====================
vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

// ==================== 绕Y轴旋转向量 ====================
vec3 RotateVectorAboutY(float angle, vec3 vec)
{
    angle = radians(angle);
    float c = cos(angle);
    float s = sin(angle);
    mat3 rotationMatrix = mat3(
        vec3(c, 0.0, s),
        vec3(0.0, 1.0, 0.0),
        vec3(-s, 0.0, c)
    );
    return rotationMatrix * vec;
}

// ==================== 计算亮度 ====================
float CalculateLuminance(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// ==================== 直接光照计算（卡通风格）====================
vec3 CartoonLighting(vec3 F0)
{
	vec3 result = vec3(0.0);
	for(int i = 0; i < LightCount; i++)
	{
		vec3 Li = normalize(u_DirectionalLights.Direction);          // 光线方向
		vec3 Lradiance = u_DirectionalLights.Radiance * u_DirectionalLights.Multiplier; // 光强度
		
		// 计算漫反射（兰伯特光照）
		float NdotL = max(dot(m_Params.Normal, Li), 0.0);
		
		// 应用色调分离到漫反射系数
		float toonDiffuse = ToonShade(NdotL, u_MaterialUniforms.ToonLevels);
		
		// 漫反射部分（保持与PBR一致的结构）
		vec3 kd = (1.0 - F0) * (1.0 - m_Params.Metalness);
		vec3 diffuse = kd * m_Params.Albedo * toonDiffuse;
		
		// 计算高光（Blinn-Phong）
		vec3 H = normalize(Li + m_Params.View);           // 半向量
		float NdotH = max(dot(m_Params.Normal, H), 0.0);
		
		// 卡通风格的高光：更锐利、更集中的高光区域
		float shininess = 32.0 * (1.0 - m_Params.Roughness) + 1.0; // 根据粗糙度调整光泽度
		float specularFactor = pow(NdotH, shininess);
		
		// 对高光进行阈值处理，产生清晰的卡通高光
		float toonSpecular = step(u_MaterialUniforms.SpecularThreshold, specularFactor) * specularFactor;
		
		// 菲涅尔项（用于控制高光强度）
		vec3 F = fresnelSchlick(F0, max(dot(H, m_Params.View), 0.0));
		
		// 合并高光和漫反射，并乘以光照强度
		vec3 specular = F * toonSpecular * u_MaterialUniforms.SpecularIntensity;
		
		result += (diffuse + specular) * Lradiance;
	}
	return result;
}

// ==================== 基于图像的光照（IBL - 卡通风格简化版）====================
vec3 CartoonIBL(vec3 F0, vec3 Lr)
{
	vec3 result = vec3(0.0);
	
	// 只有在有环境贴图时才计算IBL
	if (u_HasEnvironmentMap)
	{
		// 漫反射IBL：采样辐照度贴图
		vec3 irradiance = texture(u_EnvIrradianceTex, m_Params.Normal).rgb;
		
		// 对环境光也进行色调分离
		vec3 toonIrradiance = ToonShadeVec3(irradiance, u_MaterialUniforms.ToonLevels);
		
		// 考虑粗糙度的菲涅尔项
		vec3 F = fresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
		
		// 漫反射部分（能量守恒）
		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
		vec3 diffuseIBL = kd * m_Params.Albedo * toonIrradiance * u_MaterialUniforms.AmbientIntensity;
		
		// 边缘光（Rim Light）效果
		float rimFactor = 1.0 - m_Params.NdotV;
		rimFactor = pow(rimFactor, u_MaterialUniforms.RimPower); // 使边缘光更加集中在轮廓处
		vec3 rimLight = vec3(1.0) * rimFactor * u_MaterialUniforms.RimLightIntensity;
		
		// 镜面反射IBL（简化版）
		int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
		vec3 specularIrradiance = textureLod(u_EnvRadianceTex, 
		                                     RotateVectorAboutY(u_MaterialUniforms.EnvMapRotation, Lr), 
		                                     m_Params.Roughness * float(envRadianceTexLevels)).rgb;
		
		vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
		vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
		
		// 对镜面反射也进行色调分离
		specularIBL = ToonShadeVec3(specularIBL, u_MaterialUniforms.ToonLevels);
		
		result = diffuseIBL + specularIBL + rimLight;
	}
	else
	{
		// 没有环境贴图时，使用简单的环境光
		vec3 ambient = m_Params.Albedo * u_MaterialUniforms.AmbientIntensity * 0.3;
		result = ambient;
	}
	
	return result;
}

// ============================================================================
// PCSS（Percentage-Closer Soft Shadows）软阴影算法
// ============================================================================

float ShadowFade = 1.0; // 阴影淡出因子

// ==================== 计算阴影偏差 ====================
float GetShadowBias()
{
	const float MINIMUM_SHADOW_BIAS = 0.002;
	float bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(m_Params.Normal, u_DirectionalLights.Direction)), MINIMUM_SHADOW_BIAS);
	return bias;
}

// ==================== 硬阴影测试 ====================
float HardShadows_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords)
{
	float bias = GetShadowBias();
	float shadowMapDepth = texture(shadowMap, vec3(shadowCoords.xy * 0.5 + 0.5, cascade)).x;
	return step(shadowCoords.z, shadowMapDepth + bias) * ShadowFade;
}

// ==================== Poisson圆盘分布（16样本，NV优化版）====================
const vec2 poissonDisk[16] = vec2[](
 vec2( -0.94201624, -0.39906216 ),
 vec2( 0.94558609, -0.76890725 ),
 vec2( -0.094184101, -0.92938870 ),
 vec2( 0.34495938, 0.29387760 ),
 vec2( -0.91588581, 0.45771432 ),
 vec2( -0.81544232, -0.87912464 ),
 vec2( -0.38277543, 0.27676845 ),
 vec2( 0.97484398, 0.75648379 ),
 vec2( 0.44323325, -0.97511554 ),
 vec2( 0.53742981, -0.47373420 ),
 vec2( -0.26496911, -0.41893023 ),
 vec2( 0.79197514, 0.19090188 ),
 vec2( -0.24188840, 0.99706507 ),
 vec2( -0.81409955, 0.91437590 ),
 vec2( 0.19984126, 0.78641367 ),
 vec2( 0.14383161, -0.14100790 )
); 

// ==================== NV优化的PCF（16样本）====================
float NV_PCF_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();

	float sum = 0.0;
	for (int i = 0; i < 16; i++)
	{
		vec2 offset = poissonDisk[i] * uvRadius;
		float z = textureLod(shadowMap, vec3((shadowCoords.xy * 0.5 + 0.5) + offset, cascade), 0).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / 16.0;
}

// ============================================================================
// 主函数
// ============================================================================

void main()
{
	// ==================== 1. 采样PBR材质参数 ====================
	m_Params.Albedo = texture(u_AlbedoTexture, Input.TexCoord).rgb * u_MaterialUniforms.AlbedoColor; 
	m_Params.Metalness = texture(u_MetalnessTexture, Input.TexCoord).r * u_MaterialUniforms.Metalness;
	m_Params.Roughness = texture(u_RoughnessTexture, Input.TexCoord).r * u_MaterialUniforms.Roughness;
    m_Params.Roughness = max(m_Params.Roughness, 0.05); // 最小粗糙度0.05，保持高光可见

	// ==================== 2. 计算表面法线 ====================
	m_Params.Normal = normalize(Input.Normal);
	if (u_MaterialUniforms.UseNormalMap)
	{
		m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, Input.TexCoord).rgb - 1.0);
		m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal);
	}
	
	// ==================== 3. 计算视线方向和N·V ====================
	m_Params.View = normalize(u_CameraPosition - Input.WorldPosition);
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
		
	// ==================== 4. 计算反射向量 ====================
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	
	// ==================== 5. 计算F0（垂直入射时的菲涅尔反射率）====================
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	// ==================== 6. 选择阴影级联索引 ====================
	uint cascadeIndex = 0;
	const uint SHADOW_MAP_CASCADE_COUNT = 4;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; i++)
	{
		if(Input.ViewPosition.z < u_CascadeSplits[i])
			cascadeIndex = i + 1;
	}

	// ==================== 7. 计算阴影淡出因子 ====================
	float shadowDistance = u_MaxShadowDistance;
	float transitionDistance = u_ShadowFade;
	float distance = length(Input.ViewPosition);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0);
	
	float shadowAmount = 1.0;

	// ==================== 8. 计算阴影 ====================
	bool fadeCascades = u_CascadeFading;
	if (fadeCascades)
	{
		float cascadeTransitionFade = u_CascadeTransitionFade;
		
		float c0 = smoothstep(u_CascadeSplits[0] + cascadeTransitionFade * 0.5f, u_CascadeSplits[0] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		float c1 = smoothstep(u_CascadeSplits[1] + cascadeTransitionFade * 0.5f, u_CascadeSplits[1] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		float c2 = smoothstep(u_CascadeSplits[2] + cascadeTransitionFade * 0.5f, u_CascadeSplits[2] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		
		if (c0 > 0.0 && c0 < 1.0)
		{
			vec3 shadowMapCoords = (Input.ShadowMapCoords[0].xyz / Input.ShadowMapCoords[0].w);
			float shadowAmount0 = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, 0, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 0, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[1].xyz / Input.ShadowMapCoords[1].w);
			float shadowAmount1 = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords);

			shadowAmount = mix(shadowAmount0, shadowAmount1, c0);
		}
		else if (c1 > 0.0 && c1 < 1.0)
		{
			vec3 shadowMapCoords = (Input.ShadowMapCoords[1].xyz / Input.ShadowMapCoords[1].w);
			float shadowAmount1 = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[2].xyz / Input.ShadowMapCoords[2].w);
			float shadowAmount2 = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords);

			shadowAmount = mix(shadowAmount1, shadowAmount2, c1);
		}
		else if (c2 > 0.0 && c2 < 1.0)
		{
			vec3 shadowMapCoords = (Input.ShadowMapCoords[2].xyz / Input.ShadowMapCoords[2].w);
			float shadowAmount2 = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[3].xyz / Input.ShadowMapCoords[3].w);
			float shadowAmount3 = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, 3, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 3, shadowMapCoords);

			shadowAmount = mix(shadowAmount2, shadowAmount3, c2);
		}
		else
		{
			vec3 shadowMapCoords = (Input.ShadowMapCoords[cascadeIndex].xyz / Input.ShadowMapCoords[cascadeIndex].w);
			shadowAmount = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords);
		}
	}
	else
	{
		vec3 shadowMapCoords = (Input.ShadowMapCoords[cascadeIndex].xyz / Input.ShadowMapCoords[cascadeIndex].w);
		shadowAmount = u_SoftShadows ? NV_PCF_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords);
	}

	// ==================== 9. 计算最终颜色（卡通渲染）====================
	vec3 lightContribution = CartoonLighting(F0) * shadowAmount; // 卡通直接光照 * 阴影系数
	vec3 iblContribution = CartoonIBL(F0, Lr);                   // 卡通IBL

	color = vec4(iblContribution + lightContribution, 1.0);

	o_BloomColor = vec4(0.0); // Bloom输出（卡通渲染通常不需要bloom）

	// ==================== 10. 级联调试可视化（可选）====================
	if (u_ShowCascades)
	{
		switch(cascadeIndex)
		{
		case 0:
			color.rgb *= vec3(1.0f, 0.25f, 0.25f);
			break;
		case 1:
			color.rgb *= vec3(0.25f, 1.0f, 0.25f);
			break;
		case 2:
			color.rgb *= vec3(0.25f, 0.25f, 1.0f);
			break;
		case 3:
			color.rgb *= vec3(1.0f, 1.0f, 0.25f);
			break;
		}
	}
}
