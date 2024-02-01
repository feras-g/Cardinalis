-- Define projects 

projects = 
{
	"SampleProject",
	"ComputeShaderToy"
}

-- Generate projects 
for i, name in ipairs(projects) do
	project(name) 
		src_dir = engine_root .. "src/"
		lib_dir = src_dir .. "thirdparty/"
		core_dir = src_dir .. "core/"
		project_dir  = src_dir .. "projects/" .. name .. "/"
		
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
			src_dir .. "projects/" .. "ProjectCommon.h"
		}

		
		if os.isfile(project_dir .. name ..".h") then
			files { project_dir .. name ..".h" }
		end

		if os.isfile(project_dir .. name ..".cpp") then
			files { project_dir .. name ..".cpp" }
		end

		if os.isfile(project_dir .. name ..".hpp") then
			files { project_dir .. name ..".hpp" }
		end

		includedirs {
			src_dir,
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
			"corelib"
		}

		filter "system:windows"
			systemversion "latest"
			linkoptions { "/ENTRY:mainCRTStartup" } -- Allows using main() instead of WinMain(...) as the entry point

		filter "configurations:Debug"
			runtime "Debug"
			symbols "on"
			buildoptions {"/Od" }
			defines { "ENGINE_DEBUG" }
			disablewarnings { 
				"4061", -- enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
				"4200", -- nonstandard extension used : zero-sized array in struct/union
				"4201", -- nonstandard extension used : nameless struct/union
				"4265", -- 'type': class has virtual functions, but its non-trivial destructor is not virtual; instances of this class may not be destructed correctly
				"4266", -- 'function' : no override available for virtual member function from base 'type'; function is hidden
				"4371", -- 'classname': layout of class may have changed from a previous version of the compiler due to better packing of member 'member'
				"4514", -- 'function' : unreferenced inline function has been removed
				"4582", -- 'type': constructor is not implicitly called
				"4583", -- 'type': destructor is not implicitly called
				"4623", -- 'derived class' : default constructor was implicitly defined as deleted because a base class default constructor is inaccessible or deleted
				"4625", -- 'derived class' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
				"4626", -- 'derived class' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
				"4710", -- 'function' : function not inlined
				"4711", -- function 'function' selected for inline expansion
				"4820", -- 'bytes' bytes padding added after construct 'member_name'
				"5026", -- 'type': move constructor was implicitly defined as deleted
				"5027", -- 'type': move assignment operator was implicitly defined as deleted
				"5045", -- Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
				"5053", -- support for 'explicit(<expr>)' in C++17 and earlier is a vendor extension
				"5204", -- 'type-name': class has virtual functions, but its trivial destructor is not virtual; instances of objects derived from this class may not be destructed correctly
				"5220", -- 'member': a non-static data member with a volatile qualified type no longer implies that compiler generated copy / move constructors and copy / move assignment operators are not trivial
				"4464", -- relative include path contains '..'
				"4191", -- '<function-style-cast>': unsafe conversion
				"4365", -- '=': conversion from 'unsigned int' to 'int', signed/unsigned mismatch
				"4365", -- '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
					
				
			}
		
		filter "configurations:Release"
			runtime "Release"
			optimize "on"
			defines { "ENGINE_RELEASE"}
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