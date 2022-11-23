-- https://premake.github.io/docs/Tokens/

workspace "Cardinalis"
	configurations { "Debug", "Release", "Debug(Validation)" }
	architecture "x64"
	engine_root = _WORKING_DIR .. "/"
	premake_dir = "premake/scripts/"
	outputdir = "%{cfg.buildcfg}-%{prj.name}-%{cfg.system}-%{cfg.architecture}" -- ex: Debug-Windows-x64

-- When creating a premake script, add it to the list 
	scripts = 
	{
		"core.lua",
		"projects.lua"
	}

-- Generate all projects
	for i, script in ipairs(scripts) do
		include(premake_dir .. script)
	end

------------------------------------------------------------------------------

-- Clean Function --
newaction 
{
	trigger     = "clean",
	description = "Removes generated files",
	execute     = function ()
	   print("Cleaning build folder...")
	   os.rmdir("build")
	   print("Done.")
	   print("Cleaning generated project files...")
	   os.remove("*.sln")
	   os.remove("**/*.vcxproj*")
	   os.rmdir(".vs")
	   print("Done.")
	end
 }