set(IMGUI_SOURCES
        ImGuiLayer.cpp
)

set(IMGUI_HEADERS
        ImGuiLayer.hpp
)

target_sources(Vanta
        PRIVATE
        ${IMGUI_SOURCES}
        ${IMGUI_HEADERS}
)