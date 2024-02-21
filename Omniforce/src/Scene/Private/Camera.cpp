#include "../Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Omni {

	// =======================
	// Camera2D Implementation
	Camera2D::Camera2D()
		: Camera(CameraProjectionType::PROJECTION_2D)
	{
		m_ViewProjectionMatrix = glm::mat4(1.0f);
		m_ProjectionMatrix = glm::mat4(1.0f);
		m_ViewMatrix = glm::mat4(1.0f);
		m_Rotation = 0.0f;
		m_Position = glm::vec3(0.0f);
		m_Scale = 1.0f;
	}

	void Camera2D::SetProjection(float left, float right, float bottom, float top, float zNear /*= 0.0f*/, float zFar /*= 1.0f*/)
	{
		m_ZNear = zNear;
		m_ZFar = zFar;
		m_AspectRatio = (std::abs(left) + std::abs(right)) / (std::abs(bottom) + std::abs(top));

		m_ProjectionMatrix = glm::ortho(left, right, bottom, top, zNear, zFar);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void Camera2D::SetPosition(glm::vec3 position)
	{
		m_Position = position;
	}

	void Camera2D::SetRotation(float rotation)
	{
		m_Rotation = rotation;
	}

	void Camera2D::SetScale(float32 scale)
	{
		m_Scale = scale;
		SetProjection(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, m_ZNear, m_ZFar);
	}

	void Camera2D::SetAspectRatio(float32 ratio)
	{
		m_AspectRatio = ratio;
		SetProjection(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, m_ZNear, m_ZFar);
	}

	void Camera2D::CalculateMatrices()
	{
		m_ViewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), m_Position) * glm::rotate(glm::mat4(1.0f), m_Rotation, glm::vec3(0.0f, 0.0f, 1.0f)));
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	// =======================
	// Camera3D Implementation	
	Camera3D::Camera3D()
		: Camera(CameraProjectionType::PROJECTION_3D)
	{
		m_ViewProjectionMatrix = glm::mat4(1.0f);
		m_ProjectionMatrix = glm::mat4(1.0f);
		m_ViewMatrix = glm::mat4(1.0f);
		m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
		m_Yaw = -90.0f;
		m_Pitch = 0.0f;
		m_Roll = 0.0f;
		m_ZNear = 0.001f;
		m_ZFar = 1000.0f;
		m_AspectRatio = 16.0f / 9.0f;
		m_FieldOfView = glm::radians(60.0f);

		CalculateVectors();
		SetProjection(m_FieldOfView, m_AspectRatio, m_ZNear, m_ZFar);
		CalculateMatrices();
	}

	void Camera3D::SetProjection(float fov, float ratio, float zNear, float zFar)
	{
		m_FieldOfView = fov;
		m_AspectRatio = ratio;
		m_ZNear = zNear;
		m_ZFar = zFar;

		m_ProjectionMatrix = glm::perspective(fov, ratio, zNear, zFar);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void Camera3D::SetPosition(glm::vec3 position)
	{
		m_Position = position;
	}

	void Camera3D::SetRotation(float yaw, float pitch, float roll /*= 0.0f*/)
	{
		m_Yaw = yaw;
		m_Pitch = pitch;

		CalculateVectors();
		CalculateMatrices();
	}

	void Camera3D::SetFOV(float32 fov)
	{
		m_FieldOfView = fov;
		SetProjection(m_FieldOfView, m_AspectRatio, m_ZNear, m_ZFar);
	}

	void Camera3D::SetAspectRatio(float32 ratio)
	{
		m_AspectRatio = ratio;
		SetProjection(m_FieldOfView, m_AspectRatio, m_ZNear, m_ZFar);
	}

	void Camera3D::LookAt(glm::vec3 at)
	{
		m_ViewMatrix = glm::lookAt(m_Position, at, m_UpVector);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void Camera3D::Rotate(float yawOffset, float pitchOffset, float rollOffset, bool lockPitch)
	{
		m_Yaw += yawOffset;

		// If pitch is locked (must be in -89.0f to 89.0f range) AND pitch is less than -89.0f OR greater than 89.0f
		if (lockPitch)
		{
			if (m_Pitch + pitchOffset> 89.0f)
				m_Pitch = 89.0f;
			else if (m_Pitch + pitchOffset < -89.0f)
				m_Pitch = -89.0f;
			else
				m_Pitch += pitchOffset;
		}
		else {
			m_Pitch += pitchOffset;
		}

		m_Roll += rollOffset;

		CalculateVectors();

		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_FrontVector, glm::vec3(0, 1, 0));
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void Camera3D::Move(glm::vec3 direction)
	{
		m_Position += m_RightVector * direction.x;
		m_Position += m_UpVector * direction.y;
		m_Position += m_FrontVector * direction.z;

		CalculateMatrices();
	}

	void Camera3D::CalculateMatrices()
	{
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_FrontVector, m_UpVector);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	Frustum Camera3D::GenerateFrustum()
	{
		Frustum     frustum;
		const float half_v_side = m_ZFar * tanf(m_FieldOfView * 0.5f);
		const float half_h_side = half_v_side * m_AspectRatio;
		const glm::vec3 front_mult_far = m_ZFar * m_FrontVector;

		frustum.planes[0] = { m_Position + m_ZNear * m_FrontVector, m_FrontVector };
		frustum.planes[1] = { m_Position + front_mult_far, -m_FrontVector };
		frustum.planes[2] = { m_Position, glm::cross(front_mult_far - m_RightVector * half_h_side, m_UpVector) };
		frustum.planes[3] = { m_Position, glm::cross(m_UpVector, front_mult_far + m_RightVector * half_h_side) };
		frustum.planes[4] = { m_Position, glm::cross(m_RightVector, front_mult_far - m_UpVector * half_v_side) };
		frustum.planes[5] = { m_Position, glm::cross(front_mult_far + m_UpVector * half_v_side, m_RightVector) };

		for (auto& plane : frustum.planes)
			plane.normal = glm::normalize(plane.normal);

		return frustum;
	}

	void Camera3D::CalculateVectors()
	{
		// TODO: apply Roll rotation
		glm::vec3 front_direction;

		front_direction.x = cos(2 * 3.14 * (m_Yaw / 360)) * cos(2 * 3.14 * (m_Pitch / 360));
		front_direction.y = sin(2 * 3.14 * (m_Pitch / 360));
		front_direction.z = sin(2 * 3.14 * (m_Yaw / 360)) * cos(2 * 3.14 * (m_Pitch / 360));

		m_FrontVector = glm::normalize(front_direction);
		m_RightVector = glm::normalize(glm::cross(m_FrontVector, glm::vec3(0.0f, 1.0f, 0.0f)));
		m_UpVector = glm::normalize(glm::cross(m_RightVector, m_FrontVector));
	}

}