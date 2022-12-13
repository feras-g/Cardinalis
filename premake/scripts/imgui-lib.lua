project "imgui"
    imgui_dir = engine_root .. "src/thirdparty/imgui/" 

    kind "StaticLib"
    files { "../*.h", "../*.cpp" }
    vpaths { ["imgui"] = { imgui_dir .. "*.cpp", imgui_dir .. "*.h", imgui_dir .. "misc/debuggers/*.natvis" } }
    filter { "toolset:msc*" }
        files { imgui_dir .. "misc/debuggers/*.natvis" }
    filter { "system:windows" }
    includedirs { imgui_dir  }
    files { imgui_dir .. "*.h", imgui_dir .. "*.cpp", imgui_dir .. "backends/imgui_impl_win32.h", imgui_dir .. "backends/imgui_impl_win32.cpp" }

 