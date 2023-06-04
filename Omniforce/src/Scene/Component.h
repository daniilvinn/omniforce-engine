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

		SpriteComponent() : color{ 1.0f, 1.0f, 1.0f, 1.0f }, texture(0), layer(0) {};
		SpriteComponent(const fvec4& tint) : color(tint), texture(0), layer(0) {};
		SpriteComponent(const fvec4& tint, const UUID& id) : color(tint), texture(id), layer(0) {};

		static const char* GetSerializableKey() { return "SpriteComponent"; }
	};

	struct OMNIFORCE_API CameraComponent {
		Shared<Camera> camera;

		CameraComponent() : camera(nullptr){}
		CameraComponent(Shared<Camera> value) : camera(value) {}

		static const char* GetSerializableKey() { return "CameraComponent"; }
	};

}