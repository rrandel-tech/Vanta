#include "vapch.hpp"
#include "Camera.hpp"

namespace Vanta {

	Camera::Camera(const glm::mat4& projectionMatrix)
		: m_ProjectionMatrix(projectionMatrix)
	{
	}

}