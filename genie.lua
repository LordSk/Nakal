--
dofile("config.lua");

BUILD_DIR = path.getabsolute("./build")
COMMON_FILES = {
	"src/common/**.h",
	"src/common/**.c",
	"src/common/**.cpp"
}

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
		COMMON_FILES,
		"src/client/**.h",
		"src/client/**.c",
		"src/client/**.cpp",
        "src/client/**.rc",
	}

project "HookDll"
	kind "SharedLib"
	
	configuration {}

	files {
		COMMON_FILES,
		"src/hook/**.h",
		"src/hook/**.c",
		"src/hook/**.cpp",
        "src/hook/**.rc",
	}