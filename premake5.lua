workspace "Haoyue"
	architecture "x64"
	targetdir "build"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	startproject "Haoyue-Editor"
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies.lua"

group "Dependencies"
include "Haoyue/vendor/GLFW"
include "Haoyue/vendor/ImGui"
include "Haoyue/vendor/Box2D"
group ""

group "Core"
project "Haoyue"
	location "Haoyue"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "Haoyue/src/pch.cpp"

	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp",

		"%{prj.name}/vendor/FastNoise/**.cpp",

		"%{prj.name}/vendor/yaml-cpp/src/**.cpp",
		"%{prj.name}/vendor/yaml-cpp/src/**.h",
		"%{prj.name}/vendor/yaml-cpp/include/**.h",
		"%{prj.name}/vendor/VulkanMemoryAllocator/**.h",
		"%{prj.name}/vendor/VulkanMemoryAllocator/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/vendor",

		"%{IncludeDir.Assimp}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.mono}",
		"%{IncludeDir.FastNoise}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.NvidiaAftermath}",
		"%{IncludeDir.miniaudio}",
	}
	
	links
	{ 
		"GLFW",
		"ImGui",
		"Box2D",

		"%{Library.Vulkan}",

		"%{Library.mono}",

		"%{Library.PhysX}",
		"%{Library.PhysXCharacterKinematic}",
		"%{Library.PhysXCommon}",
		"%{Library.PhysXCooking}",
		"%{Library.PhysXExtensions}",
		"%{Library.PhysXFoundation}",
		"%{Library.PhysXPvd}",
		
		"%{Library.NvidiaAftermath}",
	}

	defines
	{
		"PX_PHYSX_STATIC_LIB",
		"_CRT_SECURE_NO_WARNINGS",
		"_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING"
	}
	
	filter "files:Haoyue/vendor/FastNoise/**.cpp or files:Haoyue/vendor/yaml-cpp/src/**.cpp"
   	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }
		
		defines 
		{ 
			"HY_PLATFORM_WINDOWS",
			"HY_BUILD_DLL"
		}

	filter "configurations:Debug"
		defines "HY_DEBUG"
		symbols "On"
				
		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
			"%{Library.SPIRV_Tools_Debug}",
		}

	filter "configurations:Release"
		defines
		{
			"HY_RELEASE",
			"NDEBUG" -- PhysX Requires This
		}

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
		}

		optimize "On"

	filter "configurations:Dist"
		defines "HY_DIST"
		optimize "On"

project "Haoyue-ScriptCore"
	location "Haoyue-ScriptCore"
	kind "SharedLib"
	language "C#"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.cs", 
	}
group ""

group "Tools"
project "Haoyue-Editor"
	location "Haoyue-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	links 
	{ 
		"Haoyue"
	}
	
	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp" 
	}
	
	includedirs 
	{
		"%{prj.name}/src",
		"Haoyue/src",
		"Haoyue/vendor",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.miniaudio}",
	}

	postbuildcommands 
	{
		'{COPY} "../Haoyue/vendor/NvidiaAftermath/lib/x64/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"'
	}
	
	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }
				
		defines 
		{ 
			"HY_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug"
		defines "HY_DEBUG"
		symbols "on"

		links
		{
			"Haoyue/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Haoyue/vendor/assimp/bin/Debug/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../Haoyue/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
			'{COPY} "../Haoyue/vendor/VulkanSDK/Bin/shaderc_sharedd.dll" "%{cfg.targetdir}"'
		}
				
	filter "configurations:Release"
		defines "HY_RELEASE"
		optimize "on"

		links
		{
			"Haoyue/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Haoyue/vendor/assimp/bin/Release/assimp-vc141-mt.dll" "%{cfg.targetdir}"',
			'{COPY} "../Haoyue/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'

		}

	filter "configurations:Dist"
		defines "HY_DIST"
		optimize "on"

		links
		{
			"Haoyue/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Haoyue/vendor/assimp/bin/Release/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../Haoyue/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
group ""
