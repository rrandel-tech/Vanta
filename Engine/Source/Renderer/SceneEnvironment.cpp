#include "vapch.hpp"
#include "SceneEnvironment.hpp"

#include "SceneRenderer.hpp"

namespace Vanta {

	Environment Environment::Load(const std::string& filepath)
	{
		auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(filepath);
		return { filepath, radiance, irradiance };
	}

}