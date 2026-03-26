set(EDITOR_SOURCES
		SceneHierarchyPanel.cpp
)

set(EDITOR_HEADERS
		SceneHierarchyPanel.hpp
)

target_sources(Vanta
		PRIVATE
		${EDITOR_SOURCES}
		${EDITOR_HEADERS}
)