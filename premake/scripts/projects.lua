-- Define projects 

projects = 
{
	"SampleProject"
}

-- Generate projects 

for i, name in ipairs(projects) do
	project(name) 
		user_lib_dir = engine_root .. "src/lib/"
		project_dir  = engine_root .. "src/projects/" .. name .. "/"
		
		location (project_dir)
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++20"

		files 
		{ 
			project_dir .. "main.cpp"
		}

		includedirs
		{
			user_lib_dir .. "renderlib",
			engine_root .. "thirdparty/glm"
		}

		links
		{
			"renderlib"
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