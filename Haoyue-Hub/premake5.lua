project "Haoyue-Hub"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
    targetname "Hazel-Launcher"
	
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
		"../Haoyue/vendor"
	}

	postbuildcommands 
	{
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
		}

		postbuildcommands 
		{
		}
				
	filter "configurations:Release"
		defines "HY_RELEASE"
		optimize "on"

		links
		{
		}

		postbuildcommands 
		{
		}

	filter "configurations:Dist"
		defines "HY_DIST"
		optimize "on"

		links
		{
		}

		postbuildcommands 
		{
		}
