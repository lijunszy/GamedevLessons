#version 450

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
	light directionalLights[4];
	light pointLights[4];
	light spotLights[4];
	ivec4 lightsCount; // [0] for directionalLights, [1] for pointLights, [2] for spotLights
	float zNear;
	float zFar;
} view;
layout(set = 0, binding = 2) uniform samplerCube CubemapSampler;
layout(set = 0, binding = 3) uniform sampler2D ShadowMapSampler;
layout(set = 0, binding = 4) uniform sampler2D SkydomeSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 SkydomeColor = texture(SkydomeSampler, fragTexCoord).rgb;

	vec3 N = -fragNormal; // skydome's normal point to the centre of sphere
	vec3 P = fragPosition;
	vec3 V = normalize(view.cameraInfo.xyz - P);
	vec3 I = V;
	float ratio = 1.00 / 1.52;
	vec3 R = refract(I, normalize(N), ratio);
	vec3 CubeMapColor = textureLod(CubemapSampler, R, 0).rgb * 10.0;

	// Gamma correct
	vec3 FinalColor = pow(SkydomeColor, vec3(0.4545));

	outColor = vec4(FinalColor, 1.0);
}