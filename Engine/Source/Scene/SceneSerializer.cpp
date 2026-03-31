#include "vapch.hpp"
#include "SceneSerializer.hpp"

#include "Entity.hpp"
#include "Components.hpp"
#include "Renderer/MeshFactory.hpp"

#include "Asset/AssetManager.hpp"

#include "yaml-cpp/yaml.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::quat>
	{
		static Node encode(const glm::quat& rhs)
		{
			Node node;
			node.push_back(rhs.w);
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::quat& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.w = node[0].as<float>();
			rhs.x = node[1].as<float>();
			rhs.y = node[2].as<float>();
			rhs.z = node[3].as<float>();
			return true;
		}
	};
}

namespace Vanta {

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}


	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.w << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	/*static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}*/

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		UUID uuid = entity.GetComponent<IDComponent>().ID;
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity";
		out << YAML::Value << uuid;

		if (entity.HasComponent<RelationshipComponent>())
		{
			auto& relationshipComponent = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << relationshipComponent.ParentHandle;

			out << YAML::Key << "Children";
			out << YAML::Value << YAML::BeginSeq;

			for (auto child : relationshipComponent.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << child;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& transform = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap; // MeshComponent

			auto mesh = entity.GetComponent<MeshComponent>().Mesh;
			if (mesh)
			{
				auto meshAsset = mesh->GetMeshAsset();
				out << YAML::Key << "AssetID" << YAML::Value << meshAsset->Handle;
			}
			else
				out << YAML::Key << "AssetID" << YAML::Value << 0;

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;
			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera
			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			auto& directionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Radiance" << YAML::Value << directionalLightComponent.Radiance;
			out << YAML::Key << "CastShadows" << YAML::Value << directionalLightComponent.CastShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << directionalLightComponent.SoftShadows;
			out << YAML::Key << "LightSize" << YAML::Value << directionalLightComponent.LightSize;

			out << YAML::EndMap; // DirectionalLightComponent
		}

		if (entity.HasComponent<SkyLightComponent>())
		{
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap; // SkyLightComponent

			auto& skyLightComponent = entity.GetComponent<SkyLightComponent>();
			out << YAML::Key << "EnvironmentMap" << YAML::Value << skyLightComponent.SceneEnvironment->Handle;
			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;
			out << YAML::Key << "Angle" << YAML::Value << skyLightComponent.Angle;

			out << YAML::EndMap; // SkyLightComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
				out << YAML::Key << "TextureAssetPath" << YAML::Value << "path/to/asset";
			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		out << YAML::EndMap; // Entity
	}

	static void SerializeEnvironment(YAML::Emitter& out, const Ref<Scene>& scene)
	{
		out << YAML::Key << "Environment";
		out << YAML::Value;
		out << YAML::BeginMap; // Environment
		out << YAML::Key << "AssetHandle" << YAML::Value << scene->GetEnvironment()->Handle;
		const auto& light = scene->GetLight();
		out << YAML::Key << "Light" << YAML::Value;
		out << YAML::BeginMap; // Light
		out << YAML::Key << "Direction" << YAML::Value << light.Direction;
		out << YAML::Key << "Radiance" << YAML::Value << light.Radiance;
		out << YAML::Key << "Multiplier" << YAML::Value << light.Multiplier;
		out << YAML::EndMap; // Light
		out << YAML::EndMap; // Environment
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene";
		out << YAML::Value << "Scene Name";

		if (m_Scene->GetEnvironment())
			SerializeEnvironment(out, m_Scene);

		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, m_Scene.Raw() };
			if (!entity || !entity.HasComponent<IDComponent>())
				return;

			SerializeEntity(out, entity);

		});
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		VA_CORE_ASSERT(false);
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		VA_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		VA_CORE_INFO("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				VA_CORE_INFO("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntityWithID(uuid, name);

				auto& relationshipComponent = deserializedEntity.GetComponent<RelationshipComponent>();
				uint64_t parentHandle = entity["Parent"] ? entity["Parent"].as<uint64_t>() : 0;
				relationshipComponent.ParentHandle = parentHandle;

				auto children = entity["Children"];
				if (children)
				{
					for (auto child : children)
					{
						uint64_t childHandle = child["Handle"].as<uint64_t>();
						relationshipComponent.Children.push_back(childHandle);
					}
				}

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& transform = deserializedEntity.GetComponent<TransformComponent>();
					transform.Translation = transformComponent["Position"].as<glm::vec3>();
					const auto& rotationNode = transformComponent["Rotation"];
					// Rotations used to be stored as quaternions
					if (rotationNode.size() == 4)
					{
						glm::quat rotation = transformComponent["Rotation"].as<glm::quat>();
						transform.Rotation = glm::eulerAngles(rotation);
					}
					else
					{
						VA_CORE_ASSERT(rotationNode.size() == 3);
						transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					}
					transform.Scale = transformComponent["Scale"].as<glm::vec3>();

					VA_CORE_INFO("  Entity Transform:");
					VA_CORE_INFO("    Translation: {0}, {1}, {2}", transform.Translation.x, transform.Translation.y, transform.Translation.z);
					VA_CORE_INFO("    Rotation: {0}, {1}, {2}", transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
					VA_CORE_INFO("    Scale: {0}, {1}, {2}", transform.Scale.x, transform.Scale.y, transform.Scale.z);
				}

				auto scriptComponent = entity["ScriptComponent"];
				if (scriptComponent)
				{
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent)
				{
					auto& component = deserializedEntity.AddComponent<MeshComponent>();

					AssetHandle assetHandle = 0;
					if (meshComponent["AssetPath"])
						assetHandle = AssetManager::GetAssetHandleFromFilePath(meshComponent["AssetPath"].as<std::string>());
					else
						assetHandle = meshComponent["AssetID"].as<uint64_t>();

					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						component.Mesh = Ref<Mesh>::Create(AssetManager::GetAsset<MeshAsset>(assetHandle));
					}
					else
					{
						component.Mesh = Ref<Asset>::Create().As<Mesh>();
						component.Mesh->SetFlag(AssetFlag::Missing, true);

						std::string filepath = meshComponent["AssetPath"] ? meshComponent["AssetPath"].as<std::string>() : "";
						if (filepath.empty())
						{
							VA_CORE_ERROR("Tried to load non-existent mesh in Entity: {0}", (uint64_t)deserializedEntity.GetUUID());
						}
						else
						{
							VA_CORE_ERROR("Tried to load invalid mesh '{0}' in Entity {1}", filepath, (uint64_t)deserializedEntity.GetUUID());
							component.Mesh->SetFlag(AssetFlag::Invalid, true);
						}
					}
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& component = deserializedEntity.AddComponent<CameraComponent>();
					const auto& cameraNode = cameraComponent["Camera"];

					component.Camera = SceneCamera();
					auto& camera = component.Camera;

					if (cameraNode.IsMap())
					{
						if (cameraNode["ProjectionType"])
							camera.SetProjectionType((SceneCamera::ProjectionType)cameraNode["ProjectionType"].as<int>());
						if (cameraNode["PerspectiveFOV"])
							camera.SetPerspectiveVerticalFOV(cameraNode["PerspectiveFOV"].as<float>());
						if (cameraNode["PerspectiveNear"])
							camera.SetPerspectiveNearClip(cameraNode["PerspectiveNear"].as<float>());
						if (cameraNode["PerspectiveFar"])
							camera.SetPerspectiveFarClip(cameraNode["PerspectiveFar"].as<float>());
						if (cameraNode["OrthographicSize"])
							camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>());
						if (cameraNode["OrthographicNear"])
							camera.SetOrthographicNearClip(cameraNode["OrthographicNear"].as<float>());
						if (cameraNode["OrthographicFar"])
							camera.SetOrthographicFarClip(cameraNode["OrthographicFar"].as<float>());
					}

					component.Primary = cameraComponent["Primary"].as<bool>();
				}

				auto directionalLightComponent = entity["DirectionalLightComponent"];
				if (directionalLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<DirectionalLightComponent>();
					component.Radiance = directionalLightComponent["Radiance"].as<glm::vec3>();
					component.CastShadows = directionalLightComponent["CastShadows"].as<bool>();
					component.SoftShadows = directionalLightComponent["SoftShadows"].as<bool>();
					component.LightSize = directionalLightComponent["LightSize"].as<float>();
				}

				auto skyLightComponent = entity["SkyLightComponent"];
				if (skyLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<SkyLightComponent>();

					AssetHandle assetHandle = 0;
					if (skyLightComponent["EnvironmentAssetPath"])
						assetHandle = AssetManager::GetAssetHandleFromFilePath(skyLightComponent["EnvironmentAssetPath"].as<std::string>());
					else
						assetHandle = skyLightComponent["EnvironmentMap"].as<uint64_t>();

					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						component.SceneEnvironment = AssetManager::GetAsset<Environment>(assetHandle);
					}
					else
					{
						std::string filepath = meshComponent["EnvironmentAssetPath"] ? meshComponent["EnvironmentAssetPath"].as<std::string>() : "";
						if (filepath.empty())
							VA_CORE_ERROR("Tried to load non-existent environment map in Entity: {0}", (uint64_t)deserializedEntity.GetUUID());
						else
							VA_CORE_ERROR("Tried to load invalid environment map '{0}' in Entity {1}", filepath, (uint64_t)deserializedEntity.GetUUID());
					}

					component.Intensity = skyLightComponent["Intensity"].as<float>();
					component.Angle = skyLightComponent["Angle"].as<float>();
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto& component = deserializedEntity.AddComponent<SpriteRendererComponent>();
					component.Color = spriteRendererComponent["Color"].as<glm::vec4>();
					component.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
				}
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		VA_CORE_ASSERT(false);
		return false;
	}

}