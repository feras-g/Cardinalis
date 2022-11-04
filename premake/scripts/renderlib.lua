-- https://premake.github.io/docs/Tokens/

project("renderlib") 
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

	project_path = engine_root .. "src/lib/renderlib/"

	location(project_path)
	
	files { project_path .. "*.h", project_path .. "*.cpp"}

	includedirs
	{
		engine_root .. "thirdparty/glm"
	}

	links
	{
	}

	defines
	{
	}
	
	targetdir	(engine_root .. "build/bin/" .. outputdir )
	objdir		(engine_root .. "build/obj/" .. outputdir )

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		buildoptions {"/Od"}
		defines { "DEBUG_BUILD" }
		
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		defines { "RELEASE_BUILD" }

