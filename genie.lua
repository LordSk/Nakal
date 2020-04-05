--
dofile("config.lua");

BUILD_DIR = path.getabsolute("./build")

solution "Nakal solution"
	location(BUILD_DIR)
	
	configurations {
		"Debug",
		"Release"
	}

	platforms {
		"x64"
	}
	
	language "C++"
	
	configuration {"Debug"}
		targetsuffix "_debug"
		flags {
			"Symbols"
		}
		defines {
			"DEBUG",
			"CONF_DEBUG"
		}
	
	configuration {"Release"}
		targetsuffix ""
		flags {
			"Optimize"
		}
		defines {
			"NDEBUG",
			"CONF_RELEASE"
		}
	
	configuration {}
	
	targetdir("../build")
	
	includedirs {
		"src",
	}

	libdirs {
	}
	
	links {
		"user32",
		"shell32",
		"winmm",
	}
	
	flags {
		"NoExceptions",
		"NoRTTI",
		"EnableSSE",
		"EnableSSE2",
		"EnableAVX",
		"EnableAVX2",
		"Cpp14"
	}
	
	defines {
		"UNICODE"
	}
	
	-- disable exception related warnings
	buildoptions{ "/wd4577", "/wd4530" }
	

project "Nakal"
	kind "WindowedApp"
	
	configuration {}

	flags {
		"WinMain"
	}
	
	files {
		"src/**.h",
		"src/**.c",
		"src/**.cpp",
        "src/**.rc",
	}