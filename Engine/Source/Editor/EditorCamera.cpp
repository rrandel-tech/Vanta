#include "vapch.hpp"
#include "EditorCamera.hpp"

#include "Core/Input.hpp"

#include "ImGui/ImGui.hpp"

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
		m_FocalPoint = glm::vec3(0.0f);

		glm::vec3 position = { -5, 5, 5 };
		m_Distance = glm::distance(position, m_FocalPoint);

		m_Yaw = 3.0f * (float)M_PI / 4.0f;
		m_Pitch = M_PI / 4.0f;

		m_Position = CalculatePosition();
		const glm::quat orientation = GetOrientation();
		m_WorldRotation = glm::eulerAngles(orientation) * (180.0f / (float)M_PI);
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	static void DisableMouse()
	{
		Input::SetCursorMode(CursorMode::Locked);
		UI::SetMouseEnabled(false);
	}
	static void EnableMouse()
	{
		Input::SetCursorMode(CursorMode::Normal);
		UI::SetMouseEnabled(true);
	}

	void EditorCamera::OnUpdate(const Timestep ts)
	{
		const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
		const glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.002f;

		if (m_IsActive)
		{
			if (Input::IsMouseButtonPressed(MouseButton::Right) && !Input::IsKeyPressed(KeyCode::LeftAlt))
			{
				m_CameraMode = CameraMode::FLYCAM;
				DisableMouse();
				const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

				if (Input::IsKeyPressed(KeyCode::Q))
					m_PositionDelta -= ts.GetMilliseconds() * m_Speed * glm::vec3{ 0.f, yawSign, 0.f };
				if (Input::IsKeyPressed(KeyCode::E))
					m_PositionDelta += ts.GetMilliseconds() * m_Speed * glm::vec3{ 0.f, yawSign, 0.f };
				if (Input::IsKeyPressed(KeyCode::S))
					m_PositionDelta -= ts.GetMilliseconds() * m_Speed * m_WorldRotation;
				if (Input::IsKeyPressed(KeyCode::W))
					m_PositionDelta += ts.GetMilliseconds() * m_Speed * m_WorldRotation;
				if (Input::IsKeyPressed(KeyCode::A))
					m_PositionDelta -= ts.GetMilliseconds() * m_Speed * m_RightDirection;
				if (Input::IsKeyPressed(KeyCode::D))
					m_PositionDelta += ts.GetMilliseconds() * m_Speed * m_RightDirection;

				constexpr float maxRate{ 0.12f };
				m_YawDelta += glm::clamp(yawSign * delta.x, -maxRate, maxRate);
				m_PitchDelta += glm::clamp(delta.y, -maxRate, maxRate);

				m_RightDirection = glm::cross(m_WorldRotation, glm::vec3{ 0.f, yawSign, 0.f });

				m_WorldRotation = glm::rotate(glm::normalize(glm::cross(glm::angleAxis(-m_PitchDelta, m_RightDirection),
					glm::angleAxis(-m_YawDelta, glm::vec3{ 0.f, yawSign, 0.f }))), m_WorldRotation);
			}
			else if (Input::IsKeyPressed(KeyCode::LeftAlt))
			{
				m_CameraMode = CameraMode::ARCBALL;

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
			else
			{
				EnableMouse();
			}
		}
		m_InitialMousePosition = mouse;

		m_Position += m_PositionDelta;
		m_Yaw += m_YawDelta;
		m_Pitch += m_PitchDelta;

		if (m_CameraMode == CameraMode::ARCBALL)
			m_Position = CalculatePosition();

		UpdateCameraView();
	}

	void EditorCamera::UpdateCameraView()
	{
		const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

		// Extra step to handle the problem when the camera direction is the same as the up vector
		const float cosAngle = glm::dot(GetForwardDirection(), GetUpDirection());
		if (cosAngle * yawSign > 0.99f)
			m_PitchDelta = 0.f;

		const glm::vec3 lookAt = m_Position + GetForwardDirection();
		m_WorldRotation = glm::normalize(m_FocalPoint - m_Position);
		m_FocalPoint = m_Position + GetForwardDirection() * m_Distance;
		m_Distance = glm::distance(m_Position, m_FocalPoint);
		m_ViewMatrix = glm::lookAt(m_Position, lookAt, glm::vec3{ 0.f, yawSign, 0.f });

		//damping for smooth camera
		m_YawDelta *= 0.6f;
		m_PitchDelta *= 0.6f;
		m_PositionDelta *= 0.8f;
	}

	void EditorCamera::Focus(const glm::vec3& focusPoint)
	{
		m_FocalPoint = focusPoint;
		if (m_Distance > m_MinFocusDistance)
		{
			const float distance = m_Distance - m_MinFocusDistance;
			MouseZoom(distance / ZoomSpeed());
			m_CameraMode = CameraMode::ARCBALL;
		}
		m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
		UpdateCameraView();
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		const float x = std::min(float(m_ViewportWidth) / 1000.0f, 2.4f); // max = 2.4f
		const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		const float y = std::min(float(m_ViewportHeight) / 1000.0f, 2.4f); // max = 2.4f
		const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

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

	void EditorCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return OnMouseScroll(e); });
		dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& e) { return OnKeyReleased(e); });
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressed(e); });
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		if (m_IsActive)
		{
			if (Input::IsMouseButtonPressed(MouseButton::Right))
			{
				e.GetYOffset() > 0 ? m_Speed += 0.3f * m_Speed : m_Speed -= 0.3f * m_Speed;
				m_Speed = std::clamp(m_Speed, 0.0005f, 2.f);
			}
			else
			{
				MouseZoom(e.GetYOffset() * 0.1f);
				UpdateCameraView();
			}
		}

		return false;
	}

	bool EditorCamera::OnKeyPressed(KeyPressedEvent& e)
	{
		if (m_LastSpeed == 0.0f)
		{
			if (e.GetKeyCode() == KeyCode::LeftShift)
			{
				m_LastSpeed = m_Speed;
				m_Speed *= 2.0f - glm::log(m_Speed);
			}
			if (e.GetKeyCode() == KeyCode::LeftControl)
			{
				m_LastSpeed = m_Speed;
				m_Speed /= 2.0f - glm::log(m_Speed);
			}

			m_Speed = glm::clamp(m_Speed, 0.0005f, 2.0f);
		}
		return true;
	}

	bool EditorCamera::OnKeyReleased(KeyReleasedEvent& e)
	{
		if (e.GetKeyCode() == KeyCode::LeftShift || e.GetKeyCode() == KeyCode::LeftControl)
		{
			if (m_LastSpeed != 0.0f)
			{
				m_Speed = m_LastSpeed;
				m_LastSpeed = 0.0f;
			}
			m_Speed = glm::clamp(m_Speed, 0.0005f, 2.0f);
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
		const float yawSign = GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
		m_YawDelta += yawSign * delta.x * RotationSpeed();
		m_PitchDelta += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
		const glm::vec3 forwardDir = GetForwardDirection();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += forwardDir;
			m_Distance = 1.0f;
		}
		m_PositionDelta += delta * ZoomSpeed() * forwardDir;
	}

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.f, 0.f, 0.f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance + m_PositionDelta;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch - m_PitchDelta, -m_Yaw - m_YawDelta, 0.0f));
	}
}