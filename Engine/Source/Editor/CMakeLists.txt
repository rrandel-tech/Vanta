set(EDITOR_SOURCES
		EditorCamera.cpp
		SceneHierarchyPanel.cpp
		AssetEditorPanel.cpp
		AssetManagerPanel.cpp
		DefaultAssetEditors.cpp
		ObjectsPanel.cpp
)

set(EDITOR_HEADERS
		EditorCamera.hpp
		SceneHierarchyPanel.hpp
		AssetEditorPanel.hpp
		AssetManagerPanel.hpp
		DefaultAssetEditors.hpp
		ObjectsPanel.hpp
)

target_sources(Vanta
		PRIVATE
		${EDITOR_SOURCES}
		${EDITOR_HEADERS}
)