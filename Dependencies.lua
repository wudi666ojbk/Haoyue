
-- Haoyue Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["Assimp"] = "%{wks.location}/Haoyue/vendor/assimp/include"
IncludeDir["stb"] = "%{wks.location}/Haoyue/vendor/stb/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Haoyue/vendor/yaml-cpp/include"
IncludeDir["GLFW"] = "%{wks.location}/Haoyue/vendor/GLFW/include"
IncludeDir["ImGui"] = "%{wks.location}/Haoyue/vendor/ImGui"
IncludeDir["glm"] = "%{wks.location}/Haoyue/vendor/glm"
IncludeDir["Box2D"] = "%{wks.location}/Haoyue/vendor/Box2D/include"
IncludeDir["entt"] = "%{wks.location}/Haoyue/vendor/entt/include"
IncludeDir["FastNoise"] = "%{wks.location}/Haoyue/vendor/FastNoise"
IncludeDir["mono"] = "%{wks.location}/Haoyue/vendor/mono/include"
IncludeDir["PhysX"] = "%{wks.location}/Haoyue/vendor/PhysX/include"
IncludeDir["shaderc"] = "%{wks.location}/Haoyue/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Haoyue/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["NvidiaAftermath"] = "%{wks.location}/Haoyue/vendor/NvidiaAftermath/include"
IncludeDir["miniaudio"] = "%{wks.location}/Haoyue/vendor/miniaudio/include"
IncludeDir["Tracy"] = "%{wks.location}/Haoyue/vendor/tracy/tracy/public"

LibraryDir = {}

LibraryDir["PhysX"] = "%{wks.location}/Haoyue/vendor/PhysX/lib/%{cfg.buildcfg}"
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{wks.location}/Haoyue/vendor/VulkanSDK/Lib"
LibraryDir["NvidiaAftermath"] = "%{wks.location}/Haoyue/vendor/NvidiaAftermath/lib/x64"

Library = {}
Library["mono"] = "%{wks.location}/Haoyue/vendor/mono/lib/Debug/mono-2.0-sgen.lib"
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