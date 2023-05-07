-- https://premake.github.io/docs/Tokens/

project("CoreLib") 
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

	project_path = engine_root .. "src/core/"
	lib_dir = engine_root .. "src/thirdparty/"
	rendering_path = project_path .. "Rendering/"

	lib_list = {
		os.getenv("VULKAN_SDK") .. "/Lib",
	}

	location(project_path)

	files { 
		project_path .. "**.h", project_path .. "**.cpp"
	}

	vpaths { 
		["Rendering/Vulkan/*"] = { rendering_path .. "Vulkan/*" }, 
		["Window/*"] = { project_path .. "Window/*" }, 
		["Core"] = { project_path .. "Core/*" } 

	}

	includedirs {
		project_path,
		lib_dir .. "glm",
		lib_dir .. "stb",
		lib_dir .. "optick/include",
		lib_dir .. "spdlog",
		lib_dir .. "imgui",
		--lib_dir .. "assimp/include",
		lib_dir .. "cgltf",
		os.getenv("VULKAN_SDK") .. "/Include"
	}
	
	
	
	targetdir	(engine_root .. "build/bin/" .. outputdir )
	objdir		(engine_root .. "build/obj/" .. outputdir )

	filter "system:windows"
		systemversion "latest"
		defines { "WIN32_LEAN_AND_MEAN", "GLM_DEPTH_ZERO_TO_ONE" }
		links	{ "vulkan-1", "imgui", "OptickCore", "shcore" }
		
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		buildoptions {"/Od"}
		defines { "DEBUG", "ENABLE_VALIDATION_LAYERS", "GLM_DEPTH_ZERO_TO_ONE" }
		libdirs { lib_list, lib_dir .. "optick/lib/x64/debug/" }
		
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		defines { "RELEASE", "GLM_DEPTH_ZERO_TO_ONE" }
		libdirs { lib_list, lib_dir .. "optick/lib/x64/release/" }
