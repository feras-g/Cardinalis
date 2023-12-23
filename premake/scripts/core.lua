-- https://premake.github.io/docs/Tokens/

project("corelib") 
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

	project_path = engine_root .. "src/core/"
	lib_dir = engine_root .. "src/thirdparty/"

	lib_list = {
		os.getenv("VULKAN_SDK") .. "/Lib",
	}

	location(project_path)

	files 
	{ 
		project_path .. "**.h", 
		project_path .. "**.cpp",
		project_path .. "**.hpp",
	}
	removefiles 
	{ 
		engine_root .. "**/exclude/**",
	}

	vpaths { 
		["core"] = { project_path .. "/*" }, 
		["data/*"] = { engine_root .. "data/*" },
	}

	includedirs {
		engine_root .. "src",
		lib_dir .. "glm",
		lib_dir .. "stb",
		lib_dir .. "optick/include",
		lib_dir .. "spdlog",
		lib_dir .. "imgui",
		lib_dir .. "imgui/widgets/imguizmo",
		lib_dir .. "cgltf",
		os.getenv("VULKAN_SDK") .. "/Include"
	}
	
	targetdir	(engine_root .. "build/bin/" .. outputdir )
	objdir		(engine_root .. "build/obj/" .. outputdir )

	filter "system:windows"
		systemversion "latest"
		defines { "WIN32_LEAN_AND_MEAN", "GLM_FORCE_DEPTH_ZERO_TO_ONE", "_CRT_SECURE_NO_WARNINGS"}
		links	{ "vulkan-1", "imgui", "OptickCore", "shcore" }
		
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		buildoptions {"/Od", "/WX",  "/permissive-"}
		defines { "ENGINE_DEBUG", "GLM_DEPTH_ZERO_TO_ONE" }
		libdirs { lib_list, lib_dir .. "optick/lib/x64/debug/" }
		
	filter "configurations:Release"
		runtime "Release"
		optimize "off"
		buildoptions { "/WX",  "/permissive-"}
		defines { "ENGINE_RELEASE", "GLM_DEPTH_ZERO_TO_ONE" }
		libdirs { lib_list, lib_dir .. "optick/lib/x64/release/" }