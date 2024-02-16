#pragma once

#include "SceneCommon.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Omni {

	enum class CameraProjectionType {
		PROJECTION_3D,
		PROJECTION_2D
	};

	// Interface for Camera2D and Camera3D
	class OMNIFORCE_API Camera
	{
	public:
		virtual const glm::mat4& GetViewProjectionMatrix() const = 0;
		virtual const glm::mat4& GetProjectionMatrix() const = 0;
		virtual const glm::mat4& GetViewMatrix() const = 0;
		virtual void CalculateMatrices() = 0;

		CameraProjectionType GetType() const { return m_Type; }
		float32 GetNearClip() const { return m_ZNear; }
		float32 GetFarClip() const { return m_ZFar; }
		glm::vec3 GetPosition() const { return m_Position; }

		void SetType(CameraProjectionType type) { m_Type = type; }
		void SetNearClip(float32 near) { m_ZNear = near; }
		void SetFarClip(float32 far) { m_ZFar = far; }
		void SetPosition(glm::vec3 position) { m_Position = position; }
		virtual void SetAspectRatio(float32 ratio) = 0;

	protected:
		Camera(CameraProjectionType type) : m_Type(type) {}
		CameraProjectionType m_Type;
		float32 m_AspectRatio = 1.0f;
		float32 m_ZNear = 0.0f;
		float32 m_ZFar = 1.0f;
		glm::vec3 m_Position = glm::vec3(0.0f);
	};

	// 2D camera, which uses orthographic matrix as projection matrix.
	// Should be used in 2D games or to render GUI.
	class OMNIFORCE_API Camera2D : public Camera
	{
	public:
		Camera2D();

		// Matrix getters
		const glm::mat4& GetViewProjectionMatrix() const override { return m_ViewProjectionMatrix; };
		const glm::mat4& GetProjectionMatrix() const override { return m_ProjectionMatrix; };
		const glm::mat4& GetViewMatrix() const override { return m_ViewMatrix; };

		void SetProjection(float left, float right, float bottom, float top, float zNear = 0.0f, float zFar = 1.0f);
		void SetPosition(glm::vec3 position);
		void SetRotation(float rotation);
		void SetScale(float32 scale);
		void SetAspectRatio(float32 ratio) override;

		float32 GetRotation() const { return m_Rotation; }
		float32 GetScale() const { return m_Scale; }

		void CalculateMatrices() override;

	protected:
		glm::mat4 m_ViewProjectionMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;

		float32 m_Rotation;
		float32 m_Scale;
		float32 m_AspectRatio;
	};

	// 3D camera, which uses perspective matrix as projection matrix.
	// Should be used in any kind of 3D games, because it makes engine be able
	// to display depth correctly.
	class OMNIFORCE_API Camera3D : public Camera
	{
	public:
		Camera3D();

		// Matrix getters
		const glm::mat4& GetViewProjectionMatrix() const override { return m_ViewProjectionMatrix; };
		const glm::mat4& GetProjectionMatrix() const override { return m_ProjectionMatrix; };
		const glm::mat4& GetViewMatrix() const override { return m_ViewMatrix; };

		// Setters for common camera data
		void SetProjection(float32 fov, float32 ratio, float32 zNear, float32 zFar);
		void SetPosition(glm::vec3 position);
		void SetRotation(float32 yaw, float32 pitch, float32 roll = 0.0f);
		void SetFOV(float32 fov);
		void SetAspectRatio(float32 ratio) override;

		// Getters for common camera data
		float32 GetYaw() const { return m_Yaw; };
		float32 GetPitch() const { return m_Pitch; };
		float32 GetRoll() const { return m_Roll; };
		float32 GetFOV() const { return m_FieldOfView; }

		// Special methods to move / rotate 3D camera
		void LookAt(glm::vec3 at);
		void Rotate(float32 yaw_offset, float pitch_offset, float roll_offset, bool lock_pitch);
		void Move(glm::vec3 direction);


		void CalculateMatrices() override;

	private:
		void CalculateVectors();

	protected:
		// Matrices
		glm::mat4 m_ViewProjectionMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;

		// Common Camera data
		float32 m_Yaw;
		float32 m_Pitch;
		float32 m_Roll;

		float32 m_FieldOfView;

		// Other math data, used to calculate view matrix
		// Represents direction (unit vector) at which camera looks.
		glm::vec3 m_FrontVector;
		glm::vec3 m_UpVector;
		glm::vec3 m_RightVector;
	};

}