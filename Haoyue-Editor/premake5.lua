project "Haoyue-Editor"
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
		"src/**.h", 
		"src/**.c", 
		"src/**.hpp", 
		"src/**.cpp" 
	}
	
	includedirs 
	{
		"src",
		"../Haoyue/src",
		"../Haoyue/vendor",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.miniaudio}",
		"%{IncludeDir.Tracy}",
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
			"../Haoyue/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
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
			"../Haoyue/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
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
			"../Haoyue/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Haoyue/vendor/assimp/bin/Release/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../Haoyue/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
