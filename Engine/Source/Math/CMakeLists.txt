set(MATH_SOURCES
		Mat4.cpp
		Noise.cpp
		Math.cpp
)

set(MATH_HEADERS
		Mat4.hpp
		AABB.hpp
		Ray.hpp
		Noise.hpp
		Math.hpp
)

target_sources(Vanta
		PRIVATE
		${MATH_SOURCES}
		${MATH_HEADERS}
)