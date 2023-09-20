-- https://premake.github.io/docs/Tokens/

workspace "Cardinalis"
	configurations { "Debug", "Release" }
	architecture "x64"
	engine_root = _WORKING_DIR .. "/"
	premake_dir = "premake/scripts/"
	outputdir = "%{cfg.buildcfg}-%{prj.name}-%{cfg.system}-%{cfg.architecture}" -- ex: Debug-Windows-x64


-- Generate projects from scripts
	group "Core"
		include(premake_dir .. "imgui-lib.lua")
		include(premake_dir .. "core.lua")
	group "" -- end of "Core"

	group "Projects"
		include(premake_dir .. "projects.lua")
	group "" -- end of "Projects"
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