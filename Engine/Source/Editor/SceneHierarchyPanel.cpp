#include "vapch.hpp"
#include "SceneHierarchyPanel.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "Core/Application.hpp"
#include "Math/Math.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/MeshFactory.hpp"

#include "Asset/AssetManager.hpp"

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ImGui/ImGui.hpp"
#include "Renderer/Renderer.hpp"

// TODO:
// - Eventually change imgui node IDs to be entity/asset GUID

namespace Vanta {

	glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix);

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
		: m_Context(context)
	{
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
		if (m_SelectionContext && false)
		{
			// Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = m_SelectionContext.GetUUID();
			if (entityMap.find(selectedEntityID) != entityMap.end())
				m_SelectionContext = entityMap.at(selectedEntityID);
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		m_SelectionContext = entity;

		if (m_SelectionChangedCallback)
			m_SelectionChangedCallback(m_SelectionContext);
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");
		ImRect windowRect = { ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax() };

		if (m_Context)
		{
			uint32_t entityCount = 0, meshCount = 0;
			m_Context->m_Registry.each([&](auto entity)
			{
				Entity e(entity, m_Context.Raw());
				if (e.HasComponent<IDComponent>() && e.GetParentUUID() == 0)
					DrawEntityNode(e);
			});

			if (ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetCurrentWindow()->ID))
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

				if (payload)
				{
					UUID droppedHandle = *((UUID*)payload->Data);
					Entity e = m_Context->FindEntityByUUID(droppedHandle);
					m_Context->UnparentEntity(e);
				}

				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Empty Entity"))
					{
						auto newEntity = m_Context->CreateEntity("Empty Entity");
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Camera"))
					{
						auto newEntity = m_Context->CreateEntity("Camera");
						newEntity.AddComponent<CameraComponent>();
						SetSelected(newEntity);
					}
					if (ImGui::BeginMenu("3D"))
					{
						if (ImGui::MenuItem("Cube"))
						{
							auto newEntity = m_Context->CreateEntity("Cube");
							Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>("assets/meshes/Default/Cube.fbx");
							newEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshAsset));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Sphere"))
						{
							auto newEntity = m_Context->CreateEntity("Sphere");
							Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>("assets/meshes/Default/Sphere.fbx");
							newEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshAsset));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Capsule"))
						{
							auto newEntity = m_Context->CreateEntity("Capsule");
							Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>("assets/meshes/Default/Capsule.fbx");
							newEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshAsset));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Cylinder"))
						{
							auto newEntity = m_Context->CreateEntity("Cylinder");
							Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>("assets/meshes/Default/Cylinder.fbx");
							newEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshAsset));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Torus"))
						{
							auto newEntity = m_Context->CreateEntity("Torus");
							Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>("assets/meshes/Default/Torus.fbx");
							newEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshAsset));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Plane"))
						{
							auto newEntity = m_Context->CreateEntity("Plane");
							Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>("assets/meshes/Default/Plane.fbx");
							newEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshAsset));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Cone"))
						{
							auto newEntity = m_Context->CreateEntity("Cone");
							Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>("assets/meshes/Default/Cone.fbx");
							newEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshAsset));
							SetSelected(newEntity);
						}
						ImGui::EndMenu();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Directional Light"))
					{
						auto newEntity = m_Context->CreateEntity("Directional Light");
						newEntity.AddComponent<DirectionalLightComponent>();
						newEntity.GetComponent<TransformComponent>().Rotation = glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f });
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Sky Light"))
					{
						auto newEntity = m_Context->CreateEntity("Sky Light");
						newEntity.AddComponent<SkyLightComponent>();
						SetSelected(newEntity);
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::End();

			ImGui::Begin("Properties");

			if (m_SelectionContext)
				DrawComponents(m_SelectionContext);
		}
		ImGui::End();

#if TODO
		ImGui::Begin("Mesh Debug");
		if (ImGui::CollapsingHeader(mesh->m_FilePath.c_str()))
		{
			if (mesh->m_IsAnimated)
			{
				if (ImGui::CollapsingHeader("Animation"))
				{
					if (ImGui::Button(mesh->m_AnimationPlaying ? "Pause" : "Play"))
						mesh->m_AnimationPlaying = !mesh->m_AnimationPlaying;

					ImGui::SliderFloat("##AnimationTime", &mesh->m_AnimationTime, 0.0f, (float)mesh->m_Scene->mAnimations[0]->mDuration);
					ImGui::DragFloat("Time Scale", &mesh->m_TimeMultiplier, 0.05f, 0.0f, 10.0f);
				}
			}
		}
		ImGui::End();
#endif
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const char* name = "Unnamed Entity";
		if (entity.HasComponent<TagComponent>())
			name = entity.GetComponent<TagComponent>().Tag.c_str();

		ImGuiTreeNodeFlags flags = (entity == m_SelectionContext ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		if (entity.Children().empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		// TODO: This should probably be a function that checks that the entities components are valid
		bool missingMesh = entity.HasComponent<MeshComponent>() && (entity.GetComponent<MeshComponent>().Mesh && entity.GetComponent<MeshComponent>().Mesh->IsFlagSet(AssetFlag::Missing));
		if (missingMesh)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.3f, 1.0f));

		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, name);
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
			if (m_SelectionChangedCallback)
				m_SelectionChangedCallback(m_SelectionContext);
		}

		if (missingMesh)
			ImGui::PopStyleColor();

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			UUID entityId = entity.GetUUID();
			ImGui::Text(entity.GetComponent<TagComponent>().Tag.c_str());
			ImGui::SetDragDropPayload("scene_entity_hierarchy", &entityId, sizeof(UUID));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

			if (payload)
			{
				UUID droppedHandle = *((UUID*)payload->Data);
				Entity e = m_Context->FindEntityByUUID(droppedHandle);
				m_Context->ParentEntity(e, entity);
			}

			ImGui::EndDragDropTarget();
		}

		if (opened)
		{
			for (auto child : entity.Children())
			{
				Entity e = m_Context->FindEntityByUUID(child);
				if (e)
					DrawEntityNode(e);
			}

			ImGui::TreePop();
		}

		// Defer deletion until end of node UI
		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (entity == m_SelectionContext)
				m_SelectionContext = {};

			m_EntityDeletedCallback(entity);
		}
	}



	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, bool canBeRemoved = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			// NOTE:
			//	This fixes an issue where the first "+" button would display the "Remove" buttons for ALL components on an Entity.
			//	This is due to ImGui::TreeNodeEx only pushing the id for it's children if it's actually open
			ImGui::PushID((void*)typeid(T).hash_code());
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, name.c_str());
			bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
			ImGui::PopStyleVar();

			bool resetValues = false;
			bool removeComponent = false;

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }) || right_clicked)
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Reset"))
					resetValues = true;

				if (canBeRemoved)
				{
					if (ImGui::MenuItem("Remove component"))
						removeComponent = true;
				}

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent || resetValues)
				entity.RemoveComponent<T>();

			if (resetValues)
				entity.AddComponent<T>();

			ImGui::PopID();
		}
	}

	static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		bool modified = false;

		const ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			values.x = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		return modified;
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		ImGui::AlignTextToFramePadding();

		auto id = entity.GetComponent<IDComponent>().ID;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, 256);
			memcpy(buffer, tag.c_str(), tag.length());
			ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);
			if (ImGui::InputText("##Tag", buffer, 256))
			{
				tag = std::string(buffer);
			}
			ImGui::PopItemWidth();
		}

		// ID
		ImGui::SameLine();
		ImGui::TextDisabled("%llx", (uint64_t)id);
		float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 textSize = ImGui::CalcTextSize("Add Component");
		ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.y));
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponentPanel");

		if (ImGui::BeginPopup("AddComponentPanel"))
		{
			if (!m_SelectionContext.HasComponent<CameraComponent>())
			{
				if (ImGui::Button("Camera"))
				{
					m_SelectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<MeshComponent>())
			{
				if (ImGui::Button("Mesh"))
				{
					MeshComponent& component = m_SelectionContext.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<DirectionalLightComponent>())
			{
				if (ImGui::Button("Directional Light"))
				{
					m_SelectionContext.AddComponent<DirectionalLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SkyLightComponent>())
			{
				if (ImGui::Button("Sky Light"))
				{
					m_SelectionContext.AddComponent<SkyLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<ScriptComponent>())
			{
				if (ImGui::Button("Script"))
				{
					m_SelectionContext.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SpriteRendererComponent>())
			{
				if (ImGui::Button("Sprite Renderer"))
				{
					m_SelectionContext.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
		{
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component.Scale, 1.0f);
		}, false);

		DrawComponent<MeshComponent>("Mesh", entity, [&](MeshComponent& mc)
		{
			UI::BeginPropertyGrid();

			Ref<MeshAsset> meshAsset;

			if (mc.Mesh && mc.Mesh->IsValid())
				meshAsset = mc.Mesh->GetMeshAsset();

			if (UI::PropertyAssetReference("Mesh", meshAsset))
			{
				mc.Mesh = meshAsset ? Ref<Mesh>::Create(meshAsset) : nullptr;
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& cc)
		{
			UI::BeginPropertyGrid();

			// Projection Type
			const char* projTypeStrings[] = { "Perspective", "Orthographic" };
			int currentProj = (int)cc.Camera.GetProjectionType();
			if (UI::PropertyDropdown("Projection", projTypeStrings, 2, &currentProj))
			{
				cc.Camera.SetProjectionType((SceneCamera::ProjectionType)currentProj);
			}

			// Perspective parameters
			if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFOV = cc.Camera.GetPerspectiveVerticalFOV();
				if (UI::Property("Vertical FOV", verticalFOV))
					cc.Camera.SetPerspectiveVerticalFOV(verticalFOV);

				float nearClip = cc.Camera.GetPerspectiveNearClip();
				if (UI::Property("Near Clip", nearClip))
					cc.Camera.SetPerspectiveNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetPerspectiveFarClip();
				if (UI::Property("Far Clip", farClip))
					cc.Camera.SetPerspectiveFarClip(farClip);
			}

			// Orthographic parameters
			else if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = cc.Camera.GetOrthographicSize();
				if (UI::Property("Size", orthoSize))
					cc.Camera.SetOrthographicSize(orthoSize);

				float nearClip = cc.Camera.GetOrthographicNearClip();
				if (UI::Property("Near Clip", nearClip))
					cc.Camera.SetOrthographicNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetOrthographicFarClip();
				if (UI::Property("Far Clip", farClip))
					cc.Camera.SetOrthographicFarClip(farClip);
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& mc)
		{
		});

		DrawComponent<DirectionalLightComponent>("Directional Light", entity, [](DirectionalLightComponent& dlc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyColor("Radiance", dlc.Radiance);
			UI::Property("Intensity", dlc.Intensity);
			UI::Property("Cast Shadows", dlc.CastShadows);
			UI::Property("Soft Shadows", dlc.SoftShadows);
			UI::Property("Source Size", dlc.LightSize);
			UI::EndPropertyGrid();
		});

		DrawComponent<SkyLightComponent>("Sky Light", entity, [](SkyLightComponent& slc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyAssetReference("Environment Map", slc.SceneEnvironment);
			UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
			ImGui::Separator();
			UI::Property("Dynamic Sky", slc.DynamicSky);
			if (slc.DynamicSky)
			{
				bool changed = UI::Property("Turbidity", slc.TurbidityAzimuthInclination.x, 0.01f);
				changed |= UI::Property("Azimuth", slc.TurbidityAzimuthInclination.y, 0.01f);
				changed |= UI::Property("Inclination", slc.TurbidityAzimuthInclination.z, 0.01f);
				if (changed)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(slc.TurbidityAzimuthInclination.x, slc.TurbidityAzimuthInclination.y, slc.TurbidityAzimuthInclination.z);
					slc.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
				}
			}
			UI::EndPropertyGrid();
		});

		DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
		{
		});

	}

}