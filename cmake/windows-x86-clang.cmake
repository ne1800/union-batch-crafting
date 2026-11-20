set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

if(NOT DEFINED UBC_WINDOWS_SDK)
    message(FATAL_ERROR "UBC_WINDOWS_SDK muss auf das lokale xwin-SDK zeigen.")
endif()
get_filename_component(UBC_WINDOWS_SDK "${UBC_WINDOWS_SDK}" ABSOLUTE)
foreach(
    required
    crt/include
    sdk/include/ucrt
    sdk/include/shared
    sdk/include/um
    crt/lib/x86
    sdk/lib/ucrt/x86
    sdk/lib/um/x86
)
    if(NOT IS_DIRECTORY "${UBC_WINDOWS_SDK}/${required}")
        message(
            FATAL_ERROR
            "SDK-Verzeichnis fehlt: ${UBC_WINDOWS_SDK}/${required}"
        )
    endif()
endforeach()

set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER_TARGET i686-pc-windows-msvc)
set(CMAKE_LINKER lld-link)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES UBC_WINDOWS_SDK)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

set(CMAKE_CXX_FLAGS_INIT
    "--target=i686-pc-windows-msvc /imsvc\"${UBC_WINDOWS_SDK}/crt/include\" /imsvc\"${UBC_WINDOWS_SDK}/sdk/include/ucrt\" /imsvc\"${UBC_WINDOWS_SDK}/sdk/include/shared\" /imsvc\"${UBC_WINDOWS_SDK}/sdk/include/um\""
)
set(UBC_LIBRARY_PATHS
    "/libpath:\"${UBC_WINDOWS_SDK}/crt/lib/x86\" /libpath:\"${UBC_WINDOWS_SDK}/sdk/lib/ucrt/x86\" /libpath:\"${UBC_WINDOWS_SDK}/sdk/lib/um/x86\""
)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${UBC_LIBRARY_PATHS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${UBC_LIBRARY_PATHS}")
