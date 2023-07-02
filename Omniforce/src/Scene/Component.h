#pragma once

#include "SceneCommon.h"
#include <glm/glm.hpp>
#include <Core/UUID.h>

namespace Omni {

	class Image;

	struct OMNIFORCE_API UUIDComponent {
		UUID id;

		UUIDComponent() = default;
		UUIDComponent(const UUID& uuid) : id(uuid) {}

		operator UUID& () { return id; }

	};

	struct OMNIFORCE_API TagComponent {
		std::string tag;

		TagComponent() = default;
		TagComponent(const std::string& tag) { this->tag = tag; }

		operator std::string&() { return tag; }
		static const char* GetSerializableKey() { return "TagComponent"; }

	};

	struct OMNIFORCE_API TRSComponent {
		glm::vec3 translation;
		glm::vec3 rotation;
		glm::vec3 scale;

		TRSComponent() : translation(0.0f), rotation(0.0f), scale(1.0f) {}
			
		TRSComponent(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale) {
			this->translation = translation;
			this->rotation = rotation;
			this->scale = scale;
		}

		static const char* GetSerializableKey() { return "TRSComponent"; }

	};

	struct OMNIFORCE_API TransformComponent {
		glm::mat4 matrix;
		
		TransformComponent() : matrix(1.0f) {}
		TransformComponent(const glm::mat4& transform) : matrix(transform) {}

		operator glm::mat4&() { return matrix; }

		static const char* GetSerializableKey() { return "TransformComponent"; }
	};

	struct OMNIFORCE_API SpriteComponent {
		fvec4 color;
		UUID texture;
		int32 layer;
		float32 aspect_ratio; // of a texture

		SpriteComponent() : color{ 1.0f, 1.0f, 1.0f, 1.0f }, texture(0), layer(0), aspect_ratio(1.0f) {};
		SpriteComponent(const fvec4& tint) : color(tint), texture(0), layer(0) {};
		SpriteComponent(const fvec4& tint, const UUID& id) : color(tint), texture(id), layer(0) {};

		static const char* GetSerializableKey() { return "SpriteComponent"; }
	};

	struct OMNIFORCE_API CameraComponent {
		Shared<Camera> camera;
		bool primary;

		CameraComponent() : camera(nullptr), primary(false) {}
		CameraComponent(Shared<Camera> value, bool is_primary) : camera(value), primary(is_primary) {}

		static const char* GetSerializableKey() { return "CameraComponent"; }
	};

	struct OMNIFORCE_API RigidBody2DComponent {
		enum class Type { STATIC, DYNAMIC, KINEMATIC } type = Type::STATIC;
		float32 mass = 10.0f;
		float32 linear_drag = 0.1f;
		float32 angular_drag = 0.5f;
		bool disable_gravity = false;
		bool sensor_mode = false;
		void* internal_body = nullptr;

		static const char* GetSerializableKey() { return "RigidBody2DComponent"; }
	};

	struct OMNIFORCE_API BoxColliderComponent {
		fvec3 size = { 1.0f, 1.0f, 1.0f };
		float32 convex_radius = 0.1f;
		float32 restitution = 1.0f;
		float32 friction = 1.0f;
		float32 damping = 1.0f;

		static const char* GetSerializableKey() { return "BoxColliderComponent"; }
	};

	struct OMNIFORCE_API SphereColliderComponent {
		float32 radius = 1.0f;
		float32 restitution = 1.0f;
		float32 friction = 1.0f;
		float32 damping = 1.0f;

		static const char* GetSerializableKey() { return "SphereColliderComponent"; }
	};

}