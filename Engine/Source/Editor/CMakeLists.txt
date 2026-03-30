set(EDITOR_SOURCES
		EditorCamera.cpp
		SceneHierarchyPanel.cpp
		AssetEditorPanel.cpp
		ContentBrowserPanel.cpp
		DefaultAssetEditors.cpp
		ObjectsPanel.cpp
)

set(EDITOR_HEADERS
		EditorCamera.hpp
		SceneHierarchyPanel.hpp
		AssetEditorPanel.hpp
		ContentBrowserPanel.hpp
		DefaultAssetEditors.hpp
		ObjectsPanel.hpp
)

target_sources(Vanta
		PRIVATE
		${EDITOR_SOURCES}
		${EDITOR_HEADERS}
)