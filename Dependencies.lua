
-- HaoYue Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["Assimp"] = "%{wks.location}/HaoYue/vendor/assimp/include"
IncludeDir["stb"] = "%{wks.location}/HaoYue/vendor/stb/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/HaoYue/vendor/yaml-cpp/include"
IncludeDir["GLFW"] = "%{wks.location}/HaoYue/vendor/GLFW/include"
IncludeDir["ImGui"] = "%{wks.location}/HaoYue/vendor/ImGui"
IncludeDir["glm"] = "%{wks.location}/HaoYue/vendor/glm"
IncludeDir["Box2D"] = "%{wks.location}/HaoYue/vendor/Box2D/include"
IncludeDir["entt"] = "%{wks.location}/HaoYue/vendor/entt/include"
IncludeDir["FastNoise"] = "%{wks.location}/HaoYue/vendor/FastNoise"
IncludeDir["mono"] = "%{wks.location}/HaoYue/vendor/mono/include"
IncludeDir["PhysX"] = "%{wks.location}/HaoYue/vendor/PhysX/include"
IncludeDir["shaderc"] = "%{wks.location}/HaoYue/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/HaoYue/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["NvidiaAftermath"] = "%{wks.location}/HaoYue/vendor/NvidiaAftermath/include"

LibraryDir = {}

LibraryDir["PhysX"] = "%{wks.location}/HaoYue/vendor/PhysX/lib/%{cfg.buildcfg}"
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{wks.location}/HaoYue/vendor/VulkanSDK/Lib"
LibraryDir["NvidiaAftermath"] = "%{wks.location}/HaoYue/vendor/NvidiaAftermath/lib/x64"

Library = {}
Library["mono"] = "%{wks.location}/HaoYue/vendor/mono/lib/Debug/mono-2.0-sgen.lib"
Library["PhysX"] = "%{LibraryDir.PhysX}/PhysX_static_64.lib"
Library["PhysXCharacterKinematic"] = "%{LibraryDir.PhysX}/PhysXCharacterKinematic_static_64.lib"
Library["PhysXCommon"] = "%{LibraryDir.PhysX}/PhysXCommon_static_64.lib"
Library["PhysXCooking"] = "%{LibraryDir.PhysX}/PhysXCooking_static_64.lib"
Library["PhysXExtensions"] = "%{LibraryDir.PhysX}/PhysXExtensions_static_64.lib"
Library["PhysXFoundation"] = "%{LibraryDir.PhysX}/PhysXFoundation_static_64.lib"
Library["PhysXPvd"] = "%{LibraryDir.PhysX}/PhysXPvdSDK_static_64.lib"
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"
Library["NvidiaAftermath"] = "%{LibraryDir.NvidiaAftermath}/GFSDK_Aftermath_Lib.x64.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"