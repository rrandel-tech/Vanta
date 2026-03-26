set(SCENE_SOURCES
		Entity.cpp
		Scene.cpp
)

set(SCENE_HEADERS
		Entity.hpp
		Scene.hpp
)

target_sources(Vanta
		PRIVATE
		${SCENE_SOURCES}
		${SCENE_HEADERS}
)