#pragma once

#include "SceneCommon.h"
#include <glm/glm.hpp>

namespace Omni {

	class Image;

	struct OMNIFORCE_API TagComponent {
		std::string tag;

		TagComponent() = default;
		TagComponent(const std::string& tag) { this->tag = tag; }

		operator std::string&() { return tag; }

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
	};

	struct OMNIFORCE_API TransformComponent {
		glm::mat4 matrix;
		
		TransformComponent() : matrix(1.0f) {}
		TransformComponent(const glm::mat4& transform) : matrix(transform) {}

		operator glm::mat4&() { return matrix; }
	};

	struct OMNIFORCE_API SpriteComponent {
		fvec4 color;
		Shared<Image> texture;


	};

}