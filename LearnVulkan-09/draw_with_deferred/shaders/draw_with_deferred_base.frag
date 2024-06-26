#version 450

// Use this constant to control the flow of the shader depending on the SPEC_CONSTANTS value 
// selected at pipeline creation time
layout (constant_id = 0) const int SPEC_CONSTANTS = 0;

// push constants block
layout( push_constant ) uniform constants
{
	float time;
	float roughness;
	float metallic;
	uint specConstants;
	uint specConstantsCount;
} global;

struct light
{
	vec4 position;  // position.w represents type of light
	vec4 color;     // color.w represents light intensity
	vec4 direction; // direction.w represents range
	vec4 info;      // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

layout(set = 0, binding = 1) uniform uniformbuffer
{
	mat4 shadowmapSpace;
	mat4 localToWorld;
	vec4 cameraInfo;
	light directionalLights[16];
	light pointLights[512];
	light spotLights[16];
	ivec4 lightsCount; // [0] for directionalLights, [1] for pointLights, [2] for spotLights
	float zNear;
	float zFar;
} view;

uint DIRECTIONAL_LIGHTS = view.lightsCount[0];
uint POINT_LIGHTS = view.lightsCount[1];
uint SPOT_LIGHTS = view.lightsCount[2];
uint SKY_MAXMIPS = view.lightsCount[3];

layout(set = 0, binding = 2)  uniform samplerCube cubemap;  // sky cubemap
layout(set = 0, binding = 3)  uniform sampler2D shadowmap;  // sky cubemap
layout(set = 0, binding = 4)  uniform sampler2D sampler1; // basecolor
layout(set = 0, binding = 5)  uniform sampler2D sampler2; // metalic
layout(set = 0, binding = 6)  uniform sampler2D sampler3; // roughness
layout(set = 0, binding = 7)  uniform sampler2D sampler4; // normalmap
layout(set = 0, binding = 8)  uniform sampler2D sampler5; // ambient occlution
layout(set = 0, binding = 9)  uniform sampler2D sampler6; // emissive
layout(set = 0, binding = 10) uniform sampler2D sampler7; // mask

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


const float PI = 3.14159265359;
vec3 F0 = vec3(0.04);

float saturate(float t)
{
	return clamp(t, 0.0, 1.0);
}


vec3 saturate(vec3 t)
{
	return clamp(t, 0.0, 1.0);
}


float lerp(float f1, float f2, float a)
{
	return ((1.0 - a) * f1 + a * f2);
}


vec3 lerp(vec3 v1, vec3 v2, float a)
{
	return ((1.0 - a) * v1 + a * v2);
}


float remap(float value, float inputMin, float inputMax, float outputMin, float outputMax)
{
	value = clamp(value, inputMin, inputMax);
	return (value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin;
}


vec3 ComputeNormal()
{
	vec3 pos_dx = dFdx(fragPosition);
	vec3 pos_dy = dFdy(fragPosition);
	vec3 st1    = dFdx(vec3(fragTexCoord, 0.0));
	vec3 st2    = dFdy(vec3(fragTexCoord, 0.0));
	vec3 T      = (st2.t * pos_dx - st1.t * pos_dy) / (st1.s * st2.t - st2.s * st1.t);
	vec3 N      = normalize(fragNormal);
	T           = normalize(T - N * dot(N, T));
	vec3 B      = normalize(cross(N, T));
	mat3 TBN    = mat3(T, B, N);
	
	return normalize(TBN[2].xyz);
}


vec3 ComputeNormal(vec3 n)
{
	vec3 pos_dx = dFdx(fragPosition);
	vec3 pos_dy = dFdy(fragPosition);
	vec3 st1    = dFdx(vec3(fragTexCoord, 0.0));
	vec3 st2    = dFdy(vec3(fragTexCoord, 0.0));
	vec3 T      = (st2.t * pos_dx - st1.t * pos_dy) / (st1.s * st2.t - st2.s * st1.t);
	vec3 N      = normalize(fragNormal);
	T           = normalize(T - N * dot(N, T));
	vec3 B      = normalize(cross(N, T));
	mat3 TBN    = mat3(T, B, N);

	return normalize(TBN * normalize(2.0 * n - 1.0));
}


vec3 GetDirectionalLightDirection(uint index)
{
	return normalize(view.directionalLights[index].direction.xyz);
}

vec3 GetDirectionalLightColor(uint index)
{
	return view.directionalLights[index].color.rgb;
}

float GetDirectionalLightIntensity(uint index)
{
	return view.directionalLights[index].color.w;
}

vec3 ApplyDirectionalLight(uint index, vec3 n)
{
	vec3 l = GetDirectionalLightDirection(index);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	float density = GetDirectionalLightIntensity(index);
	vec3 color = GetDirectionalLightColor(index);
	return ndotl * density * color;
}


vec3 GetPointLightPosition(uint index)
{
	return view.pointLights[index].position.xyz;
}

vec3 GetPointLightDirection(uint index, vec3 pos)
{
	return normalize(view.pointLights[index].position.xyz - pos);
}

float GetPointLightFalloff(uint index)
{
	return view.pointLights[index].direction.w;
}

vec3 GetPointLightColor(uint index)
{
	return view.pointLights[index].color.rgb;
}

float GetPointLightIntensity(uint index)
{
	return view.pointLights[index].color.w;
}

vec3 ApplyPointLight(uint index, vec3 pos, vec3 n)
{
	vec3 l = GetPointLightDirection(index, pos);

	vec3 light_pos = GetPointLightPosition(index);
	float falloff = GetPointLightFalloff(index);
	float density = GetPointLightIntensity(index);
	vec3 color = GetPointLightColor(index);

	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	vec3 toLight = light_pos - pos;
	float distanceSqr = dot(toLight, toLight);

	float dist = distance(light_pos, pos);
	float attenuation = remap(dist, 0.0, falloff, 0.0, 1.0);
	attenuation = (1.0 - attenuation);
	return ndotl * density * color * attenuation;
}


// [0] Frensel Schlick
vec3 F_Schlick(vec3 f0, float f90, float u)
{
	return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}


// [1] IBL Defuse Irradiance
vec3 F_Schlick_Roughness(vec3 F0, float cos_theta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}


// [0] Diffuse Term
float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness)
{
	float E_bias        = 0.0 * (1.0 - roughness) + 0.5 * roughness;
	float E_factor      = 1.0 * (1.0 - roughness) + (1.0 / 1.51) * roughness;
	float fd90          = E_bias + 2.0 * LdotH * LdotH * roughness;
	vec3  f0            = vec3(1.0);
	float light_scatter = F_Schlick(f0, fd90, NdotL).r;
	float view_scatter  = F_Schlick(f0, fd90, NdotV).r;
	return light_scatter * view_scatter * E_factor;
}


// [0] Specular Microfacet Model
float V_SmithGGXCorrelated(float NdotV, float NdotL, float roughness)
{
	float alphaRoughnessSq = roughness * roughness;

	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
	{
		return 0.5 / GGX;
	}
	return 0.0;
}


// [0] GGX Normal Distribution Function
float D_GGX(float NdotH, float roughness)
{
	float alphaRoughnessSq = roughness * roughness;
	float f                = (NdotH * alphaRoughnessSq - NdotH) * NdotH + 1.0;
	return alphaRoughnessSq / (PI * f * f);
}


#define REFLECTION_CAPTURE_ROUGHEST_MIP 1
#define REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE 1.2
/** 
 * Compute absolute mip for a reflection capture cubemap given a roughness.
 */
float ComputeReflectionMipFromRoughness(float roughness, float cubemap_max_mip)
{
	// Heuristic that maps roughness to mip level
	// This is done in a way such that a certain mip level will always have the same roughness, regardless of how many mips are in the texture
	// Using more mips in the cubemap just allows sharper reflections to be supported
	float level_from_1x1 = REFLECTION_CAPTURE_ROUGHEST_MIP - REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE * log2(max(roughness, 0.001));
	return cubemap_max_mip - 1 - level_from_1x1;
}


vec2 EnvBRDFApproxLazarov(float Roughness, float NoV)
{
	// [ Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II" ]
	// Adaptation to fit our G term.
	const vec4 c0 = { -1, -0.0275, -0.572, 0.022 };
	const vec4 c1 = { 1, 0.0425, 1.04, -0.04 };
	vec4 r = Roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
	return AB;
}


vec3 EnvBRDFApprox( vec3 SpecularColor, float Roughness, float NoV )
{
	vec2 AB = EnvBRDFApproxLazarov(Roughness, NoV);

	// Anything less than 2% is physically impossible and is instead considered to be shadowing
	// Note: this is needed for the 'specular' show flag to work, since it uses a SpecularColor of 0
	float F90 = saturate( 50.0 * SpecularColor.g );

	return SpecularColor * AB.x + F90 * AB.y;
}


float GetSpecularOcclusion(float NoV, float RoughnessSq, float AO)
{
	return saturate( pow( NoV + AO, RoughnessSq ) - 1 + AO );
}


float DielectricSpecularToF0(float Specular)
{
	return F0.x * 2.0f * Specular;
}


vec3 ComputeF0(float Specular, vec3 BaseColor, float Metallic)
{
	// clamp pure black base color to get clear coat
	BaseColor = clamp(BaseColor, F0, vec3(1.0f));
	return lerp(DielectricSpecularToF0(Specular).xxx, BaseColor, Metallic.x);
}


struct FDirectLighting
{
	vec3 Diffuse;
	vec3 Specular;
	vec3 Transmission;
};

FDirectLighting DefaultLitBxDF(vec3 DiffuseColor, vec3 SpecularColor, float Roughness, float LoH, float NoV, float NoL, float NoH)
{
	FDirectLighting Lighting;

	float F90 = saturate(50.0 * F0.r);
	vec3 F = F_Schlick(F0, F90, LoH);

	float Vis = V_SmithGGXCorrelated(NoV, NoL, Roughness);
	float D = D_GGX(NoH, Roughness);
	vec3 Fr = F * D * Vis;
	float Fd = Fr_DisneyDiffuse(NoV, NoL, LoH, Roughness);

	Lighting.Diffuse = DiffuseColor * (vec3(1.0) - F) * Fd;

	Lighting.Specular = Fr;

	// @TODO: Energy Conservation
	//FBxDFEnergyTermsRGB EnergyTerms = ComputeGGXSpecEnergyTermsRGB(GBuffer.Roughness, Context.NoV, GBuffer.SpecularColor);
	//Lighting.Diffuse *= ComputeEnergyPreservation(EnergyTerms);
	//Lighting.Specular *= ComputeEnergyConservation(EnergyTerms);

	Lighting.Transmission = vec3(0.0f);
	return Lighting;
}

FDirectLighting IntegrateBxDF(vec3 DiffuseColor, vec3 SpecularColor, float Roughness, float LoH, float NoV, float NoL, float NoH)
{
	return DefaultLitBxDF(DiffuseColor, SpecularColor, Roughness, LoH, NoV, NoL, NoH);
}


const mat4 BiasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );


vec4 ComputeShadowCoord(vec3 Position)
{
	return BiasMat * view.shadowmapSpace * vec4(Position, 1.0);
}


float ShadowDepthProject(vec4 ShadowCoord, vec2 Offset)
{
	float ShadowFactor = 1.0;
	if ( ShadowCoord.z > -1.0 && ShadowCoord.z < 1.0 ) 
	{
		float Dist = texture( shadowmap, ShadowCoord.st + Offset ).r;
		if ( ShadowCoord.w > 0.0 && Dist < ShadowCoord.z ) 
		{
			ShadowFactor = 0.1;
		}
	}
	return ShadowFactor;
}


// Percentage Closer Filtering (PCF)
float ComputePCF(vec4 sc /*shadow croodinate*/, int r /*filtering range*/)
{
	ivec2 TexDim = textureSize(shadowmap, 0);
	float Scale = 1.5;
	float dx = Scale * 1.0 / float(TexDim.x);
	float dy = Scale * 1.0 / float(TexDim.y);

	float ShadowFactor = 0.0;
	int Count = 0;
	
	for (int x = -r; x <= r; x++)
	{
		for (int y = -r; y <= r; y++)
		{
			ShadowFactor += ShadowDepthProject(sc, vec2(dx*x, dy*y));
			Count++;
		}
	}
	return ShadowFactor / Count;
}


void main()
{
	vec3 VertexColor = fragColor;

	vec3 BaseColor = texture(sampler1, fragTexCoord).rgb;
	float Metallic = saturate(texture(sampler2, fragTexCoord).r);
	float Roughness = saturate(texture(sampler3, fragTexCoord).r);
	vec3 Normal = ComputeNormal(texture(sampler4, fragTexCoord).rgb);
	vec3 AmbientOcclution = texture(sampler5, fragTexCoord).rgb;

	Roughness = max(0.01, Roughness);
	float AO = AmbientOcclution.r;
	vec3 N = Normal;
	vec3 P = fragPosition;
	vec3 V = normalize(view.cameraInfo.xyz - P);
	float NdotV = saturate(dot(N, V));

	float ShadowFactor = 1.0;
	if (SPEC_CONSTANTS == 8)
	{
		vec4 ShadowCoord = ComputeShadowCoord(P);
		ShadowFactor = ShadowDepthProject(ShadowCoord / ShadowCoord.w, vec2(0.0));
	}
	if (SPEC_CONSTANTS == 0 || SPEC_CONSTANTS == 9)
	{
		vec4 ShadowCoord = ComputeShadowCoord(P);
		ShadowFactor = ComputePCF(ShadowCoord / ShadowCoord.w, 2);
	}

	// (1) Direct Lighting : DisneyDiffuse + SpecularGGX
	vec3 DirectLighting = vec3(0.0);
	vec3 DiffuseColor = BaseColor.rgb * (1.0 - Metallic);
	vec3 SpecularColor = vec3(1.0);
	for (uint i = 0u; i < DIRECTIONAL_LIGHTS; ++i)
	{
		vec3 L = GetDirectionalLightDirection(i);
		vec3 H = normalize(V + L);

		float LdotH = saturate(dot(L, H));
		float NdotH = saturate(dot(N, H));
		float NdotL = saturate(dot(N, L));

		FDirectLighting DirectionalLight = IntegrateBxDF(DiffuseColor, SpecularColor, Roughness, LdotH, NdotV, NdotL, NdotH);

		DirectLighting += ApplyDirectionalLight(i, N) * (DirectionalLight.Diffuse + DirectionalLight.Specular) * ShadowFactor;
	}
	for (uint i = 0u; i < POINT_LIGHTS; ++i)
	{
		vec3 L = GetPointLightDirection(i, P);
		vec3 H = normalize(V + L);

		float LdotH = saturate(dot(L, H));
		float NdotH = saturate(dot(N, H));
		float NdotL = saturate(dot(N, L));

		FDirectLighting PointLight = IntegrateBxDF(DiffuseColor, SpecularColor, Roughness, LdotH, NdotV, NdotL, NdotH);

		DirectLighting += ApplyPointLight(i, P, N) * (PointLight.Diffuse + PointLight.Specular);
	}

	// (2) Indirect Lighting : Simple lambert diffuse as indirect lighting
	vec3 IndirectLighting = DiffuseColor / PI * AO * 0.3 * ShadowFactor;

	// (3) Reflection Specular : Image based lighting
	vec3 ReflectionSpec = ComputeF0(0.5, BaseColor, Metallic);
	vec3 ReflectionBRDF = EnvBRDFApprox(ReflectionSpec, Roughness, NdotV);
	float ratio = 1.00 / 1.52;
	vec3 I = V;
	vec3 R = refract(I, normalize(N), ratio);
	float MIPS = ComputeReflectionMipFromRoughness(Roughness, SKY_MAXMIPS);
	vec3 Reflection_L = textureLod(cubemap, R, MIPS).rgb * 10.0;
	float Reflection_V = GetSpecularOcclusion(NdotV, Roughness * Roughness, AO);
	vec3 ReflectionColor = Reflection_L * Reflection_V * ReflectionBRDF;

	vec3 FinalColor = DirectLighting + IndirectLighting + ReflectionColor;

	// Gamma correct
	FinalColor = pow(FinalColor, vec3(0.4545));

	switch (SPEC_CONSTANTS) {
		case 0:
			outColor = vec4(FinalColor * ShadowFactor, 1.0); break;
		case 1:
			outColor = vec4(vec3(BaseColor), 1.0); break;
		case 2:
			outColor = vec4(vec3(Metallic), 1.0); break;
		case 3:
			outColor = vec4(vec3(Roughness), 1.0); break;
		case 4:
			outColor = vec4(vec3(Normal), 1.0); break;
		case 5:
			outColor = vec4(vec3(AmbientOcclution), 1.0); break;
		case 6:
			outColor = vec4(vec3(VertexColor), 1.0); break;
		case 7:
			outColor = vec4(vec3(ReflectionColor), 1.0); break;
		case 8:
		case 9:
			outColor = vec4(vec3(ShadowFactor), 1.0); break;
		default:
			outColor = vec4(FinalColor * ShadowFactor, 1.0); break;
	};
}
