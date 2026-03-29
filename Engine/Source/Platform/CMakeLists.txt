if (WIN32)
    set(PLATFORM_SOURCES
            Windows/WindowsFileSystem.cpp
    )

elseif (UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES
    )

else()
    message(FATAL_ERROR "[Vanta] Unsupported platform.")
endif()

target_sources(Vanta
        PRIVATE
        ${PLATFORM_SOURCES}
        ${PLATFORM_HEADERS}
)