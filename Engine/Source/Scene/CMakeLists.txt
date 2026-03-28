set(SCENE_SOURCES
		Entity.cpp
		Scene.cpp
		SceneCamera.cpp
		SceneSerializer.cpp
)

set(SCENE_HEADERS
		Components.hpp
		Entity.hpp
		Scene.hpp
		SceneCamera.hpp
		SceneSerializer.hpp
)

target_sources(Vanta
		PRIVATE
		${SCENE_SOURCES}
		${SCENE_HEADERS}
)