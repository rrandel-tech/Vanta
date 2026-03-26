set(IMGUI_SOURCES
        ImGuiLayer.cpp
        ImGuizmo.cpp
)

set(IMGUI_HEADERS
        ImGuiLayer.hpp
        ImGuizmo.h
)

target_sources(Vanta
        PRIVATE
        ${IMGUI_SOURCES}
        ${IMGUI_HEADERS}
)