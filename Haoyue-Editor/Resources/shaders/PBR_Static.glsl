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
const float PI = 3.141592;                 // 圆周率
const float Epsilon = 0.00001;             // 防止除零的小值

const int LightCount = 1;                  // 光源数量（当前仅支持1个方向光）

// 电介质（非金属）的固定菲涅尔反射率（F0）
// 大多数非金属材料在垂直入射时的反射率约为4%
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
layout(location = 1) out vec4 o_BloomColor; // Bloom效果颜色输出（当前未实际使用，硬编码为紫色）

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
// BRDF（双向反射分布函数）核心函数
// ============================================================================

// ==================== GGX/Trowbridge-Reitz法线分布函数（NDF）====================
// 描述微表面法线分布，控制高光的大小和形状
// 使用Disney的重新参数化：alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// ==================== Schlick-GGX几何函数的单项 ====================
// 用于计算微表面的遮蔽（masking）和遮挡（shadowing）
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// ==================== Schlick-GGX几何衰减函数（Smith方法）====================
// 结合遮蔽和遮挡两项，近似微表面的几何衰减
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic Games建议的粗糙度重映射（用于分析光源）
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

// ==================== Geometry Schlick-GGX（另一种实现）====================
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// ==================== Smith几何函数 ====================
// 结合视线和光线方向的几何衰减（更精确的实现）
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// ==================== Schlick菲涅尔近似 ====================
// 计算不同角度下的反射率，F0是垂直入射时的反射率
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// ==================== 考虑粗糙度的Schlick菲涅尔近似 ====================
// 用于IBL，粗糙度会影响边缘的反射率
vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

// ============================================================================
// 以下代码（来自Unreal Engine 4论文）展示如何为不同粗糙度过滤环境贴图
// 这应该离线计算并存储在立方体贴图的mipmap中，在线运行会导致性能问题
// ============================================================================

// ==================== Van der Corput序列的反向基数 ====================
// 用于生成低差异序列（Hammersley序列的基础）
float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// ==================== Hammersley低差异序列 ====================
// 用于重要性采样，提供均匀分布的样本点
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// ==================== GGX重要性采样 ====================
// 根据粗糙度和法线生成符合GGX分布的采样方向（半向量）
vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );
	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;
	
	// 构建切线空间基向量
	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
	vec3 TangentX = normalize( cross( UpVector, N ) );
	vec3 TangentY = cross( N, TangentX );
	
	// 从切线空间转换到世界空间
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float TotalWeight = 0.0;

// ==================== 预过滤环境贴图（离线版本，当前未使用）====================
// 注意：此函数在当前实现中未实际使用（PrefilteredColor始终为0）
// 正确的实现应该在循环内采样环境贴图并累加
vec3 PrefilterEnvMap(float Roughness, vec3 R)
{
	vec3 N = R;
	vec3 V = R;
	vec3 PrefilteredColor = vec3(0.0);
	int NumSamples = 1024;
	for(int i = 0; i < NumSamples; i++)
	{
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
		vec3 L = 2 * dot(V, H) * H - V; // 反射方向
		float NoL = clamp(dot(N, L), 0.0, 1.0);
		if (NoL > 0)
		{
			//PrefilteredColor += texture(u_EnvRadianceTex, L).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

// ============================================================================

// ==================== 绕Y轴旋转向量 ====================
// 用于环境贴图旋转
vec3 RotateVectorAboutY(float angle, vec3 vec)
{
    angle = radians(angle); // 角度转弧度
    mat3x3 rotationMatrix ={vec3(cos(angle),0.0,sin(angle)),
                            vec3(0.0,1.0,0.0),
                            vec3(-sin(angle),0.0,cos(angle))};
    return rotationMatrix * vec;
}

// ==================== 直接光照计算（Cook-Torrance BRDF）====================
vec3 Lighting(vec3 F0)
{
	vec3 result = vec3(0.0);
	for(int i = 0; i < LightCount; i++)
	{
		vec3 Li = u_DirectionalLights.Direction;          // 光线方向
		vec3 Lradiance = u_DirectionalLights.Radiance * u_DirectionalLights.Multiplier; // 光强度
		vec3 Lh = normalize(Li + m_Params.View);          // 半向量（H = normalize(L + V)）

		// 计算角度余弦值
		float cosLi = max(0.0, dot(m_Params.Normal, Li)); // N·L
		float cosLh = max(0.0, dot(m_Params.Normal, Lh)); // N·H

		// Cook-Torrance BRDF的三个核心项
		vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, m_Params.View))); // 菲涅尔项（Fresnel）
		float D = ndfGGX(cosLh, m_Params.Roughness);                     // 法线分布项（Normal Distribution）
		float G = gaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness); // 几何项（Geometry）

		// 漫反射部分（能量守恒：kd = 1 - F）
		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness); // 金属没有漫反射
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		// 镜面反射部分（Cook-Torrance模型）
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);

		// 累加光照贡献
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}

// ==================== 基于图像的光照（IBL）====================
// 结合环境贴图的漫反射和镜面反射
vec3 IBL(vec3 F0, vec3 Lr)
{
	// 漫反射IBL：采样辐照度贴图
	vec3 irradiance = texture(u_EnvIrradianceTex, m_Params.Normal).rgb;
	
	// 考虑粗糙度的菲涅尔项
	vec3 F = fresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
	
	// 漫反射部分（能量守恒）
	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseIBL = m_Params.Albedo * irradiance;
	
	// 镜面反射IBL：采样预过滤的辐射度贴图
	int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex); // 获取mipmap层级数
	float NoV = clamp(m_Params.NdotV, 0.0, 1.0);
	
	// 计算反射向量并应用环境贴图旋转
	vec3 R = 2.0 * dot(m_Params.View, m_Params.Normal) * m_Params.Normal - m_Params.View;
	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, 
	                                     RotateVectorAboutY(u_MaterialUniforms.EnvMapRotation, Lr), 
	                                     (m_Params.Roughness) * envRadianceTexLevels).rgb;
	
	// 采样BRDF查找表（y坐标用1.0 - roughness是因为贴图是为gloss模型生成的）
	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
	
	// 镜面反射IBL（scale + bias形式）
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
	
	// 合并漫反射和镜面反射
	return kd * diffuseIBL + specularIBL;
}

// ============================================================================
// PCSS（Percentage-Closer Soft Shadows）软阴影算法
// ============================================================================

float ShadowFade = 1.0; // 阴影淡出因子

// ==================== 计算阴影偏差 ====================
// 根据表面法线和光线方向的夹角动态调整偏差，避免阴影痤疮
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
	// 将阴影坐标从[-1,1]转换到[0,1]，并采样阴影贴图
	float shadowMapDepth = texture(shadowMap, vec3(shadowCoords.xy * 0.5 + 0.5, cascade)).x;
	// step函数：如果当前深度 <= 阴影贴图深度+偏差，则不在阴影中（返回1），否则在阴影中（返回0）
	return step(shadowCoords.z, shadowMapDepth + bias) * ShadowFade;
}

// ==================== 半影区域估计 ====================
// 来源：http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float uvLightSize, float receiverDistance)
{
	const float NEAR = 0.1;
	return uvLightSize * (receiverDistance - NEAR) / u_CameraPosition.z;
}

// ==================== 搜索区域半径（UV空间）====================
float SearchRegionRadiusUV(float zWorld)
{
	const float light_zNear = 0.0; // 0.01会产生伪影？可能是因为正交投影
	const float lightRadiusUV = 0.05;
    return lightRadiusUV * (zWorld - light_zNear) / zWorld;
}

// ==================== Poisson圆盘分布（64样本）====================
// 用于PCF和PCSS的随机采样模式
const vec2 PoissonDistribution[64] = vec2[](
	vec2(-0.884081, 0.124488),
	vec2(-0.714377, 0.027940),
	vec2(-0.747945, 0.227922),
	vec2(-0.939609, 0.243634),
	vec2(-0.985465, 0.045534),
	vec2(-0.861367, -0.136222),
	vec2(-0.881934, 0.396908),
	vec2(-0.466938, 0.014526),
	vec2(-0.558207, 0.212662),
	vec2(-0.578447, -0.095822),
	vec2(-0.740266, -0.095631),
	vec2(-0.751681, 0.472604),
	vec2(-0.553147, -0.243177),
	vec2(-0.674762, -0.330730),
	vec2(-0.402765, -0.122087),
	vec2(-0.319776, -0.312166),
	vec2(-0.413923, -0.439757),
	vec2(-0.979153, -0.201245),
	vec2(-0.865579, -0.288695),
	vec2(-0.243704, -0.186378),
	vec2(-0.294920, -0.055748),
	vec2(-0.604452, -0.544251),
	vec2(-0.418056, -0.587679),
	vec2(-0.549156, -0.415877),
	vec2(-0.238080, -0.611761),
	vec2(-0.267004, -0.459702),
	vec2(-0.100006, -0.229116),
	vec2(-0.101928, -0.380382),
	vec2(-0.681467, -0.700773),
	vec2(-0.763488, -0.543386),
	vec2(-0.549030, -0.750749),
	vec2(-0.809045, -0.408738),
	vec2(-0.388134, -0.773448),
	vec2(-0.429392, -0.894892),
	vec2(-0.131597, 0.065058),
	vec2(-0.275002, 0.102922),
	vec2(-0.106117, -0.068327),
	vec2(-0.294586, -0.891515),
	vec2(-0.629418, 0.379387),
	vec2(-0.407257, 0.339748),
	vec2(0.071650, -0.384284),
	vec2(0.022018, -0.263793),
	vec2(0.003879, -0.136073),
	vec2(-0.137533, -0.767844),
	vec2(-0.050874, -0.906068),
	vec2(0.114133, -0.070053),
	vec2(0.163314, -0.217231),
	vec2(-0.100262, -0.587992),
	vec2(-0.004942, 0.125368),
	vec2(0.035302, -0.619310),
	vec2(0.195646, -0.459022),
	vec2(0.303969, -0.346362),
	vec2(-0.678118, 0.685099),
	vec2(-0.628418, 0.507978),
	vec2(-0.508473, 0.458753),
	vec2(0.032134, -0.782030),
	vec2(0.122595, 0.280353),
	vec2(-0.043643, 0.312119),
	vec2(0.132993, 0.085170),
	vec2(-0.192106, 0.285848),
	vec2(0.183621, -0.713242),
	vec2(0.265220, -0.596716),
	vec2(-0.009628, -0.483058),
	vec2(-0.018516, 0.435703)
);

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

// ==================== 采样Poisson分布 ====================
vec2 SamplePoisson(int index)
{
   return PoissonDistribution[index % 64];
}

// ==================== 查找遮挡物距离（PCSS第一步）====================
// 在搜索区域内找到平均遮挡物深度
float FindBlockerDistance_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvLightSize)
{
	float bias = GetShadowBias();

	int numBlockerSearchSamples = 64; // 遮挡物搜索样本数
	int blockers = 0;
	float avgBlockerDistance = 0;

	// 计算搜索区域大小
	float searchWidth = SearchRegionRadiusUV(shadowCoords.z);
	
	// 在搜索区域内采样，寻找遮挡物
	for (int i = 0; i < numBlockerSearchSamples; i++)
	{
		float z = textureLod(shadowMap, vec3((shadowCoords.xy * 0.5 + 0.5) + SamplePoisson(i) * searchWidth, cascade), 0).r;
		if (z < (shadowCoords.z - bias)) // 如果采样点深度小于当前深度，则是遮挡物
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
		return avgBlockerDistance / float(blockers); // 返回平均遮挡物距离

	return -1; // 没有遮挡物
}

// ==================== PCF（Percentage-Closer Filtering）====================
// 在给定半径内进行百分比 closer 滤波，产生软阴影
float PCF_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();
	int numPCFSamples = 64; // PCF样本数
	
	float sum = 0;
	for (int i = 0; i < numPCFSamples; i++)
	{
		vec2 offset = SamplePoisson(i) * uvRadius; // Poisson分布偏移
		float z = textureLod(shadowMap, vec3((shadowCoords.xy * 0.5 + 0.5) + offset, cascade), 0).r;
		sum += step(shadowCoords.z - bias, z); // 累加可见性
	}
	return sum / numPCFSamples; // 返回平均可见性
}

// ==================== NV优化的PCF（16样本）====================
float NV_PCF_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();

	float sum = 0;
	for (int i = 0; i < 16; i++)
	{
		vec2 offset = poissonDisk[i] * uvRadius;
		float z = textureLod(shadowMap, vec3((shadowCoords.xy * 0.5 + 0.5) + offset, cascade), 0).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / 16.0f;
}

// ==================== PCSS（Percentage-Closer Soft Shadows）====================
// 完整的PCSS算法：查找遮挡物 -> 计算半影宽度 -> PCF滤波
float PCSS_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvLightSize)
{
	// 第一步：查找遮挡物距离
	float blockerDistance = FindBlockerDistance_DirectionalLight(shadowMap, cascade, shadowCoords, uvLightSize);
	if (blockerDistance == -1) // 没有遮挡物，完全可见
		return 1.0f;

	// 第二步：计算半影宽度
	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	// 第三步：计算PCF采样半径
	float NEAR = 0.01; 
	float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z;
	uvRadius = min(uvRadius, 0.002f); // 限制最大半径
	
	// 第四步：执行PCF滤波
	return PCF_DirectionalLight(shadowMap, cascade, shadowCoords, uvRadius) * ShadowFade;
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
		// 从法线贴图采样并转换到世界空间
		m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, Input.TexCoord).rgb - 1.0); // [0,1] -> [-1,1]
		m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal); // 切线空间 -> 世界空间
	}
	
	// ==================== 3. 计算视线方向和N·V ====================
	m_Params.View = normalize(u_CameraPosition - Input.WorldPosition);
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
		
	// ==================== 4. 计算反射向量 ====================
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	
	// ==================== 5. 计算F0（垂直入射时的菲涅尔反射率）====================
	// 金属使用反照率作为F0，非金属使用固定值0.04
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	// ==================== 6. 选择阴影级联索引 ====================
	uint cascadeIndex = 0;
	const uint SHADOW_MAP_CASCADE_COUNT = 4;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; i++)
	{
		if(Input.ViewPosition.z < u_CascadeSplits[i]) // 根据视图空间Z值选择级联
			cascadeIndex = i + 1;
	}

	// ==================== 7. 计算阴影淡出因子 ====================
	float shadowDistance = u_MaxShadowDistance;
	float transitionDistance = u_ShadowFade;
	float distance = length(Input.ViewPosition);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0); // 距离越远，阴影越淡
	
	float shadowAmount = 1.0; // 阴影系数（1.0 = 无阴影，0.0 = 完全阴影）

	// ==================== 8. 计算阴影（带级联淡入淡出）====================
	bool fadeCascades = u_CascadeFading;
	if (fadeCascades)
	{
		float cascadeTransitionFade = u_CascadeTransitionFade;
		
		// 计算每个级联边界的平滑过渡因子
		float c0 = smoothstep(u_CascadeSplits[0] + cascadeTransitionFade * 0.5f, u_CascadeSplits[0] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		float c1 = smoothstep(u_CascadeSplits[1] + cascadeTransitionFade * 0.5f, u_CascadeSplits[1] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		float c2 = smoothstep(u_CascadeSplits[2] + cascadeTransitionFade * 0.5f, u_CascadeSplits[2] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		
		if (c0 > 0.0 && c0 < 1.0)
		{
			// 在级联0和1之间混合
			vec3 shadowMapCoords = (Input.ShadowMapCoords[0].xyz / Input.ShadowMapCoords[0].w);
			float shadowAmount0 = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, 0, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 0, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[1].xyz / Input.ShadowMapCoords[1].w);
			float shadowAmount1 = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords);

			shadowAmount = mix(shadowAmount0, shadowAmount1, c0);
		}
		else if (c1 > 0.0 && c1 < 1.0)
		{
			// 在级联1和2之间混合
			vec3 shadowMapCoords = (Input.ShadowMapCoords[1].xyz / Input.ShadowMapCoords[1].w);
			float shadowAmount1 = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 1, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[2].xyz / Input.ShadowMapCoords[2].w);
			float shadowAmount2 = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords);

			shadowAmount = mix(shadowAmount1, shadowAmount2, c1);
		}
		else if (c2 > 0.0 && c2 < 1.0)
		{
			// 在级联2和3之间混合
			vec3 shadowMapCoords = (Input.ShadowMapCoords[2].xyz / Input.ShadowMapCoords[2].w);
			float shadowAmount2 = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 2, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[3].xyz / Input.ShadowMapCoords[3].w);
			float shadowAmount3 = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, 3, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, 3, shadowMapCoords);

			shadowAmount = mix(shadowAmount2, shadowAmount3, c2);
		}
		else
		{
			// 不在过渡区域，直接使用选定的级联
			vec3 shadowMapCoords = (Input.ShadowMapCoords[cascadeIndex].xyz / Input.ShadowMapCoords[cascadeIndex].w);
			shadowAmount = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords);
		}
	}
	else
	{
		// 不使用级联淡入淡出，直接使用选定的级联
		vec3 shadowMapCoords = (Input.ShadowMapCoords[cascadeIndex].xyz / Input.ShadowMapCoords[cascadeIndex].w);
		shadowAmount = u_SoftShadows ? PCSS_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords, u_LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords);
	}

	// ==================== 9. 计算最终颜色 ====================
	vec3 lightContribution = Lighting(F0) * shadowAmount; // 直接光照 * 阴影系数
	vec3 iblContribution = IBL(F0, Lr);                   // 基于图像的光照

	color = vec4(iblContribution + lightContribution, 1.0); // 合并直接光和IBL

	o_BloomColor = vec4(1.0, 0.0, 1.0, 1.0); // Bloom输出（当前未使用）

	// ==================== 10. 级联调试可视化（可选）====================
	if (u_ShowCascades)
	{
		switch(cascadeIndex)
		{
		case 0:
			color.rgb *= vec3(1.0f, 0.25f, 0.25f); // 红色 tint
			break;
		case 1:
			color.rgb *= vec3(0.25f, 1.0f, 0.25f); // 绿色 tint
			break;
		case 2:
			color.rgb *= vec3(0.25f, 0.25f, 1.0f); // 蓝色 tint
			break;
		case 3:
			color.rgb *= vec3(1.0f, 1.0f, 0.25f); // 黄色 tint
			break;
		}
	}
}