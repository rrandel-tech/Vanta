#include "vapch.hpp"
#include "EditorCamera.hpp"

#include "Core/Input.hpp"

#include <SDL3/SDL.h>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Core/Application.hpp"
#include <imgui.h>

#define M_PI 3.14159f

namespace Vanta {

	EditorCamera::EditorCamera(const glm::mat4& projectionMatrix)
		: Camera(projectionMatrix)
	{
		m_Rotation = glm::vec3(90.0f, 0.0f, 0.0f);
		m_FocalPoint = glm::vec3(0.0f);

		glm::vec3 position = { -5, 5, 5 };
		m_Distance = glm::distance(position, m_FocalPoint);

		m_Yaw = 3.0f * (float)M_PI / 4.0f;
		m_Pitch = M_PI / 4.0f;

		UpdateCameraView();
	}

	void EditorCamera::CalculateYaw(float degrees) {
		//Check bounds with the max heading rate so that we aren't moving too fast
		constexpr float maxYawRate{ 0.12f };
		degrees = glm::clamp(degrees, -maxYawRate, maxYawRate);

		//This controls how the heading is changed if the camera is pointed straight up or down
		//The heading delta direction changes
		if (m_Pitch > glm::radians(90.f) && m_Pitch < glm::radians(270.f) || m_Pitch < glm::radians(-90.f) && m_Pitch > glm::radians(-270.f))
			m_Yaw -= degrees;
		else
			m_Yaw += degrees;

		//Check bounds for the camera heading
		if (m_Yaw > glm::radians(360.f))
			m_Yaw -= glm::radians(360.f);
		else if (m_Yaw < glm::radians(-360.f))
			m_Yaw += glm::radians(360.f);
	}

	void EditorCamera::CalculatePitch(float degrees) {
		//Check bounds with the max pitch rate so that we aren't moving too fast
		constexpr float maxPitchRate{ 0.12f };
		degrees = glm::clamp(degrees, -maxPitchRate, maxPitchRate);

		m_Pitch += degrees;

		//Check bounds for the camera pitch
		if (m_Pitch > glm::radians(360.f))
			m_Pitch -= glm::radians(360.f);
		else if (m_Pitch < -glm::radians(360.f))
			m_Pitch += glm::radians(360.f);


	}

	void EditorCamera::UpdateCameraView()
	{
		if (m_CameraMode == CameraMode::FLYCAM)
		{
			m_Rotation = glm::normalize(m_FocalPoint - m_Position);
			m_RightDirection = glm::cross(m_Rotation, glm::vec3{ 0.f, 1.f, 0.f });

			m_Rotation = glm::rotate(glm::normalize(glm::cross(glm::angleAxis(m_Pitch, m_RightDirection),
				glm::angleAxis(m_Yaw, glm::vec3{ 0.f, 1.f, 0.f }))), m_Rotation);

			m_Position += m_CameraPositionDelta;

			//damping for smooth camera
			m_Yaw *= 0.5f;
			m_Pitch *= 0.5f;
			m_CameraPositionDelta *= 0.8f;

			m_FocalPoint = m_Position + m_Rotation;
			m_ViewMatrix = glm::lookAt(m_Position, m_FocalPoint, glm::vec3{ 0.f, 1.f, 0.f });
		}
		else
		{
			m_Position = CalculatePosition();

			glm::quat orientation = GetOrientation();
			m_Rotation = glm::eulerAngles(orientation) * (180.0f / (float)M_PI);
			m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
			m_ViewMatrix = glm::inverse(m_ViewMatrix);
		}
	}

	void EditorCamera::Focus(const glm::vec3& focusPoint)
	{
		m_FocalPoint = focusPoint;
		if (m_Distance > m_MinFocusDistance)
		{
			float distance = m_Distance - m_MinFocusDistance;
			MouseZoom(distance / ZoomSpeed());
			m_CameraMode = CameraMode::ARCBALL;
			UpdateCameraView();
		}
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min(float(m_ViewportWidth) / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(float(m_ViewportHeight) / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void DisableMouse()
	{
		SDL_Window* window = static_cast<SDL_Window*>(Application::Get().GetWindow().GetNativeWindow());
		SDL_SetWindowRelativeMouseMode(window, true);
		SDL_SetWindowMouseGrab(window, true);

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}

	void EnableMouse()
	{
		SDL_Window* window = static_cast<SDL_Window*>(Application::Get().GetWindow().GetNativeWindow());
		SDL_SetWindowRelativeMouseMode(window, false);
		SDL_SetWindowMouseGrab(window, false);

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}

	void EditorCamera::OnUpdate(Timestep ts)
	{
		const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
		const glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
		m_InitialMousePosition = mouse;

		if (m_IsActive)
		{
			m_CameraMode = CameraMode::ARCBALL;
			if (Input::IsKeyPressed(KeyCode::LeftAlt))
			{
				if (Input::IsMouseButtonPressed(MouseButton::Middle))
				{
					DisableMouse();
					MousePan(delta);
				}
				else if (Input::IsMouseButtonPressed(MouseButton::Left))
				{
					DisableMouse();
					MouseRotate(delta);
				}
				else if (Input::IsMouseButtonPressed(MouseButton::Right))
				{
					DisableMouse();
					MouseZoom(delta.x + delta.y);
				}
				else
					EnableMouse();
			}
			else if (Input::IsMouseButtonPressed(MouseButton::Right) && false)
			{
				m_CameraMode = CameraMode::FLYCAM;
				DisableMouse();

				if (Input::IsKeyPressed(KeyCode::Q))
					m_CameraPositionDelta -= ts.GetMilliseconds() * m_Speed * glm::vec3{ 0.f, 1.f, 0.f };
				if (Input::IsKeyPressed(KeyCode::E))
					m_CameraPositionDelta += ts.GetMilliseconds() * m_Speed * glm::vec3{ 0.f, 1.f, 0.f };

				if (Input::IsKeyPressed(KeyCode::S))
					m_CameraPositionDelta -= ts.GetMilliseconds() * m_Speed * m_Rotation;
				if (Input::IsKeyPressed(KeyCode::W))
					m_CameraPositionDelta += ts.GetMilliseconds() * m_Speed * m_Rotation;

				if (Input::IsKeyPressed(KeyCode::A))
					m_CameraPositionDelta -= ts.GetMilliseconds() * m_Speed * m_RightDirection;
				if (Input::IsKeyPressed(KeyCode::D))
					m_CameraPositionDelta += ts.GetMilliseconds() * m_Speed * m_RightDirection;

				CalculateYaw(-delta.x);
				CalculatePitch(-delta.y);
			}
			else
			{
				EnableMouse();
			}
		}
		m_InitialMousePosition = mouse;

		UpdateCameraView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(VA_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
		dispatcher.Dispatch<KeyReleasedEvent>(VA_BIND_EVENT_FN(EditorCamera::OnKeyReleased));
		dispatcher.Dispatch<KeyPressedEvent>(VA_BIND_EVENT_FN(EditorCamera::OnKeyPressed));
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		if (m_CameraMode == CameraMode::ARCBALL)
		{
			MouseZoom(e.GetYOffset() * 0.1f);
			UpdateCameraView();
		}
		else if (Input::IsMouseButtonPressed(MouseButton::Right))
		{
			e.GetYOffset() > 0 ? m_Speed += 0.3f * m_Speed : m_Speed -= 0.3f * m_Speed;
			m_Speed = std::clamp(m_Speed, 0.0005f, 2.f);
		}

		return false;
	}

	bool EditorCamera::OnKeyPressed(KeyPressedEvent& e)
	{
		if (m_LastSpeed == 0.f)
		{
			if (e.GetKeyCode() == KeyCode::LeftShift)
			{
				m_LastSpeed = m_Speed;
				m_Speed *= 2 - std::log(m_Speed);
			}
			if (e.GetKeyCode() == KeyCode::LeftControl)
			{
				m_LastSpeed = m_Speed;
				m_Speed /= 2 - std::log(m_Speed);
			}

			m_Speed = std::clamp(m_Speed, 0.0005f, 2.f);
		}
		return true;
	}

	bool EditorCamera::OnKeyReleased(KeyReleasedEvent& e)
	{
		if (e.GetKeyCode() == KeyCode::LeftShift || e.GetKeyCode() == KeyCode::LeftControl)
		{

			if (m_LastSpeed != 0.f)
			{
				m_Speed = m_LastSpeed;

				m_LastSpeed = 0.f;
			}

			m_Speed = std::clamp(m_Speed, 0.0005f, 2.f);
		}
		return true;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	glm::vec3 EditorCamera::GetUpDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetForwardDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition()
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}
}