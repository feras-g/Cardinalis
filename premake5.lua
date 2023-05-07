-- https://premake.github.io/docs/Tokens/

workspace "Cardinalis"
	configurations { "Debug", "Release" }
	architecture "x64"
	engine_root = _WORKING_DIR .. "/"
	premake_dir = "premake/scripts/"
	outputdir = "%{cfg.buildcfg}-%{prj.name}-%{cfg.system}-%{cfg.architecture}" -- ex: Debug-Windows-x64


-- Generate projects based on a list of scripts
	scripts = 
	{
		--"assimp-lib.lua",
		"imgui-lib.lua",
		"core.lua",
		"projects.lua"
	}

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