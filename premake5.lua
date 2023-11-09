workspace "V3"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64" }
    staticruntime "On"
    runtime "Debug"

project "V3"
    --symbolspath '$(OutDir)$(TargetName).pdb'
    kind "WindowedApp" --WindowedApp --ConsoleApp
    language "C++"
    cppdialect "C++latest"
    targetdir "build/%{cfg.platform}/%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"
    editandcontinue "Off"
    characterset "ASCII"
    links {
        "SDL2",
        "SDL2main",
        "d3d11.lib",
        "dxgi.lib",
        "dxguid.lib",
    }

    libdirs {
        "contrib/SDL2/lib/%{cfg.platform}/",
        "contrib/ImGui",
        "contrib/tracy-master",
        --"contrib/**",
    }
    includedirs {
        "contrib",
        "contrib/ImGui",
        "contrib/SDL2/include",
        "contrib/tracy-master/public/tracy",
        --"contrib/GLEW/include",
        --"contrib/GLEW/include/GL",
    }
    flags {
        "MultiProcessorCompile",
        "FatalWarnings",
        "NoPCH",
    }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        --"GLEW_STATIC",
        --"CAMERA", --DO WE NEED THIS?
    }
    files {
        "Source/**",
        --"contrib/GLEW/src/glew.c",
        "contrib/tracy-master/public/TracyClient.cpp",
        "contrib/ImGui/*.cpp",
        "contrib/ImGui/*.h",
        --"contrib/ImGui/backends/imgui_impl_opengl3.*",
        "contrib/ImGui/backends/imgui_impl_sdl2.*",
        "contrib/ImGui/backends/imgui_impl_dx11.*",
        --"contrib/ImGui/backends/imgui_impl_opengl3.h",
        --"contrib/ImGui/backends/imgui_impl_opengl3.cpp",
        --"contrib/ImGui/backends/imgui_impl_sdl.h",
        --"contrib/ImGui/backends/imgui_impl_sdl.cpp",
    }


    postbuildcommands
    {
        "{COPY} contrib/SDL2/lib/%{cfg.platform}/SDL2.dll %{cfg.targetdir}",
        "{COPY} contrib/d3dcompiler_47.dll %{cfg.targetdir}",
    }


    filter "configurations:Debug"
        defines { "_DEBUG" , "TRACY_ENABLE", "NOMINMAX"}
        symbols  "On"
        optimize "Off"

    filter "configurations:Profile"
        defines { "NDEBUG" , "TRACY_ENABLE", "NOMINMAX"}
        symbols  "on"
        optimize "Speed"

    filter "configurations:Release"
        defines { "NDEBUG", "NOMINMAX" }
        symbols  "on"
        optimize "Speed"

    filter("files:**.hlsl")
        flags("ExcludeFromBuild")
