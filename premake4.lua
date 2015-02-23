solution "zc"
  local sdl2_config = "sdl2-config"

  if _ACTION == "clean" then
    os.rmdir("build")
  elseif _ACTION == "gmake" and os.is("windows") then
    local mingw32 = "i686-w64-mingw32"
    premake.gcc.cc = mingw32 .. "-gcc"
    sdl2_config = "/usr/" .. mingw32  .. "/sys-root/mingw/bin/" .. sdl2_config
    if not os.isfile(sdl2_config) then
      printf("WARNING: '%s' cannot be found.", sdl2_config)
    end
  end

  location "build"
  objdir "build"
  targetdir "build"
  language "C"

  configurations { "debug", "release" }
  includedirs { "src" }  

  configuration "debug"
    defines { "ZC_DEBUG" }

  configuration "release"
    defines { "ZC_NDEBUG" }

  configuration { "release", "gmake" }
    buildoptions { "-O3" }

  configuration { "gmake" }
    buildoptions { "`" .. sdl2_config .. " --cflags`" }
    linkoptions { "`" .. sdl2_config .. " --libs`" }

  configuration { "windows", "gmake" }
    links { "opengl32", "glu32" }    

  configuration { "macosx", "gmake" }
    links { "OpenGL.framework" }

  configuration { "linux", "gmake" }
    links { "GL", "GLU", "m" }

  configuration { "linux", "debug", "gmake" }
    buildoptions { "-Wall", "-Wextra", "-Werror", "-ansi", "-pedantic" }
    defines { "_GNU_SOURCE" }

  project "zc"
    kind "WindowedApp"
    files { "src/**.c" }
