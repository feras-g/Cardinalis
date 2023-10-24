project "imgui"
    imgui_dir = engine_root .. "src/thirdparty/imgui/" 
    widgets_dir = imgui_dir .. "widgets/" 
    location (imgui_dir)

    targetdir	(engine_root .. "build/bin/" .. outputdir)
	objdir		(engine_root .. "build/obj/" .. outputdir)

    kind "StaticLib"
    files { "../*.h", "../*.cpp" }
    vpaths { ["imgui"] = { imgui_dir .. "*.cpp", imgui_dir .. "*.h", imgui_dir .. "misc/debuggers/*.natvis" } }
    filter { "toolset:msc*" }
        files { imgui_dir .. "misc/debuggers/*.natvis" }
    filter { "system:windows" }
    includedirs { imgui_dir, os.getenv("VULKAN_SDK") .. "/Include"  }
    files { 
        imgui_dir .. "*.h", imgui_dir .. "*.cpp", 
        imgui_dir .. "backends/imgui_impl_win32.h", 
        imgui_dir .. "backends/imgui_impl_vulkan.h", 
        imgui_dir .. "backends/imgui_impl_win32.cpp",
        imgui_dir .. "backends/imgui_impl_vulkan.cpp", 
        widgets_dir .. "imguizmo/" .. "*.h", widgets_dir .. "imguizmo/" .. "*.cpp",
    }

 