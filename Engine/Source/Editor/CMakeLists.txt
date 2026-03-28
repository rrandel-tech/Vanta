set(EDITOR_SOURCES
		EditorCamera.cpp
		SceneHierarchyPanel.cpp
)

set(EDITOR_HEADERS
		EditorCamera.hpp
		SceneHierarchyPanel.hpp
)

target_sources(Vanta
		PRIVATE
		${EDITOR_SOURCES}
		${EDITOR_HEADERS}
)