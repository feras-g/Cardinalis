-- Define projects 

projects = 
{
	"SampleApp"
}

-- Generate projects 

for i, name in ipairs(projects) do
	project(name) 
		lib_dir = engine_root .. "src/thirdparty/"
		core_dir = engine_root .. "src/core/"
		project_dir  = engine_root .. "src/projects/" .. name .. "/"
		
		location (project_dir)
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++20"
		targetdir	(engine_root .. "build/bin/" .. outputdir )
		objdir		(engine_root .. "build/obj/" .. outputdir )
		libdirs		(lib_dir)

		libdirs 
		{
			os.getenv("VULKAN_SDK") .. "/Lib"
		}
		
		files 
		{ 
			project_dir .. "main.cpp",
			project_dir .. name ..".hpp",
		}


		includedirs {
			core_dir,
			project_dir,
			lib_dir .. "glm",
			lib_dir .. "stb",
			lib_dir .. "optick/include",
			lib_dir .. "spdlog",
			os.getenv("VULKAN_SDK") .. "/Include"
		}

		links
		{
			"CoreLib"
		}

		filter "system:windows"
			systemversion "latest"
			linkoptions { "/ENTRY:mainCRTStartup" } -- Allows using main() instead of WinMain(...) as the entry point

		filter "configurations:Debug"
			runtime "Debug"
			symbols "on"
			buildoptions {"/Od"}
			defines { "DEBUG", "ENABLE_VALIDATION_LAYERS" }
		
		filter "configurations:Release"
			runtime "Release"
			optimize "on"
			defines { "RELEASE" }
end

------------------------------------------------------------------------------

-- Clean Function --
newaction 
{
	trigger     = "clean",
	description = "Removes generated files",
	execute     = function ()
	   print("Cleaning build folder...")
	   os.remove(BuildPaths.ToyDX12 .. "/**")
	   print("Done.")
	   print("Cleaning generated project files...")
	   os.remove("ToyEngine.sln")
	   os.remove("ToyDX12/**/*.vcxproj*")
	   os.rmdir(".vs")
	   print("Done.")
	end
 }