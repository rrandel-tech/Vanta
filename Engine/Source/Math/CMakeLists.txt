set(MATH_SOURCES
		Mat4.cpp
)

set(MATH_HEADERS
		Mat4.hpp
		AABB.hpp
		Ray.hpp
)

target_sources(Vanta
		PRIVATE
		${MATH_SOURCES}
		${MATH_HEADERS}
)