#pragma once

#include "Renderer/Camera.hpp"
#include "Core/TimeStep.hpp"
#include "Events/KeyEvent.hpp"
#include "Events/MouseEvent.hpp"

namespace Vanta {

	enum class CameraMode
	{
		NONE, FLYCAM, ARCBALL
	};

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(const glm::mat4& projectionMatrix);

		void Focus(const glm::vec3& focusPoint);
		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		bool IsActive() const { return m_IsActive; }
		void SetActive(bool active) { m_IsActive = active; }

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		inline void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return m_ProjectionMatrix * m_ViewMatrix; }

		glm::vec3 GetUpDirection();
		glm::vec3 GetRightDirection();
		glm::vec3 GetForwardDirection();

		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }
		float& GetCameraSpeed() { return m_Speed; }
		float GetCameraSpeed() const { return m_Speed; }
	private:
		void CalculateYaw(float degrees);
		void CalculatePitch(float degrees);
		void UpdateCameraView();

		bool OnMouseScroll(MouseScrolledEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnKeyReleased(KeyReleasedEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition();

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position, m_Rotation, m_FocalPoint;

		bool m_IsActive = false;
		bool m_Panning, m_Rotating;
		glm::vec2 m_InitialMousePosition {};
		glm::vec3 m_InitialFocalPoint, m_InitialRotation;

		float m_Distance;
		float m_Speed {0.005f};
		float m_LastSpeed = 0.f;

		float m_Pitch, m_Yaw;
		glm::vec3 m_CameraPositionDelta;
		glm::vec3 m_RightDirection {};

		bool m_IsAllowed = false;

		CameraMode m_CameraMode { CameraMode::NONE };

		float m_MinFocusDistance = 100.0f;

		uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
	};

}