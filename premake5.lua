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

	defines
	{
		"TRACY_ENABLE",
		"TRACY_ON_DEMAND",
		"TRACY_CALLSTACK=10",
	}

	startproject "Haoyue-Editor"
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies.lua"

group "Dependencies"
	include "Haoyue/vendor/GLFW"
	include "Haoyue/vendor/ImGui"
	include "Haoyue/vendor/Box2D"
	include "Haoyue/vendor/tracy"
group ""

group "Core"
	include "Haoyue/premake5.lua"
	include "Haoyue-ScriptCore/premake5.lua"
group ""

group "Launcher"
	include "Haoyue-Hub/premake5.lua"
group ""

include "Haoyue-Editor/premake5.lua"
