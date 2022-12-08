project "imgui"
    kind "StaticLib"
    files { "../*.h", "../*.cpp" }
    vpaths { ["imgui"] = { "../*.cpp", "../*.h", "../misc/debuggers/*.natvis" } }
    filter { "toolset:msc*" }
        files { "../misc/debuggers/*.natvis" }
    filter { "system:windows" }
    includedirs { "../" }
    files { "../*.h", "../*.cpp", "../backends/imgui_impl_win32.h", "../backends/imgui_impl_win32.cpp" }

 