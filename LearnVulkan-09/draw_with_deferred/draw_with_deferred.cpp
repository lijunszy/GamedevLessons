// Copyright LearnVulkan-09: Draw with Deferred, @xukai. All Rights Reserved.
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // 深度缓存区，OpenGL默认是（-1.0， 1.0）Vulakn为（0.0， 1.0）
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <regex>
#include <limits>
#include <optional>
#include <vector>
#include <set>
#include <array>
#include <chrono>
#include <unordered_map>


#define VIEWPORT_WIDTH 1080;
#define VIEWPORT_HEIGHT 720;
#define PBR_SAMPLER_NUMBER 7 // BC + M + R + N + AO + Emissive + Mask
#define POINT_LIGHTS_NUM 16
#define SHADOWMAP_DIM 1024
#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1
#define INSTANCE_COUNT 8192
#define SCENE_SHOW_STAGE false
#define SCENE_SHOW_SKYDOME true
#define ENABLE_INDIRECT_DRAW false
#define ENABLE_DEFEERED_RENDERING true

/** 同时渲染多帧的最大帧数*/
const int MAX_FRAMES_IN_FLIGHT = 2;

/** 每个Instance 的数据块*/
struct FInstanceData {
	glm::vec3 InstancePosition;
	glm::vec3 InstanceRotation;
	glm::float32 InstancePScale;
	glm::uint8 InstanceTexIndex;
};


enum EVertexType
{
	VertexIndexed = 0,
	Instanced,
	ScreenRect
};


enum EGraphicPipelineType
{
	Forward = 0,
	DeferredScene,
	DeferredLighting,
};


struct FLight
{
	glm::vec4 Position;
	glm::vec4 Color; // rgb for Color, a for intensity
	glm::vec4 Direction;
	glm::vec4 LightInfo;

	FLight& FLight::operator=(const FLight& rhs)
	{
		Position = rhs.Position;
		Color = rhs.Color;
		Direction = rhs.Direction;
		LightInfo = rhs.LightInfo;
		return *this;
	}
};

/** 物体的MVP矩阵信息*/
struct FUniformBufferBase {
	glm::mat4 Model;
	glm::mat4 View;
	glm::mat4 Proj;
};

/** 顶点数据存储结构*/
struct FVertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Color;
	glm::vec2 TexCoord;

	// 顶点描述
	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = VERTEX_BUFFER_BIND_ID;
		bindingDescription.stride = sizeof(FVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

		attributeDescriptions[0].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(FVertex, Position);

		attributeDescriptions[1].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(FVertex, Normal);

		attributeDescriptions[2].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(FVertex, Color);

		attributeDescriptions[3].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(FVertex, TexCoord);

		return attributeDescriptions;
	}

	// 顶点描述，带Instance
	static std::array<VkVertexInputBindingDescription, 2> GetBindingInstancedDescriptions() {
		VkVertexInputBindingDescription bindingDescription0{};
		bindingDescription0.binding = VERTEX_BUFFER_BIND_ID;
		bindingDescription0.stride = sizeof(FVertex);
		bindingDescription0.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputBindingDescription bindingDescription1{};
		bindingDescription1.binding = INSTANCE_BUFFER_BIND_ID;
		bindingDescription1.stride = sizeof(FInstanceData);
		bindingDescription1.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return { bindingDescription0, bindingDescription1 };
	}

	static std::array<VkVertexInputAttributeDescription, 8> GetAttributeInstancedDescriptions() {
		std::array<VkVertexInputAttributeDescription, 8> attributeDescriptions{};

		attributeDescriptions[0].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(FVertex, Position);

		attributeDescriptions[1].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(FVertex, Normal);

		attributeDescriptions[2].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(FVertex, Color);

		attributeDescriptions[3].binding = VERTEX_BUFFER_BIND_ID;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(FVertex, TexCoord);

		attributeDescriptions[4].binding = INSTANCE_BUFFER_BIND_ID;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(FInstanceData, InstancePosition);

		attributeDescriptions[5].binding = INSTANCE_BUFFER_BIND_ID;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(FInstanceData, InstanceRotation);

		attributeDescriptions[6].binding = INSTANCE_BUFFER_BIND_ID;
		attributeDescriptions[6].location = 6;
		attributeDescriptions[6].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[6].offset = offsetof(FInstanceData, InstancePScale);

		attributeDescriptions[7].binding = INSTANCE_BUFFER_BIND_ID;
		attributeDescriptions[7].location = 7;
		attributeDescriptions[7].format = VK_FORMAT_R8_UINT;
		attributeDescriptions[7].offset = offsetof(FInstanceData, InstanceTexIndex);

		return attributeDescriptions;
	}

	bool operator==(const FVertex& other) const {
		return Position == other.Position && Normal == other.Normal && Color == other.Color && TexCoord == other.TexCoord;
	}
};

namespace std {
	template<> struct hash<FVertex> {
		size_t operator()(FVertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}


const std::vector<const char*> ValidationLayers = { "VK_LAYER_KHRONOS_validation" }; // VK_LAYER_KHRONOS_validation这个是固定的，不能重命名
const std::vector<const char*> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


#ifdef NDEBUG
const bool bEnableValidationLayers = false;  // Build Configuration: Release
#else
const bool bEnableValidationLayers = true;   // Build Configuration: Debug
#endif


static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


/** 所有的硬件信息*/
struct FQueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;

	bool IsComplete()
	{
		return GraphicsFamily.has_value() && PresentFamily.has_value();
	}
};


/** 支持的硬件细节信息*/
struct FSwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};


class FVulkanRendererApp
{
	struct FGlobalInput {
		glm::vec3 CameraPos;
		glm::vec3 CameraLookat;
		float CameraSpeed;
		float CameraFOV;
		float zNear;
		float zFar;

		float CameraArm;
		float CameraYaw;
		float CameraPitch;

		bool bFocusCamera;
		bool bUpdateCamera;

		float CurrentTime;
		float DeltaTime;
		bool bFirstInit;
		double LastMouseX, LastMouseY;
		bool bPlayStageRoll;
		float RollStage;
		bool bPlayLightRoll;
		float RollLight;

		void ResetToFocus()
		{
			CameraPos = glm::vec3(2.0, 2.0, 2.0);
			CameraLookat = glm::vec3(0.0, 0.0, 0.0);
			CameraSpeed = 2.5;
			CameraFOV = 45.0;
			zNear = 0.1f;
			zFar = 45.0f;

			glm::mat4 transform = glm::lookAt(CameraPos, CameraLookat, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::quat rotation(transform);
			glm::vec3 Direction = glm::normalize(CameraLookat - CameraPos);
			CameraYaw = 30.0; //glm::degrees(glm::atan(Direction.x, Direction.y));
			CameraPitch = 60.0; //glm::degrees(glm::asin(Direction.z));
			CameraArm = 10.0;
			bUpdateCamera = false;
			bFocusCamera = true;

			CameraPos.x = cos(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
			CameraPos.y = sin(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
			CameraPos.z = sin(glm::radians(CameraPitch)) * CameraArm;
		}
		void ResetToTravel()
		{
			CameraPos = glm::vec3(2.0, 0.0, 0.0);
			CameraLookat = glm::vec3(0.0, 0.0, 0.0);
			CameraSpeed = 2.5;
			CameraFOV = 45.0;
			zNear = 0.1f;
			zFar = 45.0f;

			CameraArm = (float)(CameraLookat - CameraPos).length();
			glm::mat4 transform = glm::lookAt(CameraPos, CameraLookat, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::quat rotation(transform);
			CameraYaw = -90.0f;;
			CameraPitch = 0.0;
			bUpdateCamera = false;
			bFocusCamera = false;
		}
		void ResetAnimation()
		{
			bPlayStageRoll = false;
			RollStage = 0.0;
			bPlayLightRoll = false;
			RollLight = 0.0;
		}
	} GlobalInput;

	/** 全局常量*/
	struct FGlobalConstants {
		float Time;
		float MetallicFactor;
		float RoughnessFactor;
		uint32_t SpecConstants;
		uint32_t SpecConstantsCount;                // 特殊常数，用于优化着色器变体
		void ResetConstants()
		{
			Time = 0.0f;
			MetallicFactor = 0.0f;
			RoughnessFactor = 1.0;
			SpecConstants = 0;
			SpecConstantsCount = 10;
		}
	} GlobalConstants;

	/** 场景灯光信息*/
	struct FUniformBufferView {
		glm::mat4 ShadowmapSpace;
		glm::mat4 LocalToWorld;
		glm::vec4 CameraInfo;
		FLight DirectionalLights[16];
		FLight PointLights[512];
		FLight SpotLights[16];
		// LightsCount: [0] for number of DirectionalLights, [1] for number of PointLights, [2] for number of SpotLights, [3] for number of cube map max miplevels.
		glm::ivec4 LightsCount;
		glm::float32 zNear;
		glm::float32 zFar;

		FUniformBufferView& FUniformBufferView::operator=(const FUniformBufferView& rhs)
		{
			ShadowmapSpace = rhs.ShadowmapSpace;
			LocalToWorld = rhs.LocalToWorld;
			CameraInfo = rhs.CameraInfo;
			DirectionalLights[16] = rhs.DirectionalLights[16];
			PointLights[512] = rhs.PointLights[512];
			SpotLights[16] = rhs.SpotLights[16];
			LightsCount = rhs.LightsCount;
			zNear = rhs.zNear;
			zFar = rhs.zFar;
			return *this; 
		}
	} View;

	struct FMesh {
		std::vector<FVertex> Vertices;                       // 顶点
		std::vector<uint32_t> Indices;                       // 点序
		VkBuffer VertexBuffer;                               // 顶点缓存
		VkDeviceMemory VertexBufferMemory;                   // 顶点缓存内存
		VkBuffer IndexBuffer;                                // 点序缓存
		VkDeviceMemory IndexBufferMemory;                    // 点序缓存内存

		// only init with instanced mesh
		VkBuffer InstancedBuffer;                            // Instanced buffer
		VkDeviceMemory InstancedBufferMemory;                // Instanced buffer memory
	};

	typedef FMesh FInstancedMesh;

	struct FCluster {

	};

	struct FActor {
		std::vector<FCluster> Clusters;
	};

	struct FIndirectMesh : public FMesh
	{
		std::vector<FActor> Actors;
	};

	struct FIndirectInstancedMesh : public FInstancedMesh
	{
		std::vector<FActor> Actors;
	};

	struct FMaterial {
		std::vector<VkImage> TextureImages;                  // 贴图
		std::vector<VkDeviceMemory> TextureImageMemorys;     // 贴图内存
		std::vector<VkImageView> TextureImageViews;          // 贴图视口
		std::vector<VkSampler> TextureSamplers;              // 贴图采样器

		VkDescriptorPool DescriptorPool;                     // 描述符池
		std::vector<VkDescriptorSet> DescriptorSets;         // 描述符集合
	};

	struct FRenderBase
	{
		FMaterial MateData;
	};

	/** 构建 RenderObject 需要的 Vulkan 资源*/
	struct FRenderObject : public FRenderBase
	{
		FMesh MeshData;
	};

	struct FRenderInstancedObject : public FRenderBase
	{
		FInstancedMesh MeshData;
		uint32_t InstanceCount;
	};

	struct FRenderIndirectObjectBase : public FRenderBase
	{
		VkBuffer IndirectCommandsBuffer;							// 包含 the indirect drawing commands
		VkDeviceMemory IndirectCommandsBufferMemory;
		std::vector<VkDrawIndexedIndirectCommand> IndirectCommands; // 存储 indirect draw commands，包含 index offsets 和 Instance count per object
	};

	struct FRenderIndirectObject : public FRenderIndirectObjectBase
	{
		FIndirectMesh MeshData;
	};

	struct FRenderIndirectInstancedObject : public FRenderIndirectObjectBase
	{
		FIndirectInstancedMesh MeshData;
		uint32_t InstanceCount;
	};

	/** 延迟管线 GBuffer*/
	struct FGeometryBuffer {
		// Depth Stencil RGBAFloat
		VkFormat DepthStencilFormat;
		VkImage DepthStencilImage;
		VkDeviceMemory DepthStencilMemory;
		VkImageView DepthStencilImageView;
		VkSampler DepthStencilSampler;
		// SceneColorDeferred RGBAHalf
		VkFormat SceneColorFormat;
		VkImage SceneColorImage;
		VkDeviceMemory SceneColorMemory;
		VkImageView SceneColorImageView;
		VkSampler SceneColorSampler;
		// Normal+CastShadow R10G10B10A2
		VkFormat GBufferAFormat;
		VkImage GBufferAImage;
		VkDeviceMemory GBufferAMemory;
		VkImageView GBufferAImageView;
		VkSampler GBufferASampler;
		// M+S+R+(ShadingModelID+SelectiveOutputMask) RGBA8888
		VkFormat GBufferBFormat;
		VkImage GBufferBImage;
		VkDeviceMemory GBufferBMemory;
		VkImageView GBufferBImageView;
		VkSampler GBufferBSampler;
		// BaseColor+AO
		VkFormat GBufferCFormat;
		VkImage GBufferCImage;
		VkDeviceMemory GBufferCMemory;
		VkImageView GBufferCImageView;
		VkSampler GBufferCSampler;
		// Position+ID
		VkFormat GBufferDFormat;
		VkImage GBufferDImage;
		VkDeviceMemory GBufferDMemory;
		VkImageView GBufferDImageView;
		VkSampler GBufferDSampler;
		// MotionVector+Velocity(Currently not implemented)
		//VkImage GBufferVelocityImage;
		//VkDeviceMemory GBufferVelocityMemory;
		//VkImageView GBufferVelocityImageView;
		
		VkSampler Sampler;
	} GBuffer;

	/** 构建 ShadowmapPass 需要的 Vulkan 资源*/
	struct FShadowmapPass {
		std::vector<FRenderObject*> RenderObjects;
		std::vector<FRenderInstancedObject*> RenderInstancedObjects;
		std::vector<FRenderIndirectObject*> RenderIndirectObject;
		std::vector<FRenderIndirectInstancedObject*> RenderIndirectInstancedObject;
		float zNear, zFar;
		int32_t Width, Height;
		VkFormat Format;
		VkFramebuffer FrameBuffer;
		VkRenderPass RenderPass;
		VkImage Image;
		VkDeviceMemory Memory;
		VkImageView ImageView;
		VkSampler Sampler;
		VkDescriptorSetLayout DescriptorSetLayout;
		VkDescriptorPool DescriptorPool;
		std::vector<VkDescriptorSet> DescriptorSets;
		VkPipelineLayout PipelineLayout;
		VkPipeline Pipeline;
		VkPipeline PipelineInstanced;
		std::vector<VkBuffer> UniformBuffers;
		std::vector<VkDeviceMemory> UniformBuffersMemory;
	} ShadowmapPass;

	/** 构建 BackgroundPass 需要的 Vulkan 资源*/
	struct FBackgroundPass {
		VkImage Image;
		VkDeviceMemory Memory;
		VkImageView ImageView;
		VkSampler Sampler;
		VkRenderPass RenderPass;
		VkDescriptorSetLayout DescriptorSetLayout;
		VkDescriptorPool DescriptorPool;
		std::vector<VkDescriptorSet> DescriptorSets;
		VkPipelineLayout PipelineLayout;
		std::vector<VkPipeline> Pipelines;
	} BackgroundPass;


	/** 构建 SkydomePass 需要的 Vulkan 资源*/
	struct FSkydomePass {
		FMesh SkydomeMesh;
		VkImage Image;
		VkDeviceMemory Memory;
		VkImageView ImageView;
		VkSampler Sampler;
		VkRenderPass RenderPass;
		VkDescriptorSetLayout DescriptorSetLayout;
		VkDescriptorPool DescriptorPool;
		std::vector<VkDescriptorSet> DescriptorSets;
		VkPipelineLayout PipelineLayout;
		std::vector<VkPipeline> Pipelines;
	} SkydomePass;

	/** 构建 BaseScenePass 需要的 Vulkan 资源*/
	struct FBaseScenePass {
		std::vector<FRenderObject> RenderObjects;					// 待渲染的物体
		std::vector<FRenderInstancedObject> RenderInstancedObjects;	// 待渲染的物体
		VkDescriptorSetLayout DescriptorSetLayout;					// 描述符集合布局
		VkPipelineLayout PipelineLayout;							// 渲染管线布局
		std::vector<VkPipeline> Pipelines;							// 渲染管线
		std::vector<VkPipeline> PipelinesInstanced;					// 渲染管线
	} BaseScenePass;

	struct FBaseSceneIndirectPass {
		std::vector<FRenderIndirectObject> RenderIndirectObject;					// 待渲染的物体
		std::vector<FRenderIndirectInstancedObject> RenderIndirectInstancedObject;	// 待渲染的物体
		VkDescriptorSetLayout DescriptorSetLayout;									// 描述符集合布局
		VkPipelineLayout PipelineLayout;											// 渲染管线布局
		std::vector<VkPipeline> Pipelines;											// 渲染管线
		std::vector<VkPipeline> PipelinesInstanced;									// 渲染管线
	} BaseSceneIndirectPass;

	struct FBaseSceneDeferredRenderingPass {
		std::vector<FRenderObject> RenderObjects;					// 待渲染的物体
		std::vector<FRenderInstancedObject> RenderInstancedObjects;	// 待渲染的物体
		VkDescriptorSetLayout SceneDescriptorSetLayout;				// 描述符集合布局
		VkPipelineLayout ScenePipelineLayout;						// 渲染管线布局
		std::vector<VkPipeline> ScenePipelines;						// 渲染管线
		std::vector<VkPipeline> ScenePipelinesInstanced;			// 渲染管线
		VkFramebuffer SceneFrameBuffer;
		VkRenderPass SceneRenderPass;
		VkDescriptorSetLayout LightingDescriptorSetLayout;
		VkDescriptorPool LightingDescriptorPool;
		std::vector<VkDescriptorSet> LightingDescriptorSets;
		VkPipelineLayout LightingPipelineLayout;
		std::vector<VkPipeline> LightingPipelines;
	} BaseSceneDeferredPass;

	GLFWwindow* Window;										// Window 渲染桌面

	VkInstance Instance;									// 链接程序的Vulkan实例
	VkDebugUtilsMessengerEXT DebugMessenger;
	VkSurfaceKHR Surface;									// 链接桌面和Vulkan的实例

	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;		// 物理显卡硬件
	VkDevice Device;										// 逻辑硬件，对接物理硬件

	VkQueue GraphicsQueue;									// 显卡的队列
	VkQueue PresentQueue;									// 显示器的队列

	VkSwapchainKHR SwapChain;								// 缓存渲染图像队列，同步到显示器
	std::vector<VkImage> SwapChainImages;					// 渲染图像队列
	VkFormat SwapChainImageFormat;							// 渲染图像格式
	VkExtent2D SwapChainExtent;								// 渲染图像范围
	std::vector<VkImageView> SwapChainImageViews;			// 渲染图像队列对应的视图队列
	std::vector<VkFramebuffer> SwapChainFramebuffers;		// 渲染图像队列对应的帧缓存队列

	VkRenderPass MainRenderPass;							// 渲染层，保存Framebuffer和采样信息

	VkImage DepthImage;										// 深度纹理资源
	VkDeviceMemory DepthImageMemory;						// 深度纹理内存
	VkImageView DepthImageView;								// 深度纹理图像视口

	uint32_t CubemapMaxMips;								// 环境反射纹理最大Mips数
	VkImage CubemapImage;									// 环境反射纹理资源
	VkDeviceMemory CubemapImageMemory;						// 环境反射纹理内存
	VkImageView CubemapImageView;							// 环境反射纹理图像视口
	VkSampler CubemapSampler;								// 环境反射纹理采样器

	std::vector<VkBuffer> BaseUniformBuffers;				// 统一缓存区
	std::vector<VkDeviceMemory> BaseUniformBuffersMemory;	// 统一缓存区内存地址

	std::vector<VkBuffer> ViewUniformBuffers;				// 统一缓存区
	std::vector<VkDeviceMemory> ViewUniformBuffersMemory;	// 统一缓存区内存地址

	VkCommandPool CommandPool;								// 指令池
	VkCommandBuffer CommandBuffer;							// 指令缓存

	VkSemaphore ImageAvailableSemaphore;					// 图像是否完成的信号
	VkSemaphore RenderFinishedSemaphore;					// 渲染是否结束的信号
	VkFence InFlightFence;									// 围栏，下一帧渲染前等待上一帧全部渲染完成

	std::vector<VkCommandBuffer> CommandBuffers;			// 指令缓存
	std::vector<VkSemaphore> ImageAvailableSemaphores;		// 图像是否完成的信号
	std::vector<VkSemaphore> RenderFinishedSemaphores;		// 渲染是否结束的信号
	std::vector<VkFence> InFlightFences;					// 围栏，下一帧渲染前等待上一帧全部渲染完成
	uint32_t CurrentFrame = 0;								// 当前渲染帧序号

	bool bFramebufferResized = false;
public:
	/** 主函数调用接口*/
	void MainTask()
	{
		InitWindow();		// 使用传统的GLFW，初始化窗口
		InitVulkan();		// 初始化Vulkan，创建资源
		MainTick();			// 每帧循环调用，执行渲染指令
		ClearWindow();		// 渲染窗口关闭时，删除创建的资源
	}

public:
	/** 初始化GUI渲染窗口*/
	void InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // 打开窗口的Resize功能

		uint32_t viewportWidth = VIEWPORT_WIDTH;
		uint32_t viewportHeight = VIEWPORT_HEIGHT;
		Window = glfwCreateWindow(viewportWidth, viewportHeight, "LearnVulkan-09: Draw with Deferred", nullptr /* glfwGetPrimaryMonitor() 全屏模式*/, nullptr);
		glfwSetWindowUserPointer(Window, this);
		glfwSetFramebufferSizeCallback(Window, FramebufferResizeCallback);
		GLFWimage iconImages[2];
		iconImages[0].pixels = stbi_load("Resources/Appicons/vulkan_renderer.png", &iconImages[0].width, &iconImages[0].height, 0, STBI_rgb_alpha);
		iconImages[1].pixels = stbi_load("Resources/Appicons/vulkan_renderer_small.png", &iconImages[1].width, &iconImages[1].height, 0, STBI_rgb_alpha);
		glfwSetWindowIcon(Window, 2, iconImages);
		stbi_image_free(iconImages[0].pixels);
		stbi_image_free(iconImages[1].pixels);

		GlobalInput.ResetToFocus();
		GlobalInput.ResetAnimation();
		GlobalConstants.ResetConstants();

		glfwSetKeyCallback(Window, KeyboardCallback);
		glfwSetMouseButtonCallback(Window, MouseButtonCallback);
		glfwSetCursorPosCallback(Window, MousePositionCallback);
		glfwSetScrollCallback(Window, MouseScrollCallback);
	}

	/** 初始化Vulkan的渲染管线*/
	void InitVulkan()
	{
		CreateInstance();			// 连接此程序和Vulkan，一般由显卡驱动实现
		CreateDebugMessenger();		// 创建调试打印信息
		CreateWindowsSurface();		// 连接此程序的窗口和Vulkan，渲染Vulkan输出
		SelectPhysicalDevice();		// 找到此电脑的物理显卡硬件
		CreateLogicalDevice();		// 创建逻辑硬件，对应物理硬件
		CreateSwapChain();			// 创建交换链，用于渲染数据和图像显示的中间交换
		CreateSwapChainImageViews();// 创建图像显示，包含在SwapChain中
		CreateRenderPass();			// 创建渲染通道
		CreateFramebuffers();		// 创建帧缓存，包含在SwapChain中
		CreateCommandPool();		// 创建指令池，存储所有的渲染指令
		CreateUniformBuffers();		// 创建UnifromBuffer统一缓存区
		CreateShadowmapPass();		// 创建阴影贴图渲染通道
		CreateSkydomePass();		// 创建天空球和反射球通道
		CreateBackgroundPass();		// 创建背景渲染通道
		CreateBaseScenePass();		// 创建基础物体渲染通道
		CreateBaseSceneIndirectPass();
#if ENABLE_DEFEERED_RENDERING
		CreateBaseSceneDeferredPass();
#endif
		CreateCommandBuffer();		// 创建指令缓存，指令发送前变成指令缓存
		CreateSyncObjects();		// 创建同步围栏，确保下一帧渲染前，上一帧全部渲染完成
	}

	/** 主循环，执行每帧渲染*/
	void MainTick()
	{
		while (!glfwWindowShouldClose(Window))
		{
			glfwPollEvents();
			UpdateInputs();
			DrawFrame(); // 绘制一帧
		}

		vkDeviceWaitIdle(Device);
	}

	/** 清除Vulkan的渲染管线*/
	void ClearWindow()
	{
		CleanupSwapChain(); // 清理FrameBuffer相关的资源
		DestroyVulkan(); // 删除Vulkan对象

		glfwDestroyWindow(Window);
		glfwTerminate();
	}

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<FVulkanRendererApp*>(glfwGetWindowUserPointer(window));
		app->bFramebufferResized = true;
	}

	static void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		FVulkanRendererApp* app = reinterpret_cast<FVulkanRendererApp*>(glfwGetWindowUserPointer(window));
		FGlobalInput* input = &app->GlobalInput;
		FGlobalConstants* constants = &app->GlobalConstants;

		if (action == GLFW_PRESS && key == GLFW_KEY_F)
		{
			input->ResetToFocus();
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_G)
		{
			input->ResetToTravel();
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_R)
		{
			input->ResetAnimation();
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_M)
		{
			input->bPlayStageRoll = !input->bPlayStageRoll;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_L)
		{
			input->bPlayLightRoll = !input->bPlayLightRoll;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_0)
		{
			constants->SpecConstants = 0;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_1)
		{
			constants->SpecConstants = 1;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_2)
		{
			constants->SpecConstants = 2;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_3)
		{
			constants->SpecConstants = 3;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_4)
		{
			constants->SpecConstants = 4;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_5)
		{
			constants->SpecConstants = 5;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_6)
		{
			constants->SpecConstants = 6;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_7)
		{
			constants->SpecConstants = 7;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_8)
		{
			constants->SpecConstants = 8;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_9)
		{
			constants->SpecConstants = 9;
		}
	}

	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		FVulkanRendererApp* app = reinterpret_cast<FVulkanRendererApp*>(glfwGetWindowUserPointer(window));
		FGlobalInput* input = &app->GlobalInput;

		if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			input->bUpdateCamera = true;
			input->bFirstInit = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			input->bUpdateCamera = false;
			input->bFirstInit = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	static void MousePositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		FVulkanRendererApp* app = reinterpret_cast<FVulkanRendererApp*>(glfwGetWindowUserPointer(window));
		FGlobalInput* input = &app->GlobalInput;

		if (!input->bUpdateCamera)
		{
			return;
		}

		if (input->bFirstInit)
		{
			input->LastMouseX = xpos;
			input->LastMouseY = ypos;
			input->bFirstInit = false;
		}

		float CameraYaw = input->CameraYaw;
		float CameraPitch = input->CameraPitch;

		float xoffset = (float)(xpos - input->LastMouseX);
		float yoffset = (float)(ypos - input->LastMouseY);
		input->LastMouseX = xpos;
		input->LastMouseY = ypos;

		float sensitivityX = 1.0;
		float sensitivityY = 0.5;
		xoffset *= sensitivityX;
		yoffset *= sensitivityY;

		CameraYaw -= xoffset;
		CameraPitch += yoffset;
		if (CameraPitch > 89.0)
			CameraPitch = 89.0;
		if (CameraPitch < -89.0)
			CameraPitch = -89.0;

		if (input->bFocusCamera)
		{
			glm::vec3 CameraPos = input->CameraPos;
			float CameraArm = input->CameraArm;
			CameraPos.x = cos(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
			CameraPos.y = sin(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
			CameraPos.z = sin(glm::radians(CameraPitch)) * CameraArm;
			input->CameraPos = CameraPos;
			input->CameraYaw = CameraYaw;
			input->CameraPitch = CameraPitch;
		}
		else
		{
			// @TODO: 鼠标右键控制相机的旋转
		}
	}

	static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		FVulkanRendererApp* app = reinterpret_cast<FVulkanRendererApp*>(glfwGetWindowUserPointer(window));
		FGlobalInput* input = &app->GlobalInput;

		if (input->bFocusCamera)
		{
			glm::vec3 CameraPos = input->CameraPos;
			glm::vec3 CameraLookat = input->CameraLookat;
			glm::vec3 lookatToPos = CameraLookat - CameraPos;
			glm::vec3 Direction = glm::normalize(lookatToPos);
			float cameraDeltaMove = (float)yoffset * 0.5f;
			float camerArm = input->CameraArm;
			camerArm += cameraDeltaMove;
			camerArm = glm::max(camerArm, 1.0f);
			CameraPos = CameraLookat - camerArm * Direction;
			input->CameraPos = CameraPos;
			input->CameraArm = camerArm;
		}
	}

	void UpdateInputs()
	{
		float DeltaTime = (float)GlobalInput.DeltaTime;    // Time between current frame and last frame
		float lastFrame = (float)GlobalInput.CurrentTime;  // Time of last frame

		float CurrentFrame = (float)glfwGetTime();
		DeltaTime = CurrentFrame - lastFrame;

		bool bFocusCamera = GlobalInput.bFocusCamera;
		glm::vec3 CameraPos = GlobalInput.CameraPos;
		glm::vec3 CameraLookat = GlobalInput.CameraLookat;
		glm::vec3 cameraForward = glm::vec3(-1.0, 0.0, 0.0);
		glm::vec3 cameraUp = glm::vec3(0.0, 0.0, 1.0);
		float CameraSpeed = GlobalInput.CameraSpeed;
		float CameraYaw = GlobalInput.CameraYaw;
		float CameraPitch = GlobalInput.CameraPitch;
		glm::vec3 cameraDirection = glm::normalize(CameraLookat - CameraPos);
		const float cameraDeltaMove = 2.5f * DeltaTime; // adjust accordingly

		if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
		{
			if (bFocusCamera)
			{
				float CameraArm = GlobalInput.CameraArm;
				CameraArm -= cameraDeltaMove;
				CameraArm = glm::max(CameraArm, 1.0f);
				CameraPos = CameraLookat - CameraArm * cameraDirection;
				GlobalInput.CameraPos = CameraPos;
				GlobalInput.CameraArm = CameraArm;
			}
			else
			{
				GlobalInput.CameraPos += cameraDeltaMove * cameraDirection;
			}
			GlobalInput.CameraLookat = bFocusCamera ? CameraLookat : (CameraPos + cameraDirection);
		}
		if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
		{
			if (bFocusCamera)
			{
				float CameraArm = GlobalInput.CameraArm;
				CameraArm += cameraDeltaMove;
				CameraArm = glm::max(CameraArm, 1.0f);
				CameraPos = CameraLookat - CameraArm * cameraDirection;
				GlobalInput.CameraPos = CameraPos;
				GlobalInput.CameraArm = CameraArm;
			}
			else
			{
				GlobalInput.CameraPos -= cameraDeltaMove * cameraDirection;
			}
			GlobalInput.CameraLookat = bFocusCamera ? CameraLookat : (CameraPos + cameraDirection);
		}
		if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
		{
			if (bFocusCamera)
			{
				CameraYaw += cameraDeltaMove * 45.0f;
				float CameraArm = GlobalInput.CameraArm;
				CameraPos.x = cos(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
				CameraPos.y = sin(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
				GlobalInput.CameraYaw = CameraYaw;
			}
			else
			{
				CameraPos -= glm::normalize(glm::cross(cameraForward, cameraUp)) * cameraDeltaMove;
				CameraLookat = CameraPos + cameraForward;
			}
			GlobalInput.CameraPos = CameraPos;
			GlobalInput.CameraLookat = CameraLookat;
		}
		if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
		{
			if (bFocusCamera)
			{
				CameraYaw -= cameraDeltaMove * 45.0f;
				float CameraArm = GlobalInput.CameraArm;
				CameraPos.x = cos(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
				CameraPos.y = sin(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch)) * CameraArm;
				GlobalInput.CameraYaw = CameraYaw;
			}
			else
			{
				CameraPos += glm::normalize(glm::cross(cameraForward, cameraUp)) * cameraDeltaMove;
				CameraLookat = CameraPos + cameraForward;
			}
			GlobalInput.CameraPos = CameraPos;
			GlobalInput.CameraLookat = CameraLookat;
		}
		if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			if (!bFocusCamera)
			{
				CameraPos.z -= cameraDeltaMove;
				CameraLookat = CameraPos + cameraForward;
				GlobalInput.CameraPos = CameraPos;
				GlobalInput.CameraLookat = CameraLookat;
			}
		}
		if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS)
		{
			if (!bFocusCamera)
			{
				CameraPos.z += cameraDeltaMove;
				CameraLookat = CameraPos + cameraForward;
				GlobalInput.CameraPos = CameraPos;
				GlobalInput.CameraLookat = CameraLookat;
			}
		}
		GlobalInput.CurrentTime = CurrentFrame;
		GlobalInput.DeltaTime = DeltaTime;
	}

	/** 在创建好一切必要资源后，执行绘制操作*/
	void DrawFrame()
	{
		// 等待上一帧绘制完成
		vkWaitForFences(Device, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(Device, SwapChain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		// 当窗口过期时（窗口尺寸改变或者窗口最小化后又重新显示），需要重新创建SwapChain并且停止这一帧的绘制
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		// 更新统一缓存区（UBO）
		UpdateUniformBuffer(CurrentFrame);
		vkResetFences(Device, 1, &InFlightFences[CurrentFrame]);

		// 清除渲染指令缓存
		vkResetCommandBuffer(CommandBuffers[CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		// 记录新的所有的渲染指令缓存
		RecordCommandBuffer(CommandBuffers[CurrentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { ImageAvailableSemaphores[CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &CommandBuffers[CurrentFrame];

		VkSemaphore signalSemaphores[] = { RenderFinishedSemaphores[CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// 提交渲染指令
		if (vkQueueSubmit(GraphicsQueue, 1, &submitInfo, InFlightFences[CurrentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(PresentQueue, &presentInfo);

		CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

protected:
	/** 创建程序和Vulkan之间的连接，涉及程序和显卡驱动之间特殊细节*/
	void CreateInstance()
	{
		if (bEnableValidationLayers && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Render the Scene";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Vulkan Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
#ifdef __APPLE__
		createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

		// 获取需要的glfw拓展名
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (bEnableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
#ifdef __APPLE__
        // Fix issue on Mac(m2) "vkCreateInstance: Found no drivers!"
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        // Issue on Mac(m2), it's not a error, but a warning, ignore it by this time~
        // vkCreateDevice():  VK_KHR_portability_subset must be enabled because physical device VkPhysicalDevice 0x600003764f40[] supports it. The Vulkan spec states: If the VK_KHR_portability_subset extension is included in pProperties of vkEnumerateDeviceExtensionProperties, ppEnabledExtensionNames must include "VK_KHR_portability_subset"
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (bEnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			createInfo.ppEnabledLayerNames = ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &Instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create Instance!");
		}
	}

	/** 合法性监测层 Validation Layers
	 *	- 检查参数规范，检测是否使用
	 *	- 最终对象创建和销毁，找到资源泄漏
	 *	- 通过追踪线程原始调用，检查线程安全性
	 *	- 打印输出每次调用
	 *	- 为优化和重现追踪Vulkan调用
	*/
	void CreateDebugMessenger()
	{
		if (!bEnableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	/** WSI (Window System Integration) 链接Vulkan和Window系统，渲染Vulkan到桌面*/
	void CreateWindowsSurface()
	{
		if (glfwCreateWindowSurface(Instance, Window, nullptr, &Surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create Window Surface!");
		}
	}

	/** 选择支持Vulkan的显卡硬件*/
	void SelectPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

		for (const auto& Device : devices)
		{
			if (IsDeviceSuitable(Device))
			{
				PhysicalDevice = Device;
				break;
			}
		}

		if (PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	/** 创建逻辑硬件对接物理硬件，相同物理硬件可以对应多个逻辑硬件*/
	void CreateLogicalDevice()
	{
		FQueueFamilyIndices queue_family_indices = FindQueueFamilies(PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { queue_family_indices.GraphicsFamily.value(), queue_family_indices.PresentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.multiDrawIndirect = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = DeviceExtensions.data();

		if (bEnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			createInfo.ppEnabledLayerNames = ValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create logical Device!");
		}

		vkGetDeviceQueue(Device, queue_family_indices.GraphicsFamily.value(), 0, &GraphicsQueue);
		vkGetDeviceQueue(Device, queue_family_indices.PresentFamily.value(), 0, &PresentQueue);
	}

	/** 交换链 Swap Chain
	 * Vulkan一种基础结构，持有帧缓存FrameBuffer
	 * SwapChain持有显示到窗口的图像队列
	 * 通常Vulkan获取图像，渲染到图像上，然后将图像推入SwapChain的图像队列
	 * SwapChain显示图像，通常和屏幕刷新率保持同步
	*/
	void CreateSwapChain()
	{
		FSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

		uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.Capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = Surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		FQueueFamilyIndices queue_family_indices = FindQueueFamilies(PhysicalDevice);
		uint32_t queueFamilyIndices[] = { queue_family_indices.GraphicsFamily.value(), queue_family_indices.PresentFamily.value() };

		if (queue_family_indices.GraphicsFamily != queue_family_indices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(Device, &createInfo, nullptr, &SwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create swap chain!");
		}

		vkGetSwapchainImagesKHR(Device, SwapChain, &imageCount, nullptr);
		SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(Device, SwapChain, &imageCount, SwapChainImages.data());

		SwapChainImageFormat = surfaceFormat.format;
		SwapChainExtent = extent;
	}

	/** 重新创建SwapChain*/
	void RecreateSwapChain()
	{
		// 当窗口长宽都是零时，说明窗口被最小化了，这时需要等待
		int Width = 0, Height = 0;
		glfwGetFramebufferSize(Window, &Width, &Height);
		while (Width == 0 || Height == 0) {
			glfwGetFramebufferSize(Window, &Width, &Height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(Device);

		CleanupSwapChain();

		CreateSwapChain();
		CreateSwapChainImageViews();
		CreateFramebuffers();
#if ENABLE_DEFEERED_RENDERING
		CreateBaseSceneDeferredPass();
#endif
	}

	/** 清理旧的SwapChain*/
	void CleanupSwapChain() {
		vkDestroyImageView(Device, DepthImageView, nullptr);
		vkDestroyImage(Device, DepthImage, nullptr);
		vkFreeMemory(Device, DepthImageMemory, nullptr);

		for (auto framebuffer : SwapChainFramebuffers) {
			vkDestroyFramebuffer(Device, framebuffer, nullptr);
		}

		for (auto imageView : SwapChainImageViews) {
			vkDestroyImageView(Device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(Device, SwapChain, nullptr);
	}

	/** 图像视图 Image View
	 * 将视图显示为图像
	 * ImageView定义了SwapChain里定义的图像是什么样的
	 * 比如，带深度信息的RGB图像
	*/
	void CreateSwapChainImageViews()
	{
		SwapChainImageViews.resize(SwapChainImages.size());

		for (size_t i = 0; i < SwapChainImages.size(); i++)
		{
			CreateImageView(SwapChainImageViews[i], SwapChainImages[i], SwapChainImageFormat);
		}
	}

	/** 渲染层 RenderPass
	 * 创建渲染管线之前，需要先创建渲染层，告诉Vulkan渲染时使用的帧缓存FrameBuffer
	 * 我们需要指定渲染中使用的颜色缓存和深度缓存的数量，以及采样信息
	*/
	void CreateRenderPass()
	{
		VkAttachmentDescription ColorAttachment{};
		ColorAttachment.format = SwapChainImageFormat;
		ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription DepthAttachment{};
		DepthAttachment.format = FindDepthFormat();
		DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
#if !ENABLE_DEFEERED_RENDERING
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
#else
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
#endif
		DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
#if !ENABLE_DEFEERED_RENDERING
		DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
#else
		DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
#endif
		DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference ColorAttachmentRef{};
		ColorAttachmentRef.attachment = 0;
		ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference DepthAttachmentRef{};
		DepthAttachmentRef.attachment = 1;
		DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// 渲染子通道 SubPass
		// SubPass是RenderPass的下属任务，和RenderPass共享Framebuffer等渲染资源
		// 某些渲染操作，比如后处理的Blooming，当前渲染需要依赖上一个渲染结果，但是渲染资源不变，这是SubPass可以优化性能
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &ColorAttachmentRef;
		subpass.pDepthStencilAttachment = &DepthAttachmentRef;

		// 这里将渲染三角形的操作，简化成一个SubPass提交
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { ColorAttachment, DepthAttachment };
		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassCI.pAttachments = attachments.data();
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpass;
		renderPassCI.dependencyCount = 1;
		renderPassCI.pDependencies = &dependency;
		if (vkCreateRenderPass(Device, &renderPassCI, nullptr, &MainRenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create render pass!");
		}
	}

	/** 创建指令池，管理所有的指令，比如DrawCall或者内存交换等*/
	void CreateCommandPool()
	{
		FQueueFamilyIndices queueFamilyIndices = FindQueueFamilies(PhysicalDevice);

		VkCommandPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolCI.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

		if (vkCreateCommandPool(Device, &poolCI, nullptr, &CommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create command pool!");
		}
	}

	/** 创建帧缓存，即每帧图像对应的渲染数据*/
	void CreateFramebuffers()
	{
		// 创建深度纹理资源
		VkFormat depthFormat = FindDepthFormat();
		CreateImage(DepthImage, DepthImageMemory, SwapChainExtent.width, SwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(DepthImageView, DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		SwapChainFramebuffers.resize(SwapChainImageViews.size());

		for (size_t i = 0; i < SwapChainImageViews.size(); i++) 
		{
			std::array<VkImageView, 2> attachments = 
			{
				SwapChainImageViews[i],
				DepthImageView
			};
			VkFramebufferCreateInfo frameBufferCI{};
			frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferCI.renderPass = MainRenderPass;
			frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCI.pAttachments = attachments.data();
			frameBufferCI.width = SwapChainExtent.width;
			frameBufferCI.height = SwapChainExtent.height;
			frameBufferCI.layers = 1;

			if (vkCreateFramebuffer(Device, &frameBufferCI, nullptr, &SwapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to Create framebuffer!");
			}
		}
	}


	/**
	 * 创建阴影贴图资源 Shadow map
	*/
	void CreateShadowmapPass()
	{
		ShadowmapPass.Width = SHADOWMAP_DIM;
		ShadowmapPass.Height = SHADOWMAP_DIM;
		ShadowmapPass.Format = FindDepthFormat();
		CreateImage(
			ShadowmapPass.Image,
			ShadowmapPass.Memory,
			ShadowmapPass.Width, ShadowmapPass.Height, ShadowmapPass.Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(ShadowmapPass.ImageView, ShadowmapPass.Image, ShadowmapPass.Format, VK_IMAGE_ASPECT_DEPTH_BIT);
		CreateSampler(ShadowmapPass.Sampler,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE); // @TODO: 是否应该将 sample filter 改成 VK_FILTER_NEAREST

		//////////////////////////////////////////////////////////
		// 创建 UniformBuffers 和 UniformBuffersMemory
		VkDeviceSize bufferSize = sizeof(FUniformBufferBase);
		ShadowmapPass.UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		ShadowmapPass.UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				ShadowmapPass.UniformBuffers[i],
				ShadowmapPass.UniformBuffersMemory[i]);
		}

		//////////////////////////////////////////////////////////
		// 创建 DescriptorSetLayout
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.resize(1);
		bindings[0] = uboLayoutBinding;
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &ShadowmapPass.DescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create descriptor set layout!");
		}

		//////////////////////////////////////////////////////////
		// 创建 DescriptorPool
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.resize(1);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCI.pPoolSizes = poolSizes.data();
		poolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		if (vkCreateDescriptorPool(Device, &poolCI, nullptr, &ShadowmapPass.DescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create descriptor pool!");
		}

		//////////////////////////////////////////////////////////
		// 创建 DescriptorSets
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, ShadowmapPass.DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = ShadowmapPass.DescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();
		ShadowmapPass.DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(Device, &allocInfo, ShadowmapPass.DescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		//////////////////////////////////////////////////////////
		// 绑定 DescriptorSets
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites{};
			descriptorWrites.resize(1);
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = ShadowmapPass.UniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(FUniformBufferBase);
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = ShadowmapPass.DescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			vkUpdateDescriptorSets(Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.format = ShadowmapPass.Format;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		// Clear depth at beginning of the render pass
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// We will read from depth, so it's important to store the depth attachment results
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// We don't care about initial layout of the attachment
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// Attachment will be transitioned to shader read at render pass end
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 0;
		// Attachment will be used as depth/stencil during render pass
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		// No color attachments
		subpass.colorAttachmentCount = 0;
		// Reference to our depth attachment
		subpass.pDepthStencilAttachment = &depthReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &attachmentDescription;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());;
		renderPassCreateInfo.pDependencies = dependencies.data();

		if (vkCreateRenderPass(Device, &renderPassCreateInfo, nullptr, &ShadowmapPass.RenderPass)) {
			throw std::runtime_error("failed to Create shadow map render pass!");
		}

		// Create frame buffer
		VkFramebufferCreateInfo frameBufferCreateInfo{};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = ShadowmapPass.RenderPass;
		frameBufferCreateInfo.attachmentCount = 1;
		frameBufferCreateInfo.pAttachments = &ShadowmapPass.ImageView;
		frameBufferCreateInfo.width = ShadowmapPass.Width;
		frameBufferCreateInfo.height = ShadowmapPass.Height;
		frameBufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(Device, &frameBufferCreateInfo, nullptr, &ShadowmapPass.FrameBuffer)) {
			throw std::runtime_error("failed to Create shadow map frame buffer!");
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.depthClampEnable = VK_FALSE;
		rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.lineWidth = 1.0f;
		rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT /*VK_CULL_MODE_BACK_BIT*/;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.depthBiasEnable = VK_FALSE;
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentState.blendEnable = VK_FALSE;
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.logicOpEnable = VK_FALSE;
		colorBlendStateCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;
		colorBlendStateCI.blendConstants[0] = 0.0f;
		colorBlendStateCI.blendConstants[1] = 0.0f;
		colorBlendStateCI.blendConstants[2] = 0.0f;
		colorBlendStateCI.blendConstants[3] = 0.0f;
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateCI.minDepthBounds = 0.0f; // Optional
		depthStencilStateCI.maxDepthBounds = 1.0f; // Optional
		depthStencilStateCI.stencilTestEnable = VK_FALSE; // 没有写轮廓信息，所以跳过轮廓测试
		depthStencilStateCI.front = {}; // Optional
		depthStencilStateCI.back = {}; // Optional
		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.sampleShadingEnable = VK_FALSE;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();

		auto vertShaderCode = LoadShaderSource("Resources/Shaders/draw_with_deferred_sm_vert.spv");
		auto fragShaderCode = LoadShaderSource("Resources/Shaders/draw_with_deferred_sm_frag.spv");
		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);
		VkPipelineShaderStageCreateInfo vertShaderStageCI{};
		vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageCI.module = vertShaderModule;
		vertShaderStageCI.pName = "main";
		VkPipelineShaderStageCreateInfo fragShaderStageCI{};
		fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageCI.module = fragShaderModule;
		fragShaderStageCI.pName = "main";
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCI, fragShaderStageCI };

		VkPipelineVertexInputStateCreateInfo vertexInputCI{};
		auto bindingDescription = FVertex::GetBindingDescription();
		auto attributeDescriptions = FVertex::GetAttributeDescriptions();
		vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCI.vertexBindingDescriptionCount = 1;
		vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputCI.pVertexBindingDescriptions = &bindingDescription;
		vertexInputCI.pVertexAttributeDescriptions = attributeDescriptions.data();

		// 设置 push constants
		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(FGlobalConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_ALL;

		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &ShadowmapPass.DescriptorSetLayout;
		pipelineLayoutCI.pushConstantRangeCount = 1;
		pipelineLayoutCI.pPushConstantRanges = &pushConstant;

		if (vkCreatePipelineLayout(Device, &pipelineLayoutCI, nullptr, &ShadowmapPass.PipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages;
		pipelineCI.pVertexInputState = &vertexInputCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI; // 加上深度测试
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.layout = ShadowmapPass.PipelineLayout;
		pipelineCI.renderPass = ShadowmapPass.RenderPass;
		pipelineCI.subpass = 0;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;

		// Offscreen pipeline (vertex shader only)
		//shaderStages[0] = loadShader(getShadersPath() + "shadowmapping/offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		pipelineCI.stageCount = 1;
		// No blend attachment states (no color attachments used)
		colorBlendStateCI.attachmentCount = 0;
		// Disable culling, so all faces contribute to shadows
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		// Enable depth bias
		rasterizationStateCI.depthBiasEnable = VK_TRUE;
		// Add depth bias to dynamic state, so we can change it at runtime
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();

		pipelineCI.renderPass = ShadowmapPass.RenderPass;

		if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &ShadowmapPass.Pipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create graphics pipeline!");
		}

		// Create vertex instanced pipeline
		auto vertInstancedShaderCode = LoadShaderSource("Resources/Shaders/draw_with_deferred_sm_instanced_vert.spv");
		VkShaderModule vertInstancedShaderModule = CreateShaderModule(vertInstancedShaderCode);
		VkPipelineShaderStageCreateInfo vertInstancedShaderStageCI{};
		vertInstancedShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertInstancedShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertInstancedShaderStageCI.module = vertInstancedShaderModule;
		vertInstancedShaderStageCI.pName = "main";
		auto bindingInstancedDescriptions = FVertex::GetBindingInstancedDescriptions();
		auto attributeInstancedDescriptions = FVertex::GetAttributeInstancedDescriptions();
		vertexInputCI.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingInstancedDescriptions.size());
		vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeInstancedDescriptions.size());
		vertexInputCI.pVertexBindingDescriptions = bindingInstancedDescriptions.data();
		vertexInputCI.pVertexAttributeDescriptions = attributeInstancedDescriptions.data();
		pipelineCI.pVertexInputState = &vertexInputCI;
		shaderStages[0] = vertInstancedShaderStageCI;
		pipelineCI.pStages = shaderStages;
		if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &ShadowmapPass.PipelineInstanced) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create graphics pipeline!");
		}

		vkDestroyShaderModule(Device, fragShaderModule, nullptr);
		vkDestroyShaderModule(Device, vertShaderModule, nullptr);
		vkDestroyShaderModule(Device, vertInstancedShaderModule, nullptr);
	}

	/** Cube map Faces Rules:
	 *       Y3
	 *       ||
	 * X1 == Z4 == X0 == Z5
	 *       ||
	 *       Y4
	 * | https://matheowis.github.io/HDRI-to-CubeMap/    |
	 * | X0(p-x) | X1(n-x) | Y2(p-z) | Y3(n-z) | Z4(n-y) | Z5(p-y)|
	 * |    90   |   -90   |    0    |   180   |    0    |   180  |
	 */
	void CreateCubemapResources()
	{
		CreateImageHDRContext(CubemapImage, CubemapImageMemory, CubemapImageView, CubemapSampler, CubemapMaxMips, {
			"Resources/Textures/cubemap_X0.png",
			"Resources/Textures/cubemap_X1.png",
			"Resources/Textures/cubemap_Y2.png",
			"Resources/Textures/cubemap_Y3.png",
			"Resources/Textures/cubemap_Z4.png",
			"Resources/Textures/cubemap_Z5.png" });
	}

	/** 创建统一缓存区（UBO）*/
	void CreateUniformBuffers()
	{
		VkDeviceSize baseBufferSize = sizeof(FUniformBufferBase);
		BaseUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		BaseUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(baseBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				BaseUniformBuffers[i],
				BaseUniformBuffersMemory[i]);

			// 这里会导致 memory stack overflow ，不应该在这里 vkMapMemory
			//vkMapMemory(Device, BaseUniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}

		VkDeviceSize bufferSizeOfView = sizeof(FUniformBufferView);
		ViewUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		ViewUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(
				bufferSizeOfView,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				ViewUniformBuffers[i],
				ViewUniformBuffersMemory[i]);
		}

		FLight Moonlight;
		Moonlight.Position = glm::vec4(20.0f, 0.0f, 20.0f, 0.0);
		Moonlight.Color = glm::vec4(0.0, 0.1, 0.6, 15.0);
		Moonlight.Direction = glm::vec4(glm::normalize(glm::vec3(Moonlight.Position.x, Moonlight.Position.y, Moonlight.Position.z)), 0.0);
		Moonlight.LightInfo = glm::vec4(0.0, 0.0, 0.0, 0.0);
		View.DirectionalLights[0] = Moonlight;
		uint32_t PointLightNum = POINT_LIGHTS_NUM;
		for (uint32_t i = 0; i < PointLightNum; i++)
		{
			FLight PointLight;
			float radians = (((float)RandRange(0, 3600) / 10.0f));
			float distance = (((float)RandRange(0, 500) / 100.0f)) + 1.0f;
			float X = sin(glm::radians(radians)) * distance;
			float Y = cos(glm::radians(radians)) * distance;
			float Z = 1.0;
			PointLight.Position = glm::vec4(glm::vec3(X, Y, Z), 0.0);
			float R = (((float)RandRange(50, 75) / 100.0f));
			float G = (((float)RandRange(25, 50) / 100.0f));
			float B = 0.0;
			PointLight.Color = glm::vec4(R, G, B, 10.0);
			PointLight.Direction = glm::vec4(0.0, 0.0, 1.0, 1.5);
			PointLight.LightInfo = glm::vec4(0.0, 0.0, 0.0, 0.0);
			View.PointLights[i] = PointLight;
		}
		View.LightsCount = glm::ivec4(1, PointLightNum, 0, CubemapMaxMips);
	}

	void CreateBaseScenePass()
	{
		// 创建场景渲染流水线和着色器
		uint32_t SpecConstantsCount = GlobalConstants.SpecConstantsCount;
		CreateDescriptorSetLayout(BaseScenePass.DescriptorSetLayout, PBR_SAMPLER_NUMBER);
		BaseScenePass.Pipelines.resize(SpecConstantsCount);
		BaseScenePass.PipelinesInstanced.resize(SpecConstantsCount);
		CreatePipelineLayout(BaseScenePass.PipelineLayout, BaseScenePass.DescriptorSetLayout);
		CreateGraphicsPipelines(
			BaseScenePass.Pipelines,
			BaseScenePass.PipelineLayout, 
			MainRenderPass,
			SpecConstantsCount,
			"Resources/Shaders/draw_with_deferred_base_vert.spv",
			"Resources/Shaders/draw_with_deferred_base_frag.spv",
			true/*bDepthTest*/, true/*bCullBack*/, false/*bInstanced*/);
		CreateGraphicsPipelines(
			BaseScenePass.PipelinesInstanced,
			BaseScenePass.PipelineLayout,
			MainRenderPass,
			SpecConstantsCount,
			"Resources/Shaders/draw_with_deferred_base_instanced_vert.spv",
			"Resources/Shaders/draw_with_deferred_base_frag.spv",
			true/*bDepthTest*/, true/*bCullBack*/, true/*bInstanced*/);

#if !ENABLE_DEFEERED_RENDERING
		CreateBaseSceneResources();
#endif
		//~ 结束 创建场景，包括VBO，UBO，贴图等
	}

	void CreateBaseSceneIndirectPass()
	{
		// 创建场景渲染流水线和着色器
		CreateDescriptorSetLayout(BaseSceneIndirectPass.DescriptorSetLayout, PBR_SAMPLER_NUMBER);
		uint32_t SpecConstantsCount = GlobalConstants.SpecConstantsCount;
		BaseSceneIndirectPass.Pipelines.resize(SpecConstantsCount);
		BaseSceneIndirectPass.PipelinesInstanced.resize(SpecConstantsCount);
		CreatePipelineLayout(BaseSceneIndirectPass.PipelineLayout, BaseSceneIndirectPass.DescriptorSetLayout);
		CreateGraphicsPipelines(
			BaseSceneIndirectPass.Pipelines,
			BaseSceneIndirectPass.PipelineLayout,
			MainRenderPass,
			SpecConstantsCount,
			"Resources/Shaders/draw_with_deferred_base_vert.spv",
			"Resources/Shaders/draw_with_deferred_base_frag.spv",
			true/*bDepthTest*/, true/*bCullBack*/, false/*bInstanced*/);
		CreateGraphicsPipelines(
			BaseSceneIndirectPass.PipelinesInstanced,
			BaseSceneIndirectPass.PipelineLayout,
			MainRenderPass,
			SpecConstantsCount,
			"Resources/Shaders/draw_with_deferred_base_instanced_vert.spv",
			"Resources/Shaders/draw_with_deferred_base_frag.spv",
			true/*bDepthTest*/, true/*bCullBack*/, true/*bInstanced*/);

		if (ENABLE_INDIRECT_DRAW)
		{
			FRenderIndirectObject cube;
			std::string cube_obj = "Resources/Models/cube.obj";
			std::vector<std::string> cube_imgs = {
				"Resources/Textures/default_grey.png",		// BaseColor
				"Resources/Textures/default_black.png",		// Metallic
				"Resources/Textures/default_white.png",		// Roughness
				"Resources/Textures/default_normal.png",	// Normal
				"Resources/Textures/default_white.png",		// AmbientOcclution
				"Resources/Textures/default_black.png",		// Emissive
				"Resources/Textures/default_white.png" };	// Mask
			CreateRenderObject<FRenderIndirectObject>(cube, cube_obj, cube_imgs, BaseSceneIndirectPass.DescriptorSetLayout);

			cube.IndirectCommands.clear();

			VkDrawIndexedIndirectCommand indirectCmd{};
			indirectCmd.indexCount = (uint32_t)cube.MeshData.Indices.size(); /*indexCount*/
			indirectCmd.instanceCount = 1; /*instanceCount*/
			indirectCmd.firstIndex = 0; /*firstIndex*/
			indirectCmd.vertexOffset = 0; /*vertexOffset*/
			indirectCmd.firstInstance = 0; /*firstInstance*/
			cube.IndirectCommands.push_back(indirectCmd);

			CreateRenderIndirectBuffer<FRenderIndirectObject>(cube);
			BaseSceneIndirectPass.RenderIndirectObject.push_back(cube);

			FRenderIndirectInstancedObject cube_inst;
			std::string cube_inst_obj = "Resources/Models/cube.obj";
			std::vector<std::string> cube_inst_imgs = {
				"Resources/Textures/default_grey.png",		// BaseColor
				"Resources/Textures/default_black.png",		// Metallic
				"Resources/Textures/default_white.png",		// Roughness
				"Resources/Textures/default_normal.png",	// Normal
				"Resources/Textures/default_white.png",		// AmbientOcclution
				"Resources/Textures/default_black.png",		// Emissive
				"Resources/Textures/default_white.png" };	// Mask
			CreateRenderObject<FRenderIndirectInstancedObject>(cube_inst, cube_inst_obj, cube_inst_imgs, BaseSceneIndirectPass.DescriptorSetLayout);

			cube_inst.IndirectCommands.clear();

			std::vector<FInstanceData> instanceData;
			instanceData.resize(INSTANCE_COUNT);

			for (uint32_t i = 0; i < INSTANCE_COUNT; i++) {
				VkDrawIndexedIndirectCommand indirectCmd{};
				indirectCmd.indexCount = (uint32_t)cube_inst.MeshData.Indices.size(); /*indexCount*/
				indirectCmd.instanceCount = INSTANCE_COUNT; /*instanceCount*/
				indirectCmd.firstIndex = 0; /*firstIndex*/
				indirectCmd.vertexOffset = 0; /*vertexOffset*/
				indirectCmd.firstInstance = 0; /*firstInstance*/

				cube_inst.IndirectCommands.push_back(indirectCmd);

				float X = (((float)RandRange(-199, 199) / 100.0f));
				float Y = (((float)RandRange(-199, 199) / 100.0f));
				float Z = RandRange(0, 99) / 250.0f;
				instanceData[i].InstancePosition = glm::vec3(X, Y, Z);
				// Y(Pitch), Z(Yaw), X(Roll)
				float Yaw = float(M_PI) * RandRange(0, 99) / 100.0f;
				float Pitch = float(M_PI) * RandRange(0, 99) / 100.0f;
				float Roll = float(M_PI) * RandRange(0, 99) / 100.0f;
				instanceData[i].InstanceRotation = glm::vec3(Pitch, Yaw, Roll);
				instanceData[i].InstancePScale = RandRange(0, 99) / 500.0f;
				instanceData[i].InstanceTexIndex = RandRange(0, 99);
			}
			CreateInstancedBuffer<FRenderIndirectInstancedObject>(cube_inst, instanceData);
			CreateRenderIndirectBuffer<FRenderIndirectInstancedObject>(cube_inst);
			BaseSceneIndirectPass.RenderIndirectInstancedObject.push_back(cube_inst);
		}
	}

	/** 创建GBuffer, 用于延迟渲染*/
	void CreateBaseSceneDeferredPass()
	{
		auto CreateGraphicsPipelinesDeferred = [this](
			std::vector<VkPipeline>& outPipelines,
			const VkPipelineLayout& inPipelineLayout,
			const VkRenderPass& inRenderPass,
			const uint32_t inSpecConstantsCount,
			const EVertexType VertexType,
			const  EGraphicPipelineType GraphicPipelineType,
			const std::string& inVertFilename,
			const std::string& inFragFilename)
		{
			auto vertShaderCode = LoadShaderSource(inVertFilename);
			auto fragShaderCode = LoadShaderSource(inFragFilename);

			VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
			VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

			VkPipelineShaderStageCreateInfo vertShaderStageCI{};
			vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageCI.module = vertShaderModule;
			vertShaderStageCI.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageCI{};
			fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageCI.module = fragShaderModule;
			fragShaderStageCI.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCI, fragShaderStageCI };

			// 顶点缓存绑定的描述，定义了顶点都需要绑定什么数据，比如第一个位置绑定Position，第二个位置绑定Color，第三个位置绑定UV等
			auto bindingDescription = FVertex::GetBindingDescription();
			auto attributeDescriptions = FVertex::GetAttributeDescriptions();
			auto bindingInstancedDescriptions = FVertex::GetBindingInstancedDescriptions();
			auto attributeInstancedDescriptions = FVertex::GetAttributeInstancedDescriptions();

			// 渲染管线VertexBuffer输入
			VkPipelineVertexInputStateCreateInfo vertexInputCI{};
			switch (VertexType)
			{
			case VertexIndexed:
			{
				vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputCI.vertexBindingDescriptionCount = 1;
				vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
				vertexInputCI.pVertexBindingDescriptions = &bindingDescription;
				vertexInputCI.pVertexAttributeDescriptions = attributeDescriptions.data();
				break;
			}
			case Instanced:
			{
				vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputCI.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingInstancedDescriptions.size());
				vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeInstancedDescriptions.size());
				vertexInputCI.pVertexBindingDescriptions = bindingInstancedDescriptions.data();
				vertexInputCI.pVertexAttributeDescriptions = attributeInstancedDescriptions.data();
				break;
			}
			case ScreenRect:
			{
				vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputCI.vertexBindingDescriptionCount = 0;
				vertexInputCI.vertexAttributeDescriptionCount = 0;
			}
			default:
				break;
			}

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
			inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyCI.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportStateCI{};
			viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCI.viewportCount = 1;
			viewportStateCI.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizerCI{};
			rasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCI.depthClampEnable = VK_FALSE;
			rasterizerCI.rasterizerDiscardEnable = VK_FALSE;
			rasterizerCI.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerCI.lineWidth = 1.0f;
			// @NOTE: hard-coded for foliage single face render
			rasterizerCI.cullMode = VK_CULL_MODE_NONE;
			rasterizerCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCI.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multiSamplingCI{};
			multiSamplingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multiSamplingCI.sampleShadingEnable = VK_FALSE;
			multiSamplingCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			// 打开深度测试
			VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
			depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilCI.depthTestEnable = (VertexType != ScreenRect) ? VK_TRUE : VK_FALSE;
			depthStencilCI.depthWriteEnable = (VertexType != ScreenRect) ? VK_TRUE : VK_FALSE;
			depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencilCI.depthBoundsTestEnable = VK_FALSE;
			depthStencilCI.minDepthBounds = 0.0f; // Optional
			depthStencilCI.maxDepthBounds = 1.0f; // Optional
			depthStencilCI.stencilTestEnable = VK_FALSE; // 没有写轮廓信息，所以跳过轮廓测试
			depthStencilCI.front = {}; // Optional
			depthStencilCI.back = {}; // Optional

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			VkPipelineColorBlendStateCreateInfo colorBlendingCI{};
			colorBlendingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendingCI.logicOpEnable = VK_FALSE;
			colorBlendingCI.logicOp = VK_LOGIC_OP_COPY;
			colorBlendingCI.blendConstants[0] = 0.0f;
			colorBlendingCI.blendConstants[1] = 0.0f;
			colorBlendingCI.blendConstants[2] = 0.0f;
			colorBlendingCI.blendConstants[3] = 0.0f;
			switch (GraphicPipelineType)
			{
			case Forward:
			{
				colorBlendingCI.attachmentCount = 1;
				colorBlendingCI.pAttachments = &colorBlendAttachment;
				break;
			}
			case DeferredScene:
			{
				std::array<VkPipelineColorBlendAttachmentState, 5> colorBlendAttachments;
				colorBlendAttachments[0] = colorBlendAttachment;
				colorBlendAttachments[1] = colorBlendAttachment;
				colorBlendAttachments[2] = colorBlendAttachment;
				colorBlendAttachments[3] = colorBlendAttachment;
				colorBlendAttachments[4] = colorBlendAttachment;
				colorBlendingCI.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
				colorBlendingCI.pAttachments = colorBlendAttachments.data();
				break;
			}
			case DeferredLighting:
			{
				colorBlendingCI.attachmentCount = 1;
				colorBlendingCI.pAttachments = &colorBlendAttachment;
				break;
			}
			default:
				break;
			}

			std::vector<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			VkPipelineDynamicStateCreateInfo dynamicStateCI{};
			dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicStateCI.pDynamicStates = dynamicStates.data();

			VkGraphicsPipelineCreateInfo pipelineCI{};
			pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCI.stageCount = 2;
			pipelineCI.pStages = shaderStages;
			pipelineCI.pVertexInputState = &vertexInputCI;
			pipelineCI.pInputAssemblyState = &inputAssemblyCI;
			pipelineCI.pViewportState = &viewportStateCI;
			pipelineCI.pRasterizationState = &rasterizerCI;
			pipelineCI.pMultisampleState = &multiSamplingCI;
			pipelineCI.pDepthStencilState = &depthStencilCI; // 加上深度测试
			pipelineCI.pColorBlendState = &colorBlendingCI;
			pipelineCI.pDynamicState = &dynamicStateCI;
			pipelineCI.layout = inPipelineLayout;
			pipelineCI.renderPass = inRenderPass;
			pipelineCI.subpass = 0;
			pipelineCI.basePipelineHandle = VK_NULL_HANDLE;

			// Use specialization constants 优化着色器变体
			for (uint32_t i = 0; i < inSpecConstantsCount; i++)
			{
				uint32_t SpecConstants = i;
				VkSpecializationMapEntry specializationMapEntry = VkSpecializationMapEntry{};
				specializationMapEntry.constantID = 0;
				specializationMapEntry.offset = 0;
				specializationMapEntry.size = sizeof(uint32_t);
				VkSpecializationInfo specializationCI = VkSpecializationInfo();
				specializationCI.mapEntryCount = 1;
				specializationCI.pMapEntries = &specializationMapEntry;
				specializationCI.dataSize = sizeof(uint32_t);
				specializationCI.pData = &SpecConstants;
				shaderStages[1].pSpecializationInfo = &specializationCI;

				if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &outPipelines[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("failed to Create graphics pipeline!");
				}
			}

			vkDestroyShaderModule(Device, fragShaderModule, nullptr);
			vkDestroyShaderModule(Device, vertShaderModule, nullptr);
		};

		// Depth Stencil (Currently depth-only)
		GBuffer.DepthStencilFormat = VK_FORMAT_D32_SFLOAT;
		CreateImage(GBuffer.DepthStencilImage, GBuffer.DepthStencilMemory, SwapChainExtent.width, SwapChainExtent.height, GBuffer.DepthStencilFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(GBuffer.DepthStencilImageView, GBuffer.DepthStencilImage, GBuffer.DepthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		CreateSampler(GBuffer.DepthStencilSampler);

		// Scene Color
		GBuffer.SceneColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
		CreateImage(GBuffer.SceneColorImage, GBuffer.SceneColorMemory, SwapChainExtent.width, SwapChainExtent.height, GBuffer.SceneColorFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(GBuffer.SceneColorImageView, GBuffer.SceneColorImage, GBuffer.SceneColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		CreateSampler(GBuffer.SceneColorSampler);

		// GBufferA Normal+(CastShadow+Masked)
		GBuffer.GBufferAFormat = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		CreateImage(GBuffer.GBufferAImage, GBuffer.GBufferAMemory, SwapChainExtent.width, SwapChainExtent.height, GBuffer.GBufferAFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(GBuffer.GBufferAImageView, GBuffer.GBufferAImage, GBuffer.GBufferAFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		CreateSampler(GBuffer.GBufferASampler);

		// GBufferB M+S+R+(ShadingModelID+SelectiveOutputMask) for unreal
		// GBufferB M+S+R+(OpacityMask) for me
		GBuffer.GBufferBFormat = VK_FORMAT_R8G8B8A8_UNORM;
		CreateImage(GBuffer.GBufferBImage, GBuffer.GBufferBMemory, SwapChainExtent.width, SwapChainExtent.height, GBuffer.GBufferBFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(GBuffer.GBufferBImageView, GBuffer.GBufferBImage, GBuffer.GBufferBFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		CreateSampler(GBuffer.GBufferBSampler);

		// GBufferC BaseColor + AO
		GBuffer.GBufferCFormat = VK_FORMAT_R8G8B8A8_UNORM;
		CreateImage(GBuffer.GBufferCImage, GBuffer.GBufferCMemory, SwapChainExtent.width, SwapChainExtent.height, GBuffer.GBufferCFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(GBuffer.GBufferCImageView, GBuffer.GBufferCImage, GBuffer.GBufferCFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		CreateSampler(GBuffer.GBufferCSampler);

		// GBufferD Position + ID
		GBuffer.GBufferDFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		CreateImage(GBuffer.GBufferDImage, GBuffer.GBufferDMemory, SwapChainExtent.width, SwapChainExtent.height, GBuffer.GBufferDFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CreateImageView(GBuffer.GBufferDImageView, GBuffer.GBufferDImage, GBuffer.GBufferDFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		CreateSampler(GBuffer.GBufferDSampler);

		VkAttachmentDescription DepthAttachment{};
		DepthAttachment.format = FindDepthFormat();
		DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentDescription ColorAttachment{};
		ColorAttachment.format = SwapChainImageFormat;
		ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::array<VkAttachmentDescription, 6> AttachmentDescriptions = {};
		AttachmentDescriptions[0] = DepthAttachment;
		AttachmentDescriptions[1] = ColorAttachment;
		AttachmentDescriptions[2] = ColorAttachment;
		AttachmentDescriptions[3] = ColorAttachment;
		AttachmentDescriptions[4] = ColorAttachment;
		AttachmentDescriptions[5] = ColorAttachment;
		AttachmentDescriptions[0].format = GBuffer.DepthStencilFormat;
		AttachmentDescriptions[1].format = GBuffer.SceneColorFormat;
		AttachmentDescriptions[2].format = GBuffer.GBufferAFormat;
		AttachmentDescriptions[3].format = GBuffer.GBufferBFormat;
		AttachmentDescriptions[4].format = GBuffer.GBufferCFormat;
		AttachmentDescriptions[5].format = GBuffer.GBufferDFormat;

		VkAttachmentReference DepthAttachmentRef = {};
		DepthAttachmentRef.attachment = 0;
		DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		std::vector<VkAttachmentReference> ColorAttachmentRefs;
		ColorAttachmentRefs.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		ColorAttachmentRefs.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		ColorAttachmentRefs.push_back({ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		ColorAttachmentRefs.push_back({ 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		ColorAttachmentRefs.push_back({ 5, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(ColorAttachmentRefs.size());
		subpass.pColorAttachments = ColorAttachmentRefs.data();
		subpass.pDepthStencilAttachment = &DepthAttachmentRef;

		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassCI = {};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(AttachmentDescriptions.size());
		renderPassCI.pAttachments = AttachmentDescriptions.data();
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpass;
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassCI.pDependencies = dependencies.data();

		if (vkCreateRenderPass(Device, &renderPassCI, nullptr, &BaseSceneDeferredPass.SceneRenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create render pass!");
		}

		std::array<VkImageView, 6> attachments =
		{
			GBuffer.DepthStencilImageView,
			GBuffer.SceneColorImageView,
			GBuffer.GBufferAImageView,
			GBuffer.GBufferBImageView,
			GBuffer.GBufferCImageView,
			GBuffer.GBufferDImageView,
		};

		VkFramebufferCreateInfo frameBufferCI{};
		frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCI.renderPass = BaseSceneDeferredPass.SceneRenderPass;
		frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameBufferCI.pAttachments = attachments.data();
		frameBufferCI.width = SwapChainExtent.width;
		frameBufferCI.height = SwapChainExtent.height;
		frameBufferCI.layers = 1;

		if (vkCreateFramebuffer(Device, &frameBufferCI, nullptr, &BaseSceneDeferredPass.SceneFrameBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create framebuffer!");
		}

		CreateDescriptorSetLayout(BaseSceneDeferredPass.SceneDescriptorSetLayout, PBR_SAMPLER_NUMBER);
		BaseSceneDeferredPass.ScenePipelines.resize(GlobalConstants.SpecConstantsCount);
		BaseSceneDeferredPass.ScenePipelinesInstanced.resize(GlobalConstants.SpecConstantsCount);
		CreatePipelineLayout(BaseSceneDeferredPass.ScenePipelineLayout, BaseSceneDeferredPass.SceneDescriptorSetLayout);
		CreateGraphicsPipelinesDeferred(
			BaseSceneDeferredPass.ScenePipelines,
			BaseSceneDeferredPass.ScenePipelineLayout,
			BaseSceneDeferredPass.SceneRenderPass,
			GlobalConstants.SpecConstantsCount, VertexIndexed, DeferredScene,
			"Resources/Shaders/draw_with_deferred_base_vert.spv",
			"Resources/Shaders/draw_with_deferred_scene_frag.spv");
		CreateGraphicsPipelinesDeferred(
			BaseSceneDeferredPass.ScenePipelinesInstanced,
			BaseSceneDeferredPass.ScenePipelineLayout,
			BaseSceneDeferredPass.SceneRenderPass,
			GlobalConstants.SpecConstantsCount, Instanced, DeferredScene,
			"Resources/Shaders/draw_with_deferred_base_instanced_vert.spv",
			"Resources/Shaders/draw_with_deferred_scene_frag.spv");

		/** Create DescriptorSetLayout for Lighting*/
		// UnifromBufferObject（ubo）绑定
		VkDescriptorSetLayoutBinding viewLayoutBinding{};
		viewLayoutBinding.binding = 0;
		viewLayoutBinding.descriptorCount = 1;
		viewLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		viewLayoutBinding.pImmutableSamplers = nullptr;
		viewLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// 环境反射Cubemap贴图绑定
		VkDescriptorSetLayoutBinding cubemapLayoutBinding{};
		cubemapLayoutBinding.binding = 1;
		cubemapLayoutBinding.descriptorCount = 1;
		cubemapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		cubemapLayoutBinding.pImmutableSamplers = nullptr;
		cubemapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// 阴影Shadowmap贴图绑定
		VkDescriptorSetLayoutBinding shadowmapLayoutBinding{};
		shadowmapLayoutBinding.binding = 2;
		shadowmapLayoutBinding.descriptorCount = 1;
		shadowmapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		shadowmapLayoutBinding.pImmutableSamplers = nullptr;
		shadowmapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// 将UnifromBufferObject和贴图采样器绑定到DescriptorSetLayout上
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.resize(9);
		bindings[0] = viewLayoutBinding;
		bindings[1] = cubemapLayoutBinding;
		bindings[2] = shadowmapLayoutBinding;
		for (size_t i = 0; i < 6; i++)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding{};
			samplerLayoutBinding.binding = static_cast<uint32_t>(i + 3);
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			bindings[i + 3] = samplerLayoutBinding;
		}
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &BaseSceneDeferredPass.LightingDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create descriptor set layout!");
		}

		/** Create DescriptorPool for Lighting*/
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.resize(9);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < 6; i++)
		{
			poolSizes[i + 3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[i + 3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		}

		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCI.pPoolSizes = poolSizes.data();
		poolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(Device, &poolCI, nullptr, &BaseSceneDeferredPass.LightingDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create descriptor pool!");
		}

		/** 创建DescriptorSets*/
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, BaseSceneDeferredPass.LightingDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = BaseSceneDeferredPass.LightingDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		BaseSceneDeferredPass.LightingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(Device, &allocInfo, BaseSceneDeferredPass.LightingDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			uint32_t write_size = 9;
			std::vector<VkWriteDescriptorSet> descriptorWrites{};
			descriptorWrites.resize(write_size);

			// 绑定 UnifromBuffer
			VkDescriptorBufferInfo viewBufferInfo{};
			viewBufferInfo.buffer = ViewUniformBuffers[i];
			viewBufferInfo.offset = 0;
			viewBufferInfo.range = sizeof(FUniformBufferView);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = BaseSceneDeferredPass.LightingDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &viewBufferInfo;

			VkDescriptorImageInfo cubemapImageInfo{};
			cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			cubemapImageInfo.imageView = CubemapImageView;
			cubemapImageInfo.sampler = CubemapSampler;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = BaseSceneDeferredPass.LightingDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &cubemapImageInfo;

			VkDescriptorImageInfo shadowmapImageInfo{};
			shadowmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			shadowmapImageInfo.imageView = ShadowmapPass.ImageView;
			shadowmapImageInfo.sampler = ShadowmapPass.Sampler;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = BaseSceneDeferredPass.LightingDescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &shadowmapImageInfo;

			std::vector<VkImageView> ImageViews;
			ImageViews.resize(6);
			std::vector<VkSampler> Samplers;
			Samplers.resize(6);
			ImageViews[0] = GBuffer.DepthStencilImageView;
			ImageViews[1] = GBuffer.SceneColorImageView;
			ImageViews[2] = GBuffer.GBufferAImageView;
			ImageViews[3] = GBuffer.GBufferBImageView;
			ImageViews[4] = GBuffer.GBufferCImageView;
			ImageViews[5] = GBuffer.GBufferDImageView;
			Samplers[0] = GBuffer.DepthStencilSampler;
			Samplers[1] = GBuffer.SceneColorSampler;
			Samplers[2] = GBuffer.GBufferASampler;
			Samplers[3] = GBuffer.GBufferBSampler;
			Samplers[4] = GBuffer.GBufferCSampler;
			Samplers[5] = GBuffer.GBufferDSampler;
			// 绑定 Textures
			// descriptorWrites会引用每一个创建的VkDescriptorImageInfo，所以需要用一个数组把它们存储起来
			std::vector<VkDescriptorImageInfo> imageInfos;
			imageInfos.resize(ImageViews.size());
			for (size_t j = 0; j < ImageViews.size(); j++)
			{
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = (j == 0) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = ImageViews[j];
				imageInfo.sampler = Samplers[j];
				imageInfos[j] = imageInfo;

				descriptorWrites[j + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j + 3].dstSet = BaseSceneDeferredPass.LightingDescriptorSets[i];
				descriptorWrites[j + 3].dstBinding = static_cast<uint32_t>(j + 3);
				descriptorWrites[j + 3].dstArrayElement = 0;
				descriptorWrites[j + 3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[j + 3].descriptorCount = 1;
				descriptorWrites[j + 3].pImageInfo = &imageInfos[j];
			}

			vkUpdateDescriptorSets(Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		CreatePipelineLayout(BaseSceneDeferredPass.LightingPipelineLayout, BaseSceneDeferredPass.LightingDescriptorSetLayout);
		BaseSceneDeferredPass.LightingPipelines.resize(GlobalConstants.SpecConstantsCount);
		CreateGraphicsPipelinesDeferred(
			BaseSceneDeferredPass.LightingPipelines,
			BaseSceneDeferredPass.LightingPipelineLayout,
			MainRenderPass,
			GlobalConstants.SpecConstantsCount, ScreenRect, DeferredLighting,
			"Resources/Shaders/draw_with_deferred_bg_vert.spv",
			"Resources/Shaders/draw_with_deferred_lighting_frag.spv");

		CreateBaseSceneResources();
	}

	void CreateBaseSceneResources()
	{
		FRenderObject terrain;
		std::string terrain_obj = "Resources/Models/terrain.obj";
		std::vector<std::string> terrain_imgs = {
				"Resources/Textures/terrain_bc.png",		// BaseColor
				"Resources/Textures/default_black.png",		// Metallic
				"Resources/Textures/terrain_r.png",			// Roughness
				"Resources/Textures/terrain_n.png",			// Normal
				"Resources/Textures/terrain_ao.png",		// AmbientOcclution
				"Resources/Textures/default_black.png",		// Emissive
				"Resources/Textures/default_white.png" };	// Mask

		FRenderObject rock01;
		float rock01_zone = 1.0f;
		std::string rock01_obj = "Resources/Models/rock_01.obj";
		std::vector<std::string> rock01_imgs = {
				"Resources/Textures/rock_01_bc.png",		// BaseColor
				"Resources/Textures/default_black.png",		// Metallic
				"Resources/Textures/rock_01_r.png",			// Roughness
				"Resources/Textures/rock_01_n.png",			// Normal
				"Resources/Textures/default_white.png",		// AmbientOcclution
				"Resources/Textures/default_black.png",		// Emissive
				"Resources/Textures/default_white.png" };	// Mask

		FRenderInstancedObject rock02;
		std::string rock02_obj = "Resources/Models/rock_02.obj";
		std::vector<std::string> rock02_imgs = {
				"Resources/Textures/rock_02_bc.png",			// BaseColor
				"Resources/Textures/default_black.png",		// Metallic
				"Resources/Textures/rock_02_r.png",			// Roughness
				"Resources/Textures/rock_02_n.png",			// Normal
				"Resources/Textures/default_white.png",		// AmbientOcclution
				"Resources/Textures/default_black.png",		// Emissive
				"Resources/Textures/default_white.png" };	// Mask
		std::vector<FInstanceData> rock_InstanceData;
		uint32_t rock_InstanceCount = 64;
		rock_InstanceData.resize(rock_InstanceCount);
		for (uint32_t i = 0; i < rock_InstanceCount; i++) {
			float radians = (((float)RandRange(0, 3600) / 10.0f));
			float distance = (((float)RandRange(0, 500) / 100.0f)) + rock01_zone;
			float X = sin(glm::radians(radians)) * distance;
			float Y = cos(glm::radians(radians)) * distance;
			float Z = 0.0;
			rock_InstanceData[i].InstancePosition = glm::vec3(X, Y, Z);
			// Y(Pitch), Z(Yaw), X(Roll)
			float Yaw = float(M_PI) * RandRange(0, 99) / 100.0f;
			rock_InstanceData[i].InstanceRotation = glm::vec3(0.0, Yaw, 0.0);
			rock_InstanceData[i].InstancePScale = RandRange(2, 5) / 10.0f;
			rock_InstanceData[i].InstanceTexIndex = RandRange(0, 255);
		}

		FRenderInstancedObject grass01;
		std::string grass01_obj = "Resources/Models/grass_01.obj";
		FRenderInstancedObject grass02;
		std::string grass02_obj = "Resources/Models/grass_02.obj";
		std::vector<std::string> grass_imgs = {
				"Resources/Textures/grass_bc.png",			// BaseColor
				"Resources/Textures/default_black.png",		// Metallic
				"Resources/Textures/grass_r.png",			// Roughness
				"Resources/Textures/grass_n.png",			// Normal
				"Resources/Textures/grass_ao.png",			// AmbientOcclution
				"Resources/Textures/grass_ev.png",			// Emissive
				"Resources/Textures/grass_ms.png" };		// Mask

		std::vector<FInstanceData> grass01_InstanceData;
		uint32_t grass01_InstanceCount = INSTANCE_COUNT;
		grass01_InstanceData.resize(grass01_InstanceCount);
		for (uint32_t i = 0; i < grass01_InstanceCount; i++) {
			float radians = (((float)RandRange(0, 3600) / 10.0f));
			float distance = (((float)RandRange(0, 800) / 100.0f)) + rock01_zone * 2.0f;
			float X = sin(glm::radians(radians)) * distance;
			float Y = cos(glm::radians(radians)) * distance;
			float Z = 0.0;
			grass01_InstanceData[i].InstancePosition = glm::vec3(X, Y, Z);
			// Y(Pitch), Z(Yaw), X(Roll)
			float Yaw = float(M_PI) * RandRange(0, 99) / 100.0f;
			grass01_InstanceData[i].InstanceRotation = glm::vec3(0.0, Yaw, 0.0);
			grass01_InstanceData[i].InstancePScale = RandRange(1, 5) / 10.0f;
			grass01_InstanceData[i].InstanceTexIndex = RandRange(0, 255);
		}

		std::vector<FInstanceData> grass_02_InstanceData;
		uint32_t grass02_InstanceCount = INSTANCE_COUNT;
		grass_02_InstanceData.resize(grass02_InstanceCount);
		for (uint32_t i = 0; i < grass02_InstanceCount; i++) {
			float radians = (((float)RandRange(0, 3600) / 10.0f));
			float distance = (((float)RandRange(0, 900) / 100.0f)) + rock01_zone;
			float X = sin(glm::radians(radians)) * distance;
			float Y = cos(glm::radians(radians)) * distance;
			float Z = 0.0;
			grass_02_InstanceData[i].InstancePosition = glm::vec3(X, Y, Z);
			// Y(Pitch), Z(Yaw), X(Roll)
			float Yaw = float(M_PI) * RandRange(0, 99) / 100.0f;
			grass_02_InstanceData[i].InstanceRotation = glm::vec3(0.0, Yaw, 0.0);
			grass_02_InstanceData[i].InstancePScale = RandRange(1, 5) / 10.0f;
			grass_02_InstanceData[i].InstanceTexIndex = RandRange(0, 255);
		}

#if !ENABLE_DEFEERED_RENDERING
		CreateRenderObject<FRenderObject>(terrain, terrain_obj, terrain_imgs, BaseScenePass.DescriptorSetLayout);
		BaseScenePass.RenderObjects.push_back(terrain);

		CreateRenderObject<FRenderObject>(rock01, rock01_obj, rock01_imgs, BaseScenePass.DescriptorSetLayout);
		BaseScenePass.RenderObjects.push_back(rock01);

		CreateRenderObject<FRenderInstancedObject>(rock02, rock02_obj, rock02_imgs, BaseScenePass.DescriptorSetLayout);
		CreateInstancedBuffer<FRenderInstancedObject>(rock02, rock_InstanceData);
		BaseScenePass.RenderInstancedObjects.push_back(rock02);

		CreateRenderObject<FRenderInstancedObject>(grass01, grass01_obj, grass_imgs, BaseScenePass.DescriptorSetLayout);
		CreateInstancedBuffer<FRenderInstancedObject>(grass01, grass01_InstanceData);
		BaseScenePass.RenderInstancedObjects.push_back(grass01);

		CreateRenderObject<FRenderInstancedObject>(grass02, grass02_obj, grass_imgs, BaseScenePass.DescriptorSetLayout);
		CreateInstancedBuffer<FRenderInstancedObject>(grass02, grass_02_InstanceData);
		BaseScenePass.RenderInstancedObjects.push_back(grass02);
#else
		CreateRenderObject<FRenderObject>(terrain, terrain_obj, terrain_imgs, BaseSceneDeferredPass.SceneDescriptorSetLayout);
		BaseSceneDeferredPass.RenderObjects.push_back(terrain);

		CreateRenderObject<FRenderObject>(rock01, rock01_obj, rock01_imgs, BaseSceneDeferredPass.SceneDescriptorSetLayout);
		BaseSceneDeferredPass.RenderObjects.push_back(rock01);

		CreateRenderObject<FRenderInstancedObject>(rock02, rock02_obj, rock02_imgs, BaseSceneDeferredPass.SceneDescriptorSetLayout);
		CreateInstancedBuffer<FRenderInstancedObject>(rock02, rock_InstanceData);
		BaseSceneDeferredPass.RenderInstancedObjects.push_back(rock02);

		CreateRenderObject<FRenderInstancedObject>(grass01, grass01_obj, grass_imgs, BaseSceneDeferredPass.SceneDescriptorSetLayout);
		CreateInstancedBuffer<FRenderInstancedObject>(grass01, grass01_InstanceData);
		BaseSceneDeferredPass.RenderInstancedObjects.push_back(grass01);

		CreateRenderObject<FRenderInstancedObject>(grass02, grass02_obj, grass_imgs, BaseSceneDeferredPass.SceneDescriptorSetLayout);
		CreateInstancedBuffer<FRenderInstancedObject>(grass02, grass_02_InstanceData);
		BaseSceneDeferredPass.RenderInstancedObjects.push_back(grass02);
#endif
	}

	void CreateBackgroundPass()
	{
		// 创建背景贴图
		CreateImageContext(
			BackgroundPass.Image,
			BackgroundPass.Memory,
			BackgroundPass.ImageView,
			BackgroundPass.Sampler,
			"Resources/Textures/background.png");
		CreateDescriptorSetLayout(BackgroundPass.DescriptorSetLayout);
		CreateDescriptorPool(BackgroundPass.DescriptorPool);
		CreateDescriptorSets(
			BackgroundPass.DescriptorSets,
			BackgroundPass.DescriptorPool,
			BackgroundPass.DescriptorSetLayout,
			BackgroundPass.ImageView,
			BackgroundPass.Sampler);
		BackgroundPass.Pipelines.resize(1);
		CreatePipelineLayout(BackgroundPass.PipelineLayout, BackgroundPass.DescriptorSetLayout);

		CreateGraphicsPipelines(
			BackgroundPass.Pipelines,
			BackgroundPass.PipelineLayout, 
			MainRenderPass,
			1,
			"Resources/Shaders/draw_with_deferred_bg_vert.spv",
			"Resources/Shaders/draw_with_deferred_bg_frag.spv",
			false/*bDepthTest*/, false/*bCullBack*/, false/*bInstanced*/);
	}

	void CreateSkydomePass()
	{
		// 创建环境反射纹理资源
		CreateCubemapResources();

		// HDRI texture map
		// @see https://polyhaven.com/a/preller_drive
		CreateImageContext(
			SkydomePass.Image,
			SkydomePass.Memory,
			SkydomePass.ImageView,
			SkydomePass.Sampler,
			"Resources/Textures/skydome.png");
		CreateDescriptorSetLayout(SkydomePass.DescriptorSetLayout);
		CreateDescriptorPool(SkydomePass.DescriptorPool);
		CreateDescriptorSets(
			SkydomePass.DescriptorSets,
			SkydomePass.DescriptorPool,
			SkydomePass.DescriptorSetLayout,
			SkydomePass.ImageView,
			SkydomePass.Sampler);
		SkydomePass.Pipelines.resize(1);
		CreatePipelineLayout(SkydomePass.PipelineLayout, SkydomePass.DescriptorSetLayout);
		CreateGraphicsPipelines(
			SkydomePass.Pipelines,
			SkydomePass.PipelineLayout,
			MainRenderPass,
			1,
			"Resources/Shaders/draw_with_deferred_sky_vert.spv",
			"Resources/Shaders/draw_with_deferred_sky_frag.spv",
			true/*bDepthTest*/, true/*bCullBack*/, false/*bInstanced*/);

		std::string skydome_obj = "Resources/Models/skydome.obj";
		CreateMesh(SkydomePass.SkydomeMesh.Vertices, SkydomePass.SkydomeMesh.Indices, skydome_obj);
		CreateVertexBuffer(
			SkydomePass.SkydomeMesh.VertexBuffer,
			SkydomePass.SkydomeMesh.VertexBufferMemory,
			SkydomePass.SkydomeMesh.Vertices);
		CreateIndexBuffer(
			SkydomePass.SkydomeMesh.IndexBuffer,
			SkydomePass.SkydomeMesh.IndexBufferMemory,
			SkydomePass.SkydomeMesh.Indices);
	}

	/** 创建指令缓存，多个CPU Core可以并行的往CommandBuffer中发送指令，可以充分利用CPU的多核性能*/
	void CreateCommandBuffer()
	{
		CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)CommandBuffers.size();

		if (vkAllocateCommandBuffers(Device, &allocInfo, CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	/** 把需要执行的指令写入指令缓存，对应每一个SwapChain的图像*/
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		auto BeginTransitionImageLayoutRT = [commandBuffer](VkImage & image, const VkImageAspectFlagBits aspectMask, const VkImageLayout oldLayout, const VkImageLayout newLayout)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = aspectMask;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		};

		auto EndTransitionImageLayoutRT = [commandBuffer](VkImage& image, const VkImageAspectFlagBits aspectMask, const VkImageLayout oldLayout, const VkImageLayout newLayout)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = aspectMask;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		};

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		// 开始记录指令
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		// 【阴影】渲染阴影
		{
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = ShadowmapPass.RenderPass;
			renderPassInfo.framebuffer = ShadowmapPass.FrameBuffer;
			renderPassInfo.renderArea.extent.width = ShadowmapPass.Width;
			renderPassInfo.renderArea.extent.height = ShadowmapPass.Height;
			std::array<VkClearValue, 1> clearValues{};
			clearValues[0].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = clearValues.data();

			// 【阴影】开始 RenderPass
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// 【阴影】视口信息
			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)ShadowmapPass.Width;
			viewport.height = (float)ShadowmapPass.Height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			// 【阴影】视口剪切信息
			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent.width = ShadowmapPass.Width;
			scissor.extent.height = ShadowmapPass.Height;

			// 【阴影】设置渲染视口
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			// 【阴影】设置视口剪切，是否可以通过这个函数来实现 Tiled-Based Rendering ？
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artifacts
			// Depth bias (and slope) are used to avoid shadowing artifacts
			// Constant depth bias factor (always applied)
			float depthBiasConstant = 1.25f;
			// Slope depth bias factor, applied depending on polygon's slope
			float depthBiasSlope = 7.5; // change from 1.75f to fix PCF artifact
			vkCmdSetDepthBias(
				commandBuffer,
				depthBiasConstant,
				0.0f,
				depthBiasSlope);

			// 【阴影】渲染场景
			for (size_t i = 0; i < ShadowmapPass.RenderObjects.size(); i++)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowmapPass.Pipeline);
				FRenderObject* renderObject = ShadowmapPass.RenderObjects[i];
				VkBuffer objectVertexBuffers[] = { renderObject->MeshData.VertexBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, renderObject->MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					ShadowmapPass.PipelineLayout, 0, 1, &ShadowmapPass.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, ShadowmapPass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(renderObject->MeshData.Indices.size()), 1, 0, 0, 0);
			}
			// 【阴影】渲染 Instanced 场景
			for (size_t i = 0; i < ShadowmapPass.RenderInstancedObjects.size(); i++)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowmapPass.PipelineInstanced);
				FRenderInstancedObject* renderInstancedObject = ShadowmapPass.RenderInstancedObjects[i];
				VkBuffer objectVertexBuffers[] = { renderInstancedObject->MeshData.VertexBuffer };
				VkBuffer objectInstanceBuffers[] = { renderInstancedObject->MeshData.InstancedBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, objectInstanceBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, renderInstancedObject->MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					ShadowmapPass.PipelineLayout, 0, 1, &ShadowmapPass.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, ShadowmapPass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(renderInstancedObject->MeshData.Indices.size()), renderInstancedObject->InstanceCount, 0, 0, 0);
			}
			// 【阴影】渲染Indirect场景
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowmapPass.Pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				ShadowmapPass.PipelineLayout, 0, 1, &ShadowmapPass.DescriptorSets[CurrentFrame], 0, nullptr);
			vkCmdPushConstants(commandBuffer, ShadowmapPass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
			for (size_t i = 0; i < ShadowmapPass.RenderIndirectObject.size(); i++)
			{
				FRenderIndirectObject* RenderIndirectObject = ShadowmapPass.RenderIndirectObject[i];
				VkBuffer objectVertexBuffers[] = { RenderIndirectObject->MeshData.VertexBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, RenderIndirectObject->MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				uint32_t indirectDrawCount = static_cast<uint32_t>(RenderIndirectObject->IndirectCommands.size());
				if (IsSupportMultiDrawIndirect(PhysicalDevice))
				{
					vkCmdDrawIndexedIndirect(
						commandBuffer, /*commandBuffer*/
						RenderIndirectObject->IndirectCommandsBuffer, /*buffer*/
						0, /*offset*/
						indirectDrawCount, /*drawCount*/
						sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
				}
				else
				{
					// If multi draw is not available, we must issue separate draw commands
					for (auto j = 0; j < RenderIndirectObject->IndirectCommands.size(); j++)
					{
						vkCmdDrawIndexedIndirect(
							commandBuffer, /*commandBuffer*/
							RenderIndirectObject->IndirectCommandsBuffer, /*buffer*/
							j * sizeof(VkDrawIndexedIndirectCommand), /*offset*/
							1, /*drawCount*/
							sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
					}
				}
			}
			// 【阴影】渲染Indirect Instanced 场景
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowmapPass.PipelineInstanced);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				ShadowmapPass.PipelineLayout, 0, 1, &ShadowmapPass.DescriptorSets[CurrentFrame], 0, nullptr);
			vkCmdPushConstants(commandBuffer, ShadowmapPass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
			for (size_t i = 0; i < ShadowmapPass.RenderIndirectInstancedObject.size(); i++)
			{
				FRenderIndirectInstancedObject* RenderIndirectInstancedObject = ShadowmapPass.RenderIndirectInstancedObject[i];
				VkBuffer objectVertexBuffers[] = { RenderIndirectInstancedObject->MeshData.VertexBuffer };
				VkBuffer objectInstanceBuffers[] = { RenderIndirectInstancedObject->MeshData.InstancedBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, objectInstanceBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, RenderIndirectInstancedObject->MeshData.IndexBuffer, 0 , VK_INDEX_TYPE_UINT32);
				uint32_t indirectDrawCount = static_cast<uint32_t>(RenderIndirectInstancedObject->IndirectCommands.size());
				if (IsSupportMultiDrawIndirect(PhysicalDevice))
				{
					vkCmdDrawIndexedIndirect(
						commandBuffer, /*commandBuffer*/
						RenderIndirectInstancedObject->IndirectCommandsBuffer, /*buffer*/
						0, /*offset*/
						indirectDrawCount, /*drawCount*/
						sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
				}
				else
				{
					// If multi draw is not available, we must issue separate draw commands
					for (auto j = 0; j < RenderIndirectInstancedObject->IndirectCommands.size(); j++)
					{
						vkCmdDrawIndexedIndirect(
							commandBuffer, /*commandBuffer*/
							RenderIndirectInstancedObject->IndirectCommandsBuffer, /*buffer*/
							j * sizeof(VkDrawIndexedIndirectCommand), /*offset*/
							1, /*drawCount*/
							sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
					}
				}
			}

			// 【阴影】结束 RenderPass
			vkCmdEndRenderPass(commandBuffer);
		}

		// 渲染视口信息
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)SwapChainExtent.width;
		viewport.height = (float)SwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// 视口剪切信息
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = SwapChainExtent;

#if ENABLE_DEFEERED_RENDERING
		// 【延迟渲染】渲染场景
		{
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = BaseSceneDeferredPass.SceneRenderPass;
			renderPassInfo.framebuffer = BaseSceneDeferredPass.SceneFrameBuffer;
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = SwapChainExtent;

			std::array<VkClearValue, 6> clearValues{};
			clearValues[0].depthStencil = { 1.0f, 0 };
			clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[2].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
			clearValues[3].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[4].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[5].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			// 【延迟渲染】开始 RenderPass
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// 【延迟渲染】渲染场景
			for (size_t i = 0; i < BaseSceneDeferredPass.RenderObjects.size(); i++)
			{
				uint32_t SpecConstants = GlobalConstants.SpecConstants;
				VkPipeline baseScenePassPipeline = BaseSceneDeferredPass.ScenePipelines[SpecConstants];
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, baseScenePassPipeline);
				FRenderObject renderObject = BaseSceneDeferredPass.RenderObjects[i];
				VkBuffer objectVertexBuffers[] = { renderObject.MeshData.VertexBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, renderObject.MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					BaseSceneDeferredPass.ScenePipelineLayout, 0, 1,
					&renderObject.MateData.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, BaseSceneDeferredPass.ScenePipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(renderObject.MeshData.Indices.size()), 1, 0, 0, 0);
			}
			// 【延迟渲染】渲染 Instanced 场景
			for (size_t i = 0; i < BaseSceneDeferredPass.RenderInstancedObjects.size(); i++)
			{
				uint32_t SpecConstants = GlobalConstants.SpecConstants;
				VkPipeline BaseScenePassPipelineInstanced = BaseSceneDeferredPass.ScenePipelinesInstanced[SpecConstants];
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BaseScenePassPipelineInstanced);
				FRenderInstancedObject renderInstancedObject = BaseSceneDeferredPass.RenderInstancedObjects[i];
				VkBuffer objectVertexBuffers[] = { renderInstancedObject.MeshData.VertexBuffer };
				VkBuffer objectInstanceBuffers[] = { renderInstancedObject.MeshData.InstancedBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, objectInstanceBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, renderInstancedObject.MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					BaseSceneDeferredPass.ScenePipelineLayout, 0, 1,
					&renderInstancedObject.MateData.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, BaseSceneDeferredPass.ScenePipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(renderInstancedObject.MeshData.Indices.size()), renderInstancedObject.InstanceCount, 0, 0, 0);
			}

			// 【延迟渲染】结束RenderPass
			vkCmdEndRenderPass(commandBuffer);
		}

		VkImageCopy copyRegion = {};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };
		copyRegion.extent.width = static_cast<uint32_t>(SwapChainExtent.width);
		copyRegion.extent.height = static_cast<uint32_t>(SwapChainExtent.height);
		copyRegion.extent.depth = 1;

		BeginTransitionImageLayoutRT(GBuffer.DepthStencilImage, 
			VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		BeginTransitionImageLayoutRT(DepthImage, 
			VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdCopyImage(commandBuffer, GBuffer.DepthStencilImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			DepthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		EndTransitionImageLayoutRT(GBuffer.DepthStencilImage, 
			VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
		EndTransitionImageLayoutRT(DepthImage, 
			VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
#endif

		// 【主场景】渲染场景
		{
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = MainRenderPass;
			renderPassInfo.framebuffer = SwapChainFramebuffers[imageIndex];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = SwapChainExtent;

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			// 【主场景】开始 RenderPass
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// 【主场景】设置渲染视口
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			// 【主场景】设置视口剪切，是否可以通过这个函数来实现 Tiled-Based Rendering ？
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// 【主场景】渲染背景面片
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BackgroundPass.Pipelines[0]);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BackgroundPass.PipelineLayout, 0, 1, &BackgroundPass.DescriptorSets[CurrentFrame], 0, nullptr);
			vkCmdPushConstants(commandBuffer, BackgroundPass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
			vkCmdDraw(commandBuffer, 6, 1, 0, 0);

#if ENABLE_DEFEERED_RENDERING
			// 【主场景】渲染延迟渲染灯光
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BaseSceneDeferredPass.LightingPipelines[GlobalConstants.SpecConstants]);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BaseSceneDeferredPass.LightingPipelineLayout, 0, 1, &BaseSceneDeferredPass.LightingDescriptorSets[CurrentFrame], 0, nullptr);
			vkCmdPushConstants(commandBuffer, BaseSceneDeferredPass.LightingPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
			vkCmdDraw(commandBuffer, 6, 1, 0, 0);
#endif
			// 【主场景】渲染场景
			for (size_t i = 0; i < BaseScenePass.RenderObjects.size(); i++)
			{
				VkPipeline baseScenePassPipeline = BaseScenePass.Pipelines[GlobalConstants.SpecConstants];
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, baseScenePassPipeline);
				FRenderObject renderObject = BaseScenePass.RenderObjects[i];
				VkBuffer objectVertexBuffers[] = { renderObject.MeshData.VertexBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, renderObject.MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					BaseScenePass.PipelineLayout, 0, 1,
					&renderObject.MateData.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, BaseScenePass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(renderObject.MeshData.Indices.size()), 1, 0, 0, 0);
			}
			// 【主场景】渲染 Instanced 场景
			for (size_t i = 0; i < BaseScenePass.RenderInstancedObjects.size(); i++)
			{
				VkPipeline BaseScenePassPipelineInstanced = BaseScenePass.PipelinesInstanced[GlobalConstants.SpecConstants];
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BaseScenePassPipelineInstanced);
				FRenderInstancedObject renderInstancedObject = BaseScenePass.RenderInstancedObjects[i];
				VkBuffer objectVertexBuffers[] = { renderInstancedObject.MeshData.VertexBuffer };
				VkBuffer objectInstanceBuffers[] = { renderInstancedObject.MeshData.InstancedBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, objectInstanceBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, renderInstancedObject.MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					BaseScenePass.PipelineLayout, 0, 1,
					&renderInstancedObject.MateData.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, BaseScenePass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(renderInstancedObject.MeshData.Indices.size()), renderInstancedObject.InstanceCount, 0, 0, 0);
			}
			// 【主场景】渲染 Indirect 场景
			for (size_t i = 0; i < BaseSceneIndirectPass.RenderIndirectObject.size(); i++)
			{
				VkPipeline indirectScenePassPipeline = BaseSceneIndirectPass.Pipelines[GlobalConstants.SpecConstants];
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, indirectScenePassPipeline);
				FRenderIndirectObject RenderIndirectObject = BaseSceneIndirectPass.RenderIndirectObject[i];
				VkBuffer objectVertexBuffers[] = { RenderIndirectObject.MeshData.VertexBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, RenderIndirectObject.MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					BaseSceneIndirectPass.PipelineLayout, 0, 1,
					&RenderIndirectObject.MateData.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, BaseSceneIndirectPass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				uint32_t indirectDrawCount = static_cast<uint32_t>(RenderIndirectObject.IndirectCommands.size());
				if (IsSupportMultiDrawIndirect(PhysicalDevice))
				{
					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//01 void FakeDrawIndexedIndirect(VkCommandBuffer commandBuffer, void* buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
					//02 {
					//03 	char* memory = (char*)buffer + offset;
					//04 
					//05 	for (uint32_t i = 0; i < drawCount; i++)
					//06 	{
					//07 		VkDrawIndexedIndirectCommand* command = (VkDrawIndexedIndirectCommand*)(memory + (i * stride));
					//08 
					//09 		vkCmdDrawIndexed(commandBuffer,
					//10 			command->indexCount,
					//11 			command->instanceCount,
					//12 			command->firstIndex,
					//13 			command->vertexOffset,
					//14 			command->firstInstance);
					//15 	}
					//16 }
					vkCmdDrawIndexedIndirect(
						commandBuffer, /*commandBuffer*/
						RenderIndirectObject.IndirectCommandsBuffer, /*buffer*/
						0, /*offset*/
						indirectDrawCount, /*drawCount*/
						sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
				}
				else
				{
					// If multi draw is not available, we must issue separate draw commands
					for (auto j = 0; j < RenderIndirectObject.IndirectCommands.size(); j++)
					{
						vkCmdDrawIndexedIndirect(
							commandBuffer, /*commandBuffer*/
							RenderIndirectObject.IndirectCommandsBuffer, /*buffer*/
							j * sizeof(VkDrawIndexedIndirectCommand), /*offset*/
							1, /*drawCount*/
							sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
					}
				}
			}
			// 【主场景】渲染 Indirect Instanced 场景
			for (size_t i = 0; i < BaseSceneIndirectPass.RenderIndirectInstancedObject.size(); i++)
			{
				VkPipeline indirectScenePassPipelineInstanced = BaseSceneIndirectPass.PipelinesInstanced[GlobalConstants.SpecConstants];
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, indirectScenePassPipelineInstanced);
				FRenderIndirectInstancedObject RenderIndirectInstancedObject = BaseSceneIndirectPass.RenderIndirectInstancedObject[i];
				VkBuffer objectVertexBuffers[] = { RenderIndirectInstancedObject.MeshData.VertexBuffer };
				VkBuffer objectInstanceBuffers[] = { RenderIndirectInstancedObject.MeshData.InstancedBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				// Binding point 0 : Mesh vertex buffer
				vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, objectVertexBuffers, objectOffsets);
				// Binding point 1 : Instance data buffer
				vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, objectInstanceBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, RenderIndirectInstancedObject.MeshData.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					BaseSceneIndirectPass.PipelineLayout, 0, 1,
					&RenderIndirectInstancedObject.MateData.DescriptorSets[CurrentFrame], 0, nullptr);
				vkCmdPushConstants(commandBuffer, BaseSceneIndirectPass.PipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FGlobalConstants), &GlobalConstants);
				uint32_t indirectDrawCount = static_cast<uint32_t>(RenderIndirectInstancedObject.IndirectCommands.size());
				if (IsSupportMultiDrawIndirect(PhysicalDevice))
				{
					vkCmdDrawIndexedIndirect(
						commandBuffer, /*commandBuffer*/
						RenderIndirectInstancedObject.IndirectCommandsBuffer, /*buffer*/
						0, /*offset*/
						indirectDrawCount, /*drawCount*/
						sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
				}
				else
				{
					// If multi draw is not available, we must issue separate draw commands
					for (auto j = 0; j < RenderIndirectInstancedObject.IndirectCommands.size(); j++)
					{
						vkCmdDrawIndexedIndirect(
							commandBuffer, /*commandBuffer*/
							RenderIndirectInstancedObject.IndirectCommandsBuffer, /*buffer*/
							j * sizeof(VkDrawIndexedIndirectCommand), /*offset*/
							1, /*drawCount*/
							sizeof(VkDrawIndexedIndirectCommand) /*stride*/);
					}
				}
			}

			// 【主场景】渲染天空球
			if (SCENE_SHOW_SKYDOME && GlobalConstants.SpecConstants == 0 /* Don't render sky on debug mode*/)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkydomePass.Pipelines[0]);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkydomePass.PipelineLayout, 0, 1, &SkydomePass.DescriptorSets[CurrentFrame], 0, nullptr);
				VkBuffer objectVertexBuffers[] = { SkydomePass.SkydomeMesh.VertexBuffer };
				VkDeviceSize objectOffsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, objectVertexBuffers, objectOffsets);
				vkCmdBindIndexBuffer(commandBuffer, SkydomePass.SkydomeMesh.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(SkydomePass.SkydomeMesh.Indices.size()), 1, 0, 0, 0);
			}

			// 【主场景】结束RenderPass
			vkCmdEndRenderPass(commandBuffer);
		}

		// 结束记录指令
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	/** 创建同步物体，同步显示当前渲染*/
	void CreateSyncObjects()
	{
		ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(Device, &fenceInfo, nullptr, &InFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to Create synchronization objects for a frame!");
			}
		}
	}

	/** 删除函数 InitVulkan 中创建的元素*/
	void DestroyVulkan()
	{
		// CommandBuffer 不需要释放
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(Device, RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(Device, ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(Device, InFlightFences[i], nullptr);
		}

		vkDestroyRenderPass(Device, MainRenderPass, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(Device, BaseUniformBuffers[i], nullptr);
			vkFreeMemory(Device, BaseUniformBuffersMemory[i], nullptr);
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(Device, ViewUniformBuffers[i], nullptr);
			vkFreeMemory(Device, ViewUniformBuffersMemory[i], nullptr);
		}

		vkDestroyImageView(Device, CubemapImageView, nullptr);
		vkDestroySampler(Device, CubemapSampler, nullptr);
		vkDestroyImage(Device, CubemapImage, nullptr);
		vkFreeMemory(Device, CubemapImageMemory, nullptr);

		// 清理 ShadowmapPass
		vkDestroyRenderPass(Device, ShadowmapPass.RenderPass, nullptr);
		vkDestroyFramebuffer(Device, ShadowmapPass.FrameBuffer, nullptr);
		vkDestroyDescriptorPool(Device, ShadowmapPass.DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(Device, ShadowmapPass.DescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, ShadowmapPass.PipelineLayout, nullptr);
		vkDestroyPipeline(Device, ShadowmapPass.Pipeline, nullptr);
		vkDestroyPipeline(Device, ShadowmapPass.PipelineInstanced, nullptr);
		vkDestroyImageView(Device, ShadowmapPass.ImageView, nullptr);
		vkDestroySampler(Device, ShadowmapPass.Sampler, nullptr);
		vkDestroyImage(Device, ShadowmapPass.Image, nullptr);
		vkFreeMemory(Device, ShadowmapPass.Memory, nullptr);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(Device, ShadowmapPass.UniformBuffers[i], nullptr);
			vkFreeMemory(Device, ShadowmapPass.UniformBuffersMemory[i], nullptr);
		}

		// 清理 SkydomePass
		vkDestroyDescriptorSetLayout(Device, SkydomePass.DescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, SkydomePass.PipelineLayout, nullptr);
		vkDestroyPipeline(Device, SkydomePass.Pipelines[0], nullptr);
		vkDestroyDescriptorPool(Device, SkydomePass.DescriptorPool, nullptr);
		vkDestroyImageView(Device, SkydomePass.ImageView, nullptr);
		vkDestroySampler(Device, SkydomePass.Sampler, nullptr);
		vkDestroyImage(Device, SkydomePass.Image, nullptr);
		vkFreeMemory(Device, SkydomePass.Memory, nullptr);
		vkDestroyBuffer(Device, SkydomePass.SkydomeMesh.VertexBuffer, nullptr);
		vkFreeMemory(Device, SkydomePass.SkydomeMesh.VertexBufferMemory, nullptr);
		vkDestroyBuffer(Device, SkydomePass.SkydomeMesh.IndexBuffer, nullptr);
		vkFreeMemory(Device, SkydomePass.SkydomeMesh.IndexBufferMemory, nullptr);

		// 清理 BackgroundPass
		vkDestroyDescriptorSetLayout(Device, BackgroundPass.DescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, BackgroundPass.PipelineLayout, nullptr);
		vkDestroyPipeline(Device, BackgroundPass.Pipelines[0], nullptr);
		vkDestroyDescriptorPool(Device, BackgroundPass.DescriptorPool, nullptr);
		vkDestroyImageView(Device, BackgroundPass.ImageView, nullptr);
		vkDestroySampler(Device, BackgroundPass.Sampler, nullptr);
		vkDestroyImage(Device, BackgroundPass.Image, nullptr);
		vkFreeMemory(Device, BackgroundPass.Memory, nullptr);

		// 清理 BaseScenePass
		vkDestroyDescriptorSetLayout(Device, BaseScenePass.DescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, BaseScenePass.PipelineLayout, nullptr);
		for (uint32_t i = 0; i < GlobalConstants.SpecConstantsCount; i++)
		{
			vkDestroyPipeline(Device, BaseScenePass.Pipelines[i], nullptr);
			vkDestroyPipeline(Device, BaseScenePass.PipelinesInstanced[i], nullptr);
		}
		for (size_t i = 0; i < BaseScenePass.RenderObjects.size(); i++)
		{
			FRenderObject& renderObject = BaseScenePass.RenderObjects[i];

			vkDestroyDescriptorPool(Device, renderObject.MateData.DescriptorPool, nullptr);

			for (size_t j = 0; j < renderObject.MateData.TextureImages.size(); j++)
			{
				vkDestroyImageView(Device, renderObject.MateData.TextureImageViews[j], nullptr);
				vkDestroySampler(Device, renderObject.MateData.TextureSamplers[j], nullptr);
				vkDestroyImage(Device, renderObject.MateData.TextureImages[j], nullptr);
				vkFreeMemory(Device, renderObject.MateData.TextureImageMemorys[j], nullptr);
			}

			vkDestroyBuffer(Device, renderObject.MeshData.VertexBuffer, nullptr);
			vkFreeMemory(Device, renderObject.MeshData.VertexBufferMemory, nullptr);
			vkDestroyBuffer(Device, renderObject.MeshData.IndexBuffer, nullptr);
			vkFreeMemory(Device, renderObject.MeshData.IndexBufferMemory, nullptr);
		}
		for (size_t i = 0; i < BaseScenePass.RenderInstancedObjects.size(); i++)
		{
			FRenderInstancedObject& renderInstancedObject = BaseScenePass.RenderInstancedObjects[i];

			vkDestroyDescriptorPool(Device, renderInstancedObject.MateData.DescriptorPool, nullptr);

			for (size_t j = 0; j < renderInstancedObject.MateData.TextureImages.size(); j++)
			{
				vkDestroyImageView(Device, renderInstancedObject.MateData.TextureImageViews[j], nullptr);
				vkDestroySampler(Device, renderInstancedObject.MateData.TextureSamplers[j], nullptr);
				vkDestroyImage(Device, renderInstancedObject.MateData.TextureImages[j], nullptr);
				vkFreeMemory(Device, renderInstancedObject.MateData.TextureImageMemorys[j], nullptr);
			}

			vkDestroyBuffer(Device, renderInstancedObject.MeshData.InstancedBuffer, nullptr);
			vkFreeMemory(Device, renderInstancedObject.MeshData.InstancedBufferMemory, nullptr);
			vkDestroyBuffer(Device, renderInstancedObject.MeshData.VertexBuffer, nullptr);
			vkFreeMemory(Device, renderInstancedObject.MeshData.VertexBufferMemory, nullptr);
			vkDestroyBuffer(Device, renderInstancedObject.MeshData.IndexBuffer, nullptr);
			vkFreeMemory(Device, renderInstancedObject.MeshData.IndexBufferMemory, nullptr);
		}

		// 清理 BaseSceneIndirectPass
		vkDestroyDescriptorSetLayout(Device, BaseSceneIndirectPass.DescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, BaseSceneIndirectPass.PipelineLayout, nullptr);
		for (uint32_t i = 0; i < GlobalConstants.SpecConstantsCount; i++)
		{
			vkDestroyPipeline(Device, BaseSceneIndirectPass.Pipelines[i], nullptr);
			vkDestroyPipeline(Device, BaseSceneIndirectPass.PipelinesInstanced[i], nullptr);
		}
		for (size_t i = 0; i < BaseSceneIndirectPass.RenderIndirectObject.size(); i++)
		{
			FRenderIndirectObject& RenderIndirectObject = BaseSceneIndirectPass.RenderIndirectObject[i];

			vkDestroyDescriptorPool(Device, RenderIndirectObject.MateData.DescriptorPool, nullptr);

			for (size_t j = 0; j < RenderIndirectObject.MateData.TextureImages.size(); j++)
			{
				vkDestroyImageView(Device, RenderIndirectObject.MateData.TextureImageViews[j], nullptr);
				vkDestroySampler(Device, RenderIndirectObject.MateData.TextureSamplers[j], nullptr);
				vkDestroyImage(Device, RenderIndirectObject.MateData.TextureImages[j], nullptr);
				vkFreeMemory(Device, RenderIndirectObject.MateData.TextureImageMemorys[j], nullptr);
			}
			vkDestroyBuffer(Device, RenderIndirectObject.MeshData.VertexBuffer, nullptr);
			vkFreeMemory(Device, RenderIndirectObject.MeshData.VertexBufferMemory, nullptr);
			vkDestroyBuffer(Device, RenderIndirectObject.MeshData.IndexBuffer, nullptr);
			vkFreeMemory(Device, RenderIndirectObject.MeshData.IndexBufferMemory, nullptr);
			vkDestroyBuffer(Device, RenderIndirectObject.IndirectCommandsBuffer, nullptr);
			vkFreeMemory(Device, RenderIndirectObject.IndirectCommandsBufferMemory, nullptr);
		}
		for (size_t i = 0; i < BaseSceneIndirectPass.RenderIndirectInstancedObject.size(); i++)
		{
			FRenderIndirectInstancedObject& RenderIndirectInstancedObject = BaseSceneIndirectPass.RenderIndirectInstancedObject[i];

			vkDestroyDescriptorPool(Device, RenderIndirectInstancedObject.MateData.DescriptorPool, nullptr);

			for (size_t j = 0; j < RenderIndirectInstancedObject.MateData.TextureImages.size(); j++)
			{
				vkDestroyImageView(Device, RenderIndirectInstancedObject.MateData.TextureImageViews[j], nullptr);
				vkDestroySampler(Device, RenderIndirectInstancedObject.MateData.TextureSamplers[j], nullptr);
				vkDestroyImage(Device, RenderIndirectInstancedObject.MateData.TextureImages[j], nullptr);
				vkFreeMemory(Device, RenderIndirectInstancedObject.MateData.TextureImageMemorys[j], nullptr);
			}

			vkDestroyBuffer(Device, RenderIndirectInstancedObject.MeshData.InstancedBuffer, nullptr);
			vkFreeMemory(Device, RenderIndirectInstancedObject.MeshData.InstancedBufferMemory, nullptr);
			vkDestroyBuffer(Device, RenderIndirectInstancedObject.MeshData.VertexBuffer, nullptr);
			vkFreeMemory(Device, RenderIndirectInstancedObject.MeshData.VertexBufferMemory, nullptr);
			vkDestroyBuffer(Device, RenderIndirectInstancedObject.MeshData.IndexBuffer, nullptr);
			vkFreeMemory(Device, RenderIndirectInstancedObject.MeshData.IndexBufferMemory, nullptr);
			vkDestroyBuffer(Device, RenderIndirectInstancedObject.IndirectCommandsBuffer, nullptr);
			vkFreeMemory(Device, RenderIndirectInstancedObject.IndirectCommandsBufferMemory, nullptr);
		}

		// 清理 BaseSceneDeferredPass
#if ENABLE_DEFEERED_RENDERING
		vkDestroyDescriptorSetLayout(Device, BaseSceneDeferredPass.LightingDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(Device, BaseSceneDeferredPass.LightingDescriptorPool, nullptr);
		vkDestroyPipelineLayout(Device, BaseSceneDeferredPass.LightingPipelineLayout, nullptr);
		for (uint32_t i = 0; i < GlobalConstants.SpecConstantsCount; i++)
		{
			vkDestroyPipeline(Device, BaseSceneDeferredPass.LightingPipelines[i], nullptr);
		}
		vkDestroyRenderPass(Device, BaseSceneDeferredPass.SceneRenderPass, nullptr);
		vkDestroyFramebuffer(Device, BaseSceneDeferredPass.SceneFrameBuffer, nullptr);
		vkDestroyDescriptorSetLayout(Device, BaseSceneDeferredPass.SceneDescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, BaseSceneDeferredPass.ScenePipelineLayout, nullptr);
		for (uint32_t i = 0; i < GlobalConstants.SpecConstantsCount; i++)
		{
			vkDestroyPipeline(Device, BaseSceneDeferredPass.ScenePipelines[i], nullptr);
			vkDestroyPipeline(Device, BaseSceneDeferredPass.ScenePipelinesInstanced[i], nullptr);
		}
		for (size_t i = 0; i < BaseSceneDeferredPass.RenderObjects.size(); i++)
		{
			FRenderObject& renderObject = BaseSceneDeferredPass.RenderObjects[i];

			vkDestroyDescriptorPool(Device, renderObject.MateData.DescriptorPool, nullptr);

			for (size_t j = 0; j < renderObject.MateData.TextureImages.size(); j++)
			{
				vkDestroyImageView(Device, renderObject.MateData.TextureImageViews[j], nullptr);
				vkDestroySampler(Device, renderObject.MateData.TextureSamplers[j], nullptr);
				vkDestroyImage(Device, renderObject.MateData.TextureImages[j], nullptr);
				vkFreeMemory(Device, renderObject.MateData.TextureImageMemorys[j], nullptr);
			}

			vkDestroyBuffer(Device, renderObject.MeshData.VertexBuffer, nullptr);
			vkFreeMemory(Device, renderObject.MeshData.VertexBufferMemory, nullptr);
			vkDestroyBuffer(Device, renderObject.MeshData.IndexBuffer, nullptr);
			vkFreeMemory(Device, renderObject.MeshData.IndexBufferMemory, nullptr);
		}
		for (size_t i = 0; i < BaseSceneDeferredPass.RenderInstancedObjects.size(); i++)
		{
			FRenderInstancedObject& renderInstancedObject = BaseSceneDeferredPass.RenderInstancedObjects[i];

			vkDestroyDescriptorPool(Device, renderInstancedObject.MateData.DescriptorPool, nullptr);

			for (size_t j = 0; j < renderInstancedObject.MateData.TextureImages.size(); j++)
			{
				vkDestroyImageView(Device, renderInstancedObject.MateData.TextureImageViews[j], nullptr);
				vkDestroySampler(Device, renderInstancedObject.MateData.TextureSamplers[j], nullptr);
				vkDestroyImage(Device, renderInstancedObject.MateData.TextureImages[j], nullptr);
				vkFreeMemory(Device, renderInstancedObject.MateData.TextureImageMemorys[j], nullptr);
			}

			vkDestroyBuffer(Device, renderInstancedObject.MeshData.InstancedBuffer, nullptr);
			vkFreeMemory(Device, renderInstancedObject.MeshData.InstancedBufferMemory, nullptr);
			vkDestroyBuffer(Device, renderInstancedObject.MeshData.VertexBuffer, nullptr);
			vkFreeMemory(Device, renderInstancedObject.MeshData.VertexBufferMemory, nullptr);
			vkDestroyBuffer(Device, renderInstancedObject.MeshData.IndexBuffer, nullptr);
			vkFreeMemory(Device, renderInstancedObject.MeshData.IndexBufferMemory, nullptr);
		}
		vkDestroyImageView(Device, GBuffer.DepthStencilImageView, nullptr);
		vkDestroySampler(Device, GBuffer.DepthStencilSampler, nullptr);
		vkDestroyImage(Device, GBuffer.DepthStencilImage, nullptr);
		vkFreeMemory(Device, GBuffer.DepthStencilMemory, nullptr);
		vkDestroyImageView(Device, GBuffer.SceneColorImageView, nullptr);
		vkDestroySampler(Device, GBuffer.SceneColorSampler, nullptr);
		vkDestroyImage(Device, GBuffer.SceneColorImage, nullptr);
		vkFreeMemory(Device, GBuffer.SceneColorMemory, nullptr);
		vkDestroyImageView(Device, GBuffer.GBufferAImageView, nullptr);
		vkDestroySampler(Device, GBuffer.GBufferASampler, nullptr);
		vkDestroyImage(Device, GBuffer.GBufferAImage, nullptr);
		vkFreeMemory(Device, GBuffer.GBufferAMemory, nullptr);
		vkDestroyImageView(Device, GBuffer.GBufferBImageView, nullptr);
		vkDestroySampler(Device, GBuffer.GBufferBSampler, nullptr);
		vkDestroyImage(Device, GBuffer.GBufferBImage, nullptr);
		vkFreeMemory(Device, GBuffer.GBufferBMemory, nullptr);
		vkDestroyImageView(Device, GBuffer.GBufferCImageView, nullptr);
		vkDestroySampler(Device, GBuffer.GBufferCSampler, nullptr);
		vkDestroyImage(Device, GBuffer.GBufferCImage, nullptr);
		vkFreeMemory(Device, GBuffer.GBufferCMemory, nullptr);
		vkDestroyImageView(Device, GBuffer.GBufferDImageView, nullptr);
		vkDestroySampler(Device, GBuffer.GBufferDSampler, nullptr);
		vkDestroyImage(Device, GBuffer.GBufferDImage, nullptr);
		vkFreeMemory(Device, GBuffer.GBufferDMemory, nullptr);
#endif

		vkDestroyCommandPool(Device, CommandPool, nullptr);

		vkDestroyDevice(Device, nullptr);

		if (bEnableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(Instance, Surface, nullptr);
		vkDestroyInstance(Instance, nullptr);
	}

protected:
	/** 选择SwapChain渲染到视图的图像的格式*/
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		// 找到合适的图像格式
		// VK_FORMAT_B8G8R8A8_SRGB 将图像BGRA存储在 unsigned normalized format 中，使用SRGB 非线性编码，颜色空间为非线性空间，不用Gamma矫正最终结果
		// VK_FORMAT_R8G8B8A8_UNORM 将图像BGRA存储在 unsigned normalized format 中，颜色空间为线性空间，像素的最终输出颜色需要Gamma矫正
		for (const auto& availableFormat : availableFormats)
		{
			// 将 FrameBuffer Image 设置为线性空间，方便 PBR 的工作流以及颜色矫正（ColorCorrection）
			//if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	/** 选择SwapChain的显示方式
	 * VK_PRESENT_MODE_IMMEDIATE_KHR 图形立即显示在屏幕上，会出现图像撕裂
	 * VK_PRESENT_MODE_FIFO_KHR 图像会被推入一个队列，先入后出显示到屏幕，如果队列满了，程序会等待，和垂直同步相似
	 * VK_PRESENT_MODE_FIFO_RELAXED_KHR 基于第二个Mode，当队列满了，程序不会等待，而是直接渲染到屏幕，会出现图像撕裂
	 * VK_PRESENT_MODE_MAILBOX_KHR 基于第二个Mode，当队列满了，程序不会等待，而是直接替换队列中的图像，
	*/
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities)
	{
		if (Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return Capabilities.currentExtent;
		}
		else
		{
			int Width, Height;
			glfwGetFramebufferSize(Window, &Width, &Height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(Width),
				static_cast<uint32_t>(Height)
			};

			actualExtent.width = std::clamp(actualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	FSwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice Device)
	{
		FSwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &formatCount, details.Formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	/** 检测硬件是否支持 multiDrawIndirect*/
	bool IsSupportMultiDrawIndirect(VkPhysicalDevice Device)
	{
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(Device, &supportedFeatures);
		return supportedFeatures.multiDrawIndirect;
	}

	/** 检测硬件是否合适*/
	bool IsDeviceSuitable(VkPhysicalDevice Device)
	{
		FQueueFamilyIndices queue_family_indices = FindQueueFamilies(Device);

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(Device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(Device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		bool extensionsSupported = requiredExtensions.empty();

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			FSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(Device);
			swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(Device, &supportedFeatures);

		return queue_family_indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	/** 队列家族 Queue Family
	 * 找到所有支持Vulkan的显卡硬件
	*/
	FQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice Device)
	{
		FQueueFamilyIndices queue_family_indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(Device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(Device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queue_family_indices.GraphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &presentSupport);

			if (presentSupport)
			{
				queue_family_indices.PresentFamily = i;
			}

			if (queue_family_indices.IsComplete())
			{
				break;
			}

			i++;
		}

		return queue_family_indices;
	}

	/** 找到物理硬件支持的图片格式*/
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	/** 找到支持深度贴图的格式*/
	VkFormat FindDepthFormat() {
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	/** 查找内存类型*/
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

		// 自动寻找适合的内存类型
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	VkCommandBuffer BeginSingleTimeCommands()
	{
		// 和渲染一样，使用commandBuffer拷贝缓存
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(Device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		vkFreeCommandBuffers(Device, CommandPool, 1, &commandBuffer);
	}

	/** 通用函数用来创建Buffer*/
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create buffer!");
		}

		// 为VertexBuffer创建内存，并赋予
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(Device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		// 自动找到适合的内存类型
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
		// 关联分配的内存地址
		if (vkAllocateMemory(Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}
		// 绑定VertexBuffer和它的内存地址
		vkBindBufferMemory(Device, buffer, bufferMemory, 0);
	}

	/** 通用函数用来拷贝Buffer*/
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(CommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommands(CommandBuffer);
	}

	/** 从文件中读取顶点和点序*/
	void CreateMesh(std::vector<FVertex>& outVertices, std::vector<uint32_t>& outIndices, const std::string& filename)
	{
		LoadModelAsset(filename, outVertices, outIndices);
	}

	/** 创建顶点缓存区VBO*/
	void CreateVertexBuffer(VkBuffer& outBuffer, VkDeviceMemory& outMemory, const std::vector<FVertex>& inVertices)
	{
		// 根据Vertices大小创建VertexBuffer
		VkDeviceSize bufferSize = sizeof(inVertices[0]) * inVertices.size();

		// 为什么需要stagingBuffer，因为直接创建VertexBuffer，CPU端可以直接通过VertexBufferMemory范围GPU使用的内存，这样太危险了，
		// 所以我们先创建一个临时的Buffer写入数据，然后将这个Buffer拷贝给最终的VertexBuffer，
		// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT标签，使得最终的VertexBuffer位于硬件本地内存中，比如显卡的显存。
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		// 通用函数用来创建VertexBuffer，这样可以方便创建StagingBuffer和真正的VertexBuffer
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// 把数据拷贝到顶点缓存区中
		void* data;
		vkMapMemory(Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, inVertices.data(), (size_t)bufferSize);
		vkUnmapMemory(Device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

		CopyBuffer(stagingBuffer, outBuffer, bufferSize);

		vkDestroyBuffer(Device, stagingBuffer, nullptr);
		vkFreeMemory(Device, stagingBufferMemory, nullptr);
	}

	/** 创建点序缓存区IBO*/
	void CreateIndexBuffer(VkBuffer& outBuffer, VkDeviceMemory& outMemory, const std::vector<uint32_t>& inIndices)
	{
		VkDeviceSize bufferSize = sizeof(inIndices[0]) * inIndices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, inIndices.data(), (size_t)bufferSize);
		vkUnmapMemory(Device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

		CopyBuffer(stagingBuffer, outBuffer, bufferSize);

		vkDestroyBuffer(Device, stagingBuffer, nullptr);
		vkFreeMemory(Device, stagingBufferMemory, nullptr);
	}

	/** 更新统一缓存区（UBO）*/
	void UpdateUniformBuffer(const uint32_t currentImageIdx)
	{
		glm::vec3 CameraPos = GlobalInput.CameraPos;
		glm::vec3 CameraLookat = GlobalInput.CameraLookat;
		glm::vec3 cameraUp = glm::vec3(0.0, 0.0, 1.0);
		float CameraFOV = GlobalInput.CameraFOV;
		float zNear = GlobalInput.zNear;
		float zFar = GlobalInput.zFar;

		static auto startTime = std::chrono::high_resolution_clock::now();
		auto CurrentTime = std::chrono::high_resolution_clock::now();
		float Time = std::chrono::duration<float, std::chrono::seconds::period>(CurrentTime - startTime).count();
		GlobalConstants.Time = Time;

		ShadowmapPass.zNear = zNear;
		ShadowmapPass.zFar = zFar;

		float RollLight = GlobalInput.bPlayLightRoll ?
			(GlobalInput.RollLight + GlobalInput.DeltaTime) : GlobalInput.RollLight;
		GlobalInput.RollLight = RollLight;

		glm::vec3 center = glm::vec3(0.0f);
		
		FLight* MoonLight = &View.DirectionalLights[0];
		glm::vec3 lightPos = glm::vec3(MoonLight->Position.x, MoonLight->Position.y, MoonLight->Position.z);
		float RollStage = GlobalInput.bPlayStageRoll ?
			(GlobalInput.RollStage + GlobalInput.DeltaTime * glm::radians(15.0f)) : GlobalInput.RollStage;
		GlobalInput.RollStage = RollStage;
		glm::mat4 localToWorld = glm::rotate(glm::mat4(1.0f), RollStage, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 shadowView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 shadowProjection = glm::perspective(glm::radians(CameraFOV), 1.0f, ShadowmapPass.zNear, ShadowmapPass.zFar);
		shadowProjection[1][1] *= -1;

		FUniformBufferBase UBOBaseData{};
		UBOBaseData.Model = localToWorld;
		UBOBaseData.View = glm::lookAt(CameraPos, CameraLookat, cameraUp);
		UBOBaseData.Proj = glm::perspective(glm::radians(CameraFOV), SwapChainExtent.width / (float)SwapChainExtent.height, zNear, zFar);
		UBOBaseData.Proj[1][1] *= -1;

		void* data_base_ubo;
		vkMapMemory(Device, BaseUniformBuffersMemory[currentImageIdx], 0, sizeof(UBOBaseData), 0, &data_base_ubo);
		memcpy(data_base_ubo, &UBOBaseData, sizeof(UBOBaseData));
		vkUnmapMemory(Device, BaseUniformBuffersMemory[currentImageIdx]);

		// ShadowmapSpace 的 MVP 矩阵中，M矩阵在FS中计算，所以传入 localToWorld 进入FS
		View.ShadowmapSpace = shadowProjection * shadowView;
		View.LocalToWorld = localToWorld;
		View.CameraInfo = glm::vec4(CameraPos, CameraFOV);
		uint32_t PointLightNum = POINT_LIGHTS_NUM;
		for (uint32_t i = 0; i < PointLightNum; i++)
		{
			float radians = ((float)i / (float)PointLightNum) * 360.0f - RollLight * 100.0f;
			float distance = ((float)i / (float)PointLightNum) * 5.0f + 2.5f;
			float X = sin(glm::radians(radians)) * distance;
			float Y = cos(glm::radians(radians)) * distance;
			float Z = 1.5;
			View.PointLights[i].Position = glm::vec4(X, Y, Z, 1.0);
		}
		View.LightsCount = glm::ivec4(1, PointLightNum, 0, CubemapMaxMips);
		View.zNear = ShadowmapPass.zNear;
		View.zFar = ShadowmapPass.zFar;

		void* data_view;
		vkMapMemory(Device, ViewUniformBuffersMemory[currentImageIdx], 0, sizeof(View), 0, &data_view);
		memcpy(data_view, &View, sizeof(View));
		vkUnmapMemory(Device, ViewUniformBuffersMemory[currentImageIdx]);

		FUniformBufferBase UBOShadowData{};
		UBOShadowData.Model = localToWorld;
		UBOShadowData.View = shadowView;
		UBOShadowData.Proj = shadowProjection;

		void* data_shadow_ubo;
		vkMapMemory(Device, ShadowmapPass.UniformBuffersMemory[currentImageIdx], 0, sizeof(UBOShadowData), 0, &data_shadow_ubo);
		memcpy(data_shadow_ubo, &UBOShadowData, sizeof(UBOShadowData));
		vkUnmapMemory(Device, ShadowmapPass.UniformBuffersMemory[currentImageIdx]);

		// Push all render objects into shadow map pending rendering list
		ShadowmapPass.RenderObjects.clear();
		ShadowmapPass.RenderInstancedObjects.clear();
		ShadowmapPass.RenderIndirectObject.clear();
		ShadowmapPass.RenderIndirectInstancedObject.clear();
		for (size_t i = 0; i < BaseScenePass.RenderObjects.size(); i++)
		{
			ShadowmapPass.RenderObjects.push_back(&BaseScenePass.RenderObjects[i]);
		}
		for (size_t i = 0; i < BaseScenePass.RenderInstancedObjects.size(); i++)
		{
			ShadowmapPass.RenderInstancedObjects.push_back(&BaseScenePass.RenderInstancedObjects[i]);
		}
		for (size_t i = 0; i < BaseSceneIndirectPass.RenderIndirectObject.size(); i++)
		{
			ShadowmapPass.RenderIndirectObject.push_back(&BaseSceneIndirectPass.RenderIndirectObject[i]);
		}
		for (size_t i = 0; i < BaseSceneIndirectPass.RenderIndirectInstancedObject.size(); i++)
		{
			ShadowmapPass.RenderIndirectInstancedObject.push_back(&BaseSceneIndirectPass.RenderIndirectInstancedObject[i]);
		}
#if ENABLE_DEFEERED_RENDERING
		for (size_t i = 0; i < BaseSceneDeferredPass.RenderObjects.size(); i++)
		{
			ShadowmapPass.RenderObjects.push_back(&BaseSceneDeferredPass.RenderObjects[i]);
		}
		for (size_t i = 0; i < BaseSceneDeferredPass.RenderInstancedObjects.size(); i++)
		{
			ShadowmapPass.RenderInstancedObjects.push_back(&BaseSceneDeferredPass.RenderInstancedObjects[i]);
		}
#endif
	}

	/** 创建Shader模块*/
	VkShaderModule CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create shader module!");
		}

		return shaderModule;
	}

	/**创建图形渲染管线*/
	void CreatePipelineLayout(VkPipelineLayout& outPipelineLayout, const VkDescriptorSetLayout& inDescriptorSetLayout)
	{
		// 设置 push constants
		VkPushConstantRange pushConstant;
		// 这个PushConstant的范围从头开始
		pushConstant.offset = 0;
		pushConstant.size = sizeof(FGlobalConstants);
		// 这是个全局PushConstant，所以希望各个着色器都能访问到
		pushConstant.stageFlags = VK_SHADER_STAGE_ALL;

		// 在渲染管线创建时，指定DescriptorSetLayout，用来传UniformBuffer
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &inDescriptorSetLayout;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
		pipelineLayoutInfo.pushConstantRangeCount = 1;

		// PipelineLayout可以用来创建和绑定VertexBuffer和UniformBuffer，这样可以往着色器中传递参数
		if (vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &outPipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to Create pipeline layout!");
		}
	}
	

	/**创建图形渲染管线*/
	void CreateGraphicsPipelines(
		std::vector<VkPipeline>& outPipelines,
		const VkPipelineLayout& inPipelineLayout,
		const VkRenderPass& inRenderPass,
		const uint32_t inSpecConstantsCount,
		const std::string& inVertFilename,
		const std::string& inFragFilename,
		const bool bDepthTest = true,
		const bool bCullBack = true,
		const bool bInstanced = false)
	{
		auto vertShaderCode = LoadShaderSource(inVertFilename);
		auto fragShaderCode = LoadShaderSource(inFragFilename);

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageCI{};
		vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageCI.module = vertShaderModule;
		vertShaderStageCI.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageCI{};
		fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageCI.module = fragShaderModule;
		fragShaderStageCI.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCI, fragShaderStageCI };

		// 顶点缓存绑定的描述，定义了顶点都需要绑定什么数据，比如第一个位置绑定Position，第二个位置绑定Color，第三个位置绑定UV等
		auto bindingDescription = FVertex::GetBindingDescription();
		auto attributeDescriptions = FVertex::GetAttributeDescriptions();
		auto bindingInstancedDescriptions = FVertex::GetBindingInstancedDescriptions();
		auto attributeInstancedDescriptions = FVertex::GetAttributeInstancedDescriptions();

		// 渲染管线VertexBuffer输入
		VkPipelineVertexInputStateCreateInfo vertexInputCI{};
		if (!bDepthTest && !bCullBack) // 如果不做深度检测并且不做背面剔除，那么认为是渲染背景，不需要绑定VBO
		{
			vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCI.vertexBindingDescriptionCount = 0;
			vertexInputCI.vertexAttributeDescriptionCount = 0;
		}
		else if (bInstanced)
		{
			vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCI.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingInstancedDescriptions.size());
			vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeInstancedDescriptions.size());
			vertexInputCI.pVertexBindingDescriptions = bindingInstancedDescriptions.data();
			vertexInputCI.pVertexAttributeDescriptions = attributeInstancedDescriptions.data();
		}
		else // 正常VBO渲染绑定
		{
			vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCI.vertexBindingDescriptionCount = 1;
			vertexInputCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputCI.pVertexBindingDescriptions = &bindingDescription;
			vertexInputCI.pVertexAttributeDescriptions = attributeDescriptions.data();
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
		inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCI.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizerCI{};
		rasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCI.depthClampEnable = VK_FALSE;
		rasterizerCI.rasterizerDiscardEnable = VK_FALSE;
		rasterizerCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCI.lineWidth = 1.0f;
		// 关闭背面剔除，使得材质TwoSide渲染
		rasterizerCI.cullMode = bCullBack ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
		rasterizerCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCI.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multiSamplingCI{};
		multiSamplingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multiSamplingCI.sampleShadingEnable = VK_FALSE;
		multiSamplingCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// 打开深度测试
		VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
		depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCI.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE;
		depthStencilCI.depthWriteEnable = bDepthTest ? VK_TRUE : VK_FALSE;
		depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCI.depthBoundsTestEnable = VK_FALSE;
		depthStencilCI.minDepthBounds = 0.0f; // Optional
		depthStencilCI.maxDepthBounds = 1.0f; // Optional
		depthStencilCI.stencilTestEnable = VK_FALSE; // 没有写轮廓信息，所以跳过轮廓测试
		depthStencilCI.front = {}; // Optional
		depthStencilCI.back = {}; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendingCI{};
		colorBlendingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingCI.logicOpEnable = VK_FALSE;
		colorBlendingCI.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingCI.attachmentCount = 1;
		colorBlendingCI.pAttachments = &colorBlendAttachment;

		colorBlendingCI.blendConstants[0] = 0.0f;
		colorBlendingCI.blendConstants[1] = 0.0f;
		colorBlendingCI.blendConstants[2] = 0.0f;
		colorBlendingCI.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateCI.pDynamicStates = dynamicStates.data();

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages;
		pipelineCI.pVertexInputState = &vertexInputCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pRasterizationState = &rasterizerCI;
		pipelineCI.pMultisampleState = &multiSamplingCI;
		pipelineCI.pDepthStencilState = &depthStencilCI; // 加上深度测试
		pipelineCI.pColorBlendState = &colorBlendingCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.layout = inPipelineLayout;
		pipelineCI.renderPass = inRenderPass;
		pipelineCI.subpass = 0;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;

		// Use specialization constants 优化着色器变体
		for (uint32_t i = 0; i < inSpecConstantsCount; i++)
		{
			uint32_t SpecConstants = i;
			VkSpecializationMapEntry specializationMapEntry = VkSpecializationMapEntry{};
			specializationMapEntry.constantID = 0;
			specializationMapEntry.offset = 0;
			specializationMapEntry.size = sizeof(uint32_t);
			VkSpecializationInfo specializationCI = VkSpecializationInfo();
			specializationCI.mapEntryCount = 1;
			specializationCI.pMapEntries = &specializationMapEntry;
			specializationCI.dataSize = sizeof(uint32_t);
			specializationCI.pData = &SpecConstants;
			shaderStages[1].pSpecializationInfo = &specializationCI;

			if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &outPipelines[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to Create graphics pipeline!");
			}
		}

		vkDestroyShaderModule(Device, fragShaderModule, nullptr);
		vkDestroyShaderModule(Device, vertShaderModule, nullptr);
	}


	/** 读取一个贴图路径，然后创建图像、视口和采样器等资源*/
	void CreateImageContext(
		VkImage& outImage,
		VkDeviceMemory& outMemory,
		VkImageView& outImageView,
		VkSampler& outSampler,
		const std::string& filename, bool sRGB = true)
	{
		int texWidth, texHeight, texChannels, mipLevels;
        std::vector<uint8_t> pixels;
        LoadTextureAsset(filename, pixels, texWidth, texHeight, texChannels, mipLevels);

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
		vkMapMemory(Device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels.data(), static_cast<size_t>(imageSize));
		vkUnmapMemory(Device, stagingBufferMemory);
        
		VkFormat format = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
		// VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT 告诉Vulkan这张贴图即要被读也要被写
		CreateImage(
			outImage,
			outMemory,
			texWidth, texHeight, format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels);

		TransitionImageLayout(outImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
		CopyBufferToImage(stagingBuffer, outImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		//TransitionImageLayout(outImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(Device, stagingBuffer, nullptr);
		vkFreeMemory(Device, stagingBufferMemory, nullptr);

		GenerateMipmaps(outImage, format, texWidth, texHeight, mipLevels);

		CreateImageView(outImageView, outImage, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
		CreateSampler(outSampler,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			mipLevels);
	}

	/** 读取一个HDR贴图路径，然后创建CUBEMAP图像资源*/
	void CreateImageHDRContext(
		VkImage& outImage,
		VkDeviceMemory& outMemory,
		VkImageView& outImageView,
		VkSampler& outSampler,
		uint32_t& outMaxMipLevels,
		const std::vector<std::string>& filenames)
	{
		// https://matheowis.github.io/HDRI-to-CubeMap/
		int texWidth, texHeight, texChannels, mipLevels;
        std::vector<std::vector<uint8_t>> pixels_array;
        pixels_array.resize(6);
		for (int i = 0; i < 6; ++i) {
			LoadTextureAsset(filenames[i], pixels_array[i], texWidth, texHeight, texChannels, mipLevels);
		}
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize * 6, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		for (int i = 0; i < 6; ++i) {
			void* writeLocation;
			vkMapMemory(Device, stagingBufferMemory, imageSize * i, imageSize, 0, &writeLocation);
			memcpy(writeLocation, pixels_array[i].data(), imageSize);
			vkUnmapMemory(Device, stagingBufferMemory);
		}

		// CreateImage
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = texWidth;
			imageInfo.extent.height = texHeight;
			imageInfo.extent.depth = 1;
			imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.mipLevels = mipLevels;
			imageInfo.arrayLayers = 6;
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

			if (vkCreateImage(Device, &imageInfo, nullptr, &outImage) != VK_SUCCESS) {
				throw std::runtime_error("failed to Create image!");
			}

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(Device, outImage, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			if (vkAllocateMemory(Device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate image memory!");
			}

			vkBindImageMemory(Device, outImage, outMemory, 0);
		}
		// TransitionImageLayout
		{
			VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = outImage;
			VkImageSubresourceRange subresourceRange;
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mipLevels;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 6;
			barrier.subresourceRange = subresourceRange;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			vkCmdPipelineBarrier(
				CommandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
			EndSingleTimeCommands(CommandBuffer);
		}
		// CopyBufferToImage
		{
			VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			VkImageSubresourceLayers imageSubresource;
			imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageSubresource.mipLevel = 0;
			imageSubresource.baseArrayLayer = 0;
			imageSubresource.layerCount = 6;
			region.imageSubresource = imageSubresource;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight),
				1
			};
			vkCmdCopyBufferToImage(CommandBuffer, stagingBuffer, outImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			EndSingleTimeCommands(CommandBuffer);
		}
		// GenerateMipmaps
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(PhysicalDevice, VK_FORMAT_R8G8B8A8_SRGB, &formatProperties);

			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
				throw std::runtime_error("texture image format does not support linear blitting!");
			}

			VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = outImage;
			VkImageSubresourceRange subresourceRange;
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1; // 注意，这里传入的CUBEMAP只有一级Mips，不是mipLevels
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 6;
			barrier.subresourceRange = subresourceRange;

			// 当layerCount = 6时，vkCmd执行时会 loop每个faces，所以不用在外部写循环去逐个执行Cubemap的faces ： for(uint32_t face = 0; face < 6; face++) {...}
			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;
			for (uint32_t i = 1; i < static_cast<uint32_t>(mipLevels); i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.subresourceRange.levelCount = 1; // 注意，这里传入的CUBEMAP只有一级Mips，不是mipLevels
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 6;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(CommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 6;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 6;

				vkCmdBlitImage(CommandBuffer,
					outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					outImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(CommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 6;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(CommandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			EndSingleTimeCommands(CommandBuffer);
		}
		vkDestroyBuffer(Device, stagingBuffer, nullptr);
		vkFreeMemory(Device, stagingBufferMemory, nullptr);

		// CreateImageView
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = outImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 6;
			if (vkCreateImageView(Device, &viewInfo, nullptr, &outImageView) != VK_SUCCESS) {
				throw std::runtime_error("failed to Create texture image View!");
			}
		}
		CreateSampler(outSampler,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			mipLevels);
		outMaxMipLevels = mipLevels;
	}

	/** 使用ImageMemoryBarrier，可以同步的访问贴图资源，避免一张贴图被读取时正在被写入*/
	void TransitionImageLayout(VkImage& image, const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t miplevels = 1)
	{
		VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = miplevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			CommandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		EndSingleTimeCommands(CommandBuffer);
	}

	void GenerateMipmaps(VkImage& outImage, const VkFormat& imageFormat, const int32_t texWidth, const int32_t texHeight, const uint32_t mipLevels)
	{
		// 检查图像格式是否支持 linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(PhysicalDevice, imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = outImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				outImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		EndSingleTimeCommands(commandBuffer);
	}

	/** 将缓存拷贝到图片对象中*/
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer CommandBuffer = BeginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(CommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndSingleTimeCommands(CommandBuffer);
	}

	/** 创建图像资源*/
	void CreateImage(
		VkImage& outImage,
		VkDeviceMemory& outImageMemory,
		const uint32_t inWidth, const uint32_t inHeight, const VkFormat format,
		const VkImageTiling tiling, const VkImageUsageFlags usage,
		const VkMemoryPropertyFlags properties, const uint32_t miplevels = 1)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = inWidth;
		imageInfo.extent.height = inHeight;
		imageInfo.extent.depth = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.arrayLayers = 1;
		imageInfo.mipLevels = miplevels;

		if (vkCreateImage(Device, &imageInfo, nullptr, &outImage) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(Device, outImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(Device, &allocInfo, nullptr, &outImageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(Device, outImage, outImageMemory, 0);
	}

	/** 创建图像视口*/
	void CreateImageView(
		VkImageView& outImageView,
		const VkImage& inImage,
		const VkFormat inFormat,
		const VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
		const uint32_t levelCount = 1)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = inImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = inFormat;
		viewInfo.subresourceRange.aspectMask = aspectFlags; // VK_IMAGE_ASPECT_COLOR_BIT 颜色 VK_IMAGE_ASPECT_DEPTH_BIT 深度
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = levelCount;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(Device, &viewInfo, nullptr, &outImageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create texture image View!");
		}
	}

	/** 创建采样器*/
	void CreateSampler(
		VkSampler& outSampler,
		const VkFilter filter = VK_FILTER_LINEAR,
		const VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		const VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		const VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		const VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		const uint32_t miplevels = 1)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(PhysicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = filter;
		samplerInfo.minFilter = filter;
		samplerInfo.addressModeU = addressModeU;
		samplerInfo.addressModeV = addressModeV;
		samplerInfo.addressModeW = addressModeW;
		// 可在此处关闭各项异性，有些硬件可能不支持
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = borderColor;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(miplevels);
		samplerInfo.mipLodBias = 0.0f;

		if (vkCreateSampler(Device, &samplerInfo, nullptr, &outSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create texture sampler!");
		}
	}

	/** 通用函数用来创建DescriptorSetLayout*/
	void CreateDescriptorSetLayout(VkDescriptorSetLayout& outDescriptorSetLayout, uint32_t inSamplerNumber = 1)
	{
		// UnifromBufferObject（ubo）绑定
		VkDescriptorSetLayoutBinding baseUBOLayoutBinding{};
		baseUBOLayoutBinding.binding = 0;
		baseUBOLayoutBinding.descriptorCount = 1;
		baseUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		baseUBOLayoutBinding.pImmutableSamplers = nullptr;
		baseUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		// UnifromBufferObject（ubo）绑定
		VkDescriptorSetLayoutBinding viewUBOLayoutBinding{};
		viewUBOLayoutBinding.binding = 1;
		viewUBOLayoutBinding.descriptorCount = 1;
		viewUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		viewUBOLayoutBinding.pImmutableSamplers = nullptr;
		viewUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // View ubo 主要信息用于 fragment shader

		// 环境反射Cubemap贴图绑定
		VkDescriptorSetLayoutBinding cubemapLayoutBinding{};
		cubemapLayoutBinding.binding = 2;
		cubemapLayoutBinding.descriptorCount = 1;
		cubemapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		cubemapLayoutBinding.pImmutableSamplers = nullptr;
		cubemapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// 阴影Shadowmap贴图绑定
		VkDescriptorSetLayoutBinding shadowmapLayoutBinding{};
		shadowmapLayoutBinding.binding = 3;
		shadowmapLayoutBinding.descriptorCount = 1;
		shadowmapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		shadowmapLayoutBinding.pImmutableSamplers = nullptr;
		shadowmapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// 将UnifromBufferObject和贴图采样器绑定到DescriptorSetLayout上
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.resize(inSamplerNumber + 4); // 这里3是2个UniformBuffer和1个环境Cubemap贴图
		bindings[0] = baseUBOLayoutBinding;
		bindings[1] = viewUBOLayoutBinding;
		bindings[2] = cubemapLayoutBinding;
		bindings[3] = shadowmapLayoutBinding;
		for (size_t i = 0; i < inSamplerNumber; i++)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding{};
			samplerLayoutBinding.binding = static_cast<uint32_t>(i + 4);
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			bindings[i + 4] = samplerLayoutBinding;
		}
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &outDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create descriptor set layout!");
		}
	}

	/** 通用函数用来创建DescriptorPool
	 * outDescriptorPool ：输出的DescriptorPool
	 * inSamplerNumber ：贴图采样器的数量
	 */
	void CreateDescriptorPool(VkDescriptorPool& outDescriptorPool, uint32_t inSamplerNumber = 1)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.resize(inSamplerNumber + 4);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < inSamplerNumber; i++)
		{
			poolSizes[i + 4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[i + 4].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		}

		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCI.pPoolSizes = poolSizes.data();
		poolCI.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(Device, &poolCI, nullptr, &outDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to Create descriptor pool!");
		}
	}

	/** 函数用来创建默认的只有一份贴图的DescriptorSets*/
	void CreateDescriptorSets(std::vector<VkDescriptorSet>& outDescriptorSets, const VkDescriptorPool& inDescriptorPool, const VkDescriptorSetLayout& inDescriptorSetLayout, const VkImageView& inImageView, const VkSampler& inSampler)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(inImageView);
		std::vector<VkSampler> samplers;
		samplers.push_back(inSampler);
		CreateDescriptorSets(outDescriptorSets, inDescriptorPool, inDescriptorSetLayout, imageViews, samplers);
	}

	/** 通用函数用来创建DescriptorSets*/
	void CreateDescriptorSets(std::vector<VkDescriptorSet>& outDescriptorSets, const VkDescriptorPool& inDescriptorPool, const VkDescriptorSetLayout& inDescriptorSetLayout, const std::vector<VkImageView>& inImageViews, const std::vector<VkSampler>& inSamplers)
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, inDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = inDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		outDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(Device, &allocInfo, outDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			uint32_t write_size = static_cast<uint32_t>(inImageViews.size()) + 4;
			std::vector<VkWriteDescriptorSet> descriptorWrites{};
			descriptorWrites.resize(write_size);

			// 绑定 UnifromBuffer
			VkDescriptorBufferInfo baseBufferInfo{};
			baseBufferInfo.buffer = BaseUniformBuffers[i];
			baseBufferInfo.offset = 0;
			baseBufferInfo.range = sizeof(FUniformBufferBase);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = outDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &baseBufferInfo;

			// 绑定 UnifromBuffer
			VkDescriptorBufferInfo viewBufferInfo{};
			viewBufferInfo.buffer = ViewUniformBuffers[i];
			viewBufferInfo.offset = 0;
			viewBufferInfo.range = sizeof(FUniformBufferView);

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = outDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &viewBufferInfo;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = CubemapImageView;
			imageInfo.sampler = CubemapSampler;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = outDescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &imageInfo;

			VkDescriptorImageInfo shadowmapImageInfo{};
			shadowmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			shadowmapImageInfo.imageView = ShadowmapPass.ImageView;
			shadowmapImageInfo.sampler = ShadowmapPass.Sampler;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = outDescriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &shadowmapImageInfo;

			// 绑定 Textures
			// descriptorWrites会引用每一个创建的VkDescriptorImageInfo，所以需要用一个数组把它们存储起来
			std::vector<VkDescriptorImageInfo> imageInfos;
			imageInfos.resize(inImageViews.size());
			for (size_t j = 0; j < inImageViews.size(); j++)
			{
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = inImageViews[j];
				imageInfo.sampler = inSamplers[j];
				imageInfos[j] = imageInfo;

				descriptorWrites[j + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j + 4].dstSet = outDescriptorSets[i];
				descriptorWrites[j + 4].dstBinding = static_cast<uint32_t>(j + 4);
				descriptorWrites[j + 4].dstArrayElement = 0;
				descriptorWrites[j + 4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[j + 4].descriptorCount = 1;
				descriptorWrites[j + 4].pImageInfo = &imageInfos[j]; // 注意，这里是引用了VkDescriptorImageInfo，所有需要创建imageInfos这个数组，存储所有的imageInfo而不是使用局部变量imageInfo
			}

			vkUpdateDescriptorSets(Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	template <typename T>
	void CreateRenderObject(T& outObject, const std::string& objfile, const std::vector<std::string>& pngfiles, const VkDescriptorSetLayout& inDescriptorSetLayout)
	{
		CreateMesh(outObject.MeshData.Vertices, outObject.MeshData.Indices, objfile);
		outObject.MateData.TextureImages.resize(pngfiles.size());
		outObject.MateData.TextureImageMemorys.resize(pngfiles.size());
		outObject.MateData.TextureImageViews.resize(pngfiles.size());
		outObject.MateData.TextureSamplers.resize(pngfiles.size());
		for (size_t i = 0; i < pngfiles.size(); i++)
		{
			// 一个便捷函数，创建图像，视口和采样器
			bool sRGB = (i == 0);
			CreateImageContext(
				outObject.MateData.TextureImages[i],
				outObject.MateData.TextureImageMemorys[i],
				outObject.MateData.TextureImageViews[i],
				outObject.MateData.TextureSamplers[i],
				pngfiles[i], sRGB);
		}

		CreateVertexBuffer(
			outObject.MeshData.VertexBuffer,
			outObject.MeshData.VertexBufferMemory,
			outObject.MeshData.Vertices);
		CreateIndexBuffer(
			outObject.MeshData.IndexBuffer,
			outObject.MeshData.IndexBufferMemory,
			outObject.MeshData.Indices);
		CreateDescriptorPool(
			outObject.MateData.DescriptorPool,
			static_cast<uint32_t>(pngfiles.size()));
		CreateDescriptorSets(
			outObject.MateData.DescriptorSets,
			outObject.MateData.DescriptorPool,
			inDescriptorSetLayout,
			outObject.MateData.TextureImageViews,
			outObject.MateData.TextureSamplers);
	};

	template <typename T>
	void CreateInstancedBuffer(T& outObject, const std::vector<FInstanceData>& inInstanceData)
	{
		outObject.InstanceCount = static_cast<uint32_t>(inInstanceData.size());
		VkDeviceSize bufferSize = inInstanceData.size() * sizeof(FInstanceData);
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, inInstanceData.data(), (size_t)bufferSize);
		vkUnmapMemory(Device, stagingBufferMemory);

		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			outObject.MeshData.InstancedBuffer,
			outObject.MeshData.InstancedBufferMemory);
		CopyBuffer(stagingBuffer, outObject.MeshData.InstancedBuffer, bufferSize);

		vkDestroyBuffer(Device, stagingBuffer, nullptr);
		vkFreeMemory(Device, stagingBufferMemory, nullptr);
	};

	template <typename T>
	void CreateRenderIndirectBuffer(T& outObject)
	{
		VkDeviceSize bufferSize = outObject.IndirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand);
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, outObject.IndirectCommands.data(), (size_t)bufferSize);
		vkUnmapMemory(Device, stagingBufferMemory);

		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			outObject.IndirectCommandsBuffer,
			outObject.IndirectCommandsBufferMemory);
		CopyBuffer(stagingBuffer, outObject.IndirectCommandsBuffer, bufferSize);

		vkDestroyBuffer(Device, stagingBuffer, nullptr);
		vkFreeMemory(Device, stagingBufferMemory, nullptr);
	};

private:
	/** 将编译的着色器二进制SPV文件，读入内存Buffer中*/
	static std::vector<char> LoadShaderSource(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			assert(true);
			throw std::runtime_error("failed to open shader file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
    
	/** 从图片文件中读取贴像素信息*/
	static void LoadTextureAsset(const std::string& filename, std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight, int& outChannels, int& outMipLevels)
	{
        stbi_hdr_to_ldr_scale(2.2f);
        stbi_uc* pixels = stbi_load(filename.c_str(), &outWidth, &outHeight, &outChannels, STBI_rgb_alpha);
        stbi_hdr_to_ldr_scale(1.0f);
        outMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(outWidth, outHeight)))) + 1;
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
            assert(true);
        }
        outPixels.resize(outWidth * outHeight * 4);
        std::memcpy(outPixels.data(), pixels, static_cast<size_t>(outWidth * outHeight * 4));
        // clear pixels data.
        stbi_image_free(pixels);
    }

	/** 从模型文件中读取贴顶点信息*/
	static void LoadModelAsset(const std::string& filename, std::vector<FVertex>& Vertices, std::vector<uint32_t>& Indices)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
			throw std::runtime_error(warn + err);
			assert(true);
		}

		std::unordered_map<FVertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				FVertex vertex{};

				vertex.Position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.Normal = {
					attrib.normals[3 * index.vertex_index + 0],
					attrib.normals[3 * index.vertex_index + 1],
					attrib.normals[3 * index.vertex_index + 2]
				};

				vertex.Color = { 1.0f, 1.0f, 1.0f };

				vertex.TexCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(Vertices.size());
					Vertices.push_back(vertex);
				}

				Indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

	/* 随机数引擎*/
	int RandRange(int min, int max)
	{
		//std::srand(seed);
		return min + (std::rand() % (max - min + 1));
	};

	/** 选择打印Debug信息的内容*/
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	/** 检查是否支持合法性检测*/
	bool CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : ValidationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	/** 打印调试信息时的回调函数，可以用来处理调试信息*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		std::cerr << "[LOG]: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};


/** 主函数*/
int main()
{
	FVulkanRendererApp EngineApp;

	try {
		EngineApp.MainTask();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}