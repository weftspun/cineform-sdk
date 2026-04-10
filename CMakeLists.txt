# CMakeLists.txt
cmake_minimum_required(VERSION 4.2)

project (CineFormSDK
	DESCRIPTION "Desktop Core Tech Library"
	LANGUAGES C CXX
)

# Build settings
option(BUILD_LIBS "Build codec libraries" ON)
option(BUILD_TOOLS "Build test tools" ON)
option(BUILD_STATIC "Build static component" ON)
option(BUILD_SEPARATED "Build a separate de/encoder libraries" OFF)

add_definitions(-D_ALLOCATOR=1 -DWARPSTUFF=1 -D_UNICODE=1)

find_package(OpenMP)

if (WIN32)
	SET(COMPILER_FLAGS "")
endif (WIN32)

if (UNIX)
	SET(COMPILER_FLAGS -fPIC)
	SET(ADDITIONAL_LIBS "-luuid -lpthread")
	SET(TOY_LIBS "-lm")
endif (UNIX)

if (APPLE)
	SET(COMPILER_FLAGS -fvisibility=hidden)
	SET(ADDITIONAL_LIBS "-lpthread")
endif (APPLE)

if (OPENMP_FOUND)
	add_definitions(-DHAVE_OPENMP=1)
	SET(ADDITIONAL_LIBS "${ADDITIONAL_LIBS} ${OpenMP_CXX_FLAGS}")
endif (OPENMP_FOUND)
string(STRIP ${ADDITIONAL_LIBS} ADDITIONAL_LIBS)

file(GLOB CODEC_SOURCES "Codec/*.c" "Codec/*.h" "Codec/*.cpp" "WarpLib/*.c" "WarpLib/*.h" )
file(GLOB ENCODER_SOURCES "EncoderSDK/*.cpp" "Common/*.h")
file(GLOB DECODER_SOURCES "DecoderSDK/*.cpp" "Common/*.h" "WarpLib/*.c" "WarpLib/*.h" "ConvertLib/*.cpp" "ConvertLib/*.h" )
file(GLOB EXAMPLE_SOURCE "Example/*.cpp" "Example/*.h" )
file(GLOB WAVELETDEMO_SOURCE "Example/WaveletDemo/*.c" "Example/WaveletDemo/*.h" )
file(GLOB PUBLIC_HEADERS "Common/*.h")

# Build CFHDCodec library (static and shared rules)
if (BUILD_LIBS)
	if (BUILD_STATIC)
		if (BUILD_SEPARATED)
			add_library(CFHDEncoderStatic STATIC ${CODEC_SOURCES} ${ENCODER_SOURCES})
			add_library(CFHDDecoderStatic STATIC ${CODEC_SOURCES} ${DECODER_SOURCES})

			set_target_properties(CFHDEncoderStatic PROPERTIES
			    FOLDER Media
			    OUTPUT_NAME CFHDEncoderStatic
			    CXX_STANDARD 17
				C_STANDARD 11
			)
			
			set_target_properties(CFHDDecoderStatic PROPERTIES
			    FOLDER Media
			    OUTPUT_NAME CFHDDecoderStatic
			    CXX_STANDARD 17
				C_STANDARD 11
			)

			target_compile_options(CFHDEncoderStatic PUBLIC ${COMPILER_FLAGS})
			target_compile_options(CFHDDecoderStatic PUBLIC ${COMPILER_FLAGS})		
			
			target_link_libraries (CFHDEncoderStatic)
			target_link_libraries (CFHDDecoderStatic)

			target_include_directories(CFHDEncoderStatic
				PUBLIC
					Common
				PRIVATE
					Codec
					Tables
					ConvertLib
					WarpLib
					Example
			)

			target_include_directories(CFHDDecoderStatic
				PUBLIC
					Common
				PRIVATE
					Codec
					Tables
					ConvertLib
					WarpLib
					Example
			)

		else (BUILD_SEPARATED)
			add_library(CFHDCodecStatic STATIC ${CODEC_SOURCES} ${ENCODER_SOURCES} ${DECODER_SOURCES})

			set_target_properties(CFHDCodecStatic PROPERTIES
			    FOLDER Media
			    OUTPUT_NAME CFHDCodecStatic
			    CXX_STANDARD 17
				C_STANDARD 11
			)
			
			target_compile_options(CFHDCodecStatic PUBLIC ${COMPILER_FLAGS})
		
			set_target_properties(CFHDCodecStatic PROPERTIES POSITION_INDEPENDENT_CODE ON)
			if (UNIX)
				set_target_properties(CFHDCodecStatic PROPERTIES OUTPUT_NAME CFHDCodec)
			endif (UNIX)
		
			target_link_libraries(CFHDCodecStatic)

			target_include_directories(CFHDCodecStatic
				PUBLIC
					Common
				PRIVATE
					Codec
					Tables
					ConvertLib
					WarpLib
					Example
			)
		endif (BUILD_SEPARATED)
	else (BUILD_STATIC)
		if (BUILD_SEPARATED)
			add_library(CFHDEncoder SHARED ${CODEC_SOURCES} ${ENCODER_SOURCES})
			add_library(CFHDDecoder SHARED ${CODEC_SOURCES} ${DECODER_SOURCES})

			set_target_properties(CFHDEncoder PROPERTIES
			    FOLDER Media
			    OUTPUT_NAME CFHDEncoder
			    CXX_STANDARD 17
				C_STANDARD 11
			)
			
			set_target_properties(CFHDDecoder PROPERTIES
			    FOLDER Media
			    OUTPUT_NAME CFHDDecoder
			    CXX_STANDARD 17
				C_STANDARD 11
			)

			target_compile_options(CFHDEncoder PUBLIC ${COMPILER_FLAGS})
			target_compile_options(CFHDDecoder PUBLIC ${COMPILER_FLAGS})
			
			target_compile_definitions(CFHDEncoder PUBLIC -DDYNAMICLIB=1)
			target_compile_definitions(CFHDDecoder PUBLIC -DDYNAMICLIB=1)
			
			target_link_libraries (CFHDEncoder)
			target_link_libraries (CFHDDecoder)

			target_include_directories(CFHDEncoder
				PUBLIC
					Common
				PRIVATE
					Codec
					Tables
					ConvertLib
					WarpLib
					Example
			)

			target_include_directories(CFHDDecoder
				PUBLIC
					Common
				PRIVATE
					Codec
					Tables
					ConvertLib
					WarpLib
					Example
			)
		else (BUILD_SEPARATED)
			add_library(CFHDCodecShared SHARED ${CODEC_SOURCES} ${ENCODER_SOURCES} ${DECODER_SOURCES})

			set_target_properties(CFHDCodecShared PROPERTIES
			    FOLDER Media
			    OUTPUT_NAME CFHDCodec
			    CXX_STANDARD 17
				C_STANDARD 11
				POSITION_INDEPENDENT_CODE ON
			)

			target_compile_options(CFHDCodecShared PUBLIC ${COMPILER_FLAGS})
			target_compile_definitions(CFHDCodecShared PUBLIC -DDYNAMICLIB=1 -DCODECCOMBINED=1)
			
			target_link_libraries(CFHDCodecShared)

			target_include_directories(CFHDCodecShared
				PUBLIC
					Common
				PRIVATE
					Codec
					Tables
					ConvertLib
					WarpLib
					Example
			)
		endif (BUILD_SEPARATED)
	endif (BUILD_STATIC)
endif (BUILD_LIBS)

# Build tools
if (BUILD_TOOLS)
    # TestCFHD
	add_executable(TestCFHD ${EXAMPLE_SOURCE})

	set_target_properties(TestCFHD PROPERTIES
		FOLDER Media/Tests
		OUTPUT_NAME TestCFHD
		CXX_STANDARD 17
		C_STANDARD 11
	)

	if (OPENMP_FOUND)
		target_compile_options(TestCFHD PRIVATE ${OpenMP_CXX_FLAGS})
	endif ()


    if (BUILD_STATIC)
		if (BUILD_SEPARATED)
			target_link_libraries(TestCFHD CFHDEncoderStatic CFHDDecoderStatic ${INTERNAL_LIBS} ${ADDITIONAL_LIBS})
		else (BUILD_SEPARATED)
			target_link_libraries(TestCFHD CFHDCodecStatic ${INTERNAL_LIBS} ${ADDITIONAL_LIBS})
		endif (BUILD_SEPARATED)
    else (BUILD_STATIC)
		if (BUILD_SEPARATED)
			target_link_libraries(TestCFHD CFHDEncoder CFHDDecoder  ${INTERNAL_LIBS} ${ADDITIONAL_LIBS})
		else (BUILD_SEPARATED)
			target_link_libraries(TestCFHD CFHDCodecShared ${INTERNAL_LIBS} ${ADDITIONAL_LIBS})
		endif (BUILD_SEPARATED)
    endif (BUILD_STATIC)

    # WaveletDemo
    add_executable(WaveletDemo ${WAVELETDEMO_SOURCE})

	set_target_properties(WaveletDemo PROPERTIES
	    FOLDER Media/Tools
	    OUTPUT_NAME WaveletDemo
	    CXX_STANDARD 17
		C_STANDARD 11
	)

    target_link_libraries(WaveletDemo ${TOY_LIBS})
endif (BUILD_TOOLS)
