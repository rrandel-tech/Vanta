set(CORE_SOURCES
        Application.cpp
        Base.cpp
        Log.cpp
        Window.cpp
)

set(CORE_HEADERS
        Application.hpp
        Assert.hpp
        Base.hpp
        Log.hpp
        Version.hpp
        Window.hpp
)

target_sources(Vanta
        PRIVATE
        ${CORE_SOURCES}
        ${CORE_HEADERS}
)