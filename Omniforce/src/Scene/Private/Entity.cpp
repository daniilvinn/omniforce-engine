#include "../Entity.h"
#include "../Component.h"

#include <array>

#include <glm/gtx/matrix_decompose.hpp>

namespace Omni {

	Entity::Entity(Scene* scene)
		: m_OwnerScene(scene)
	{
		m_Handle = m_OwnerScene->GetRegistry()->create();
	}

	Entity::Entity(entt::entity entity, Scene* scene)
	{
		m_Handle = entity;
		m_OwnerScene = scene;
	}

	Entity::Entity()
	{

	}

	void Entity::Serialize(nlohmann::json& node)
	{
		TagComponent& tag = GetComponent<TagComponent>();
		node.emplace(TagComponent::GetSerializableKey(), tag.tag);

		TRSComponent& trs = GetComponent<TRSComponent>();
		nlohmann::json trs_node;
		trs_node.emplace("Translation", std::initializer_list<float32>({ trs.translation.x, trs.translation.y, trs.translation.z }));
		trs_node.emplace("Rotation", std::initializer_list<float32>({ trs.rotation.x, trs.rotation.y, trs.rotation.z }));
		trs_node.emplace("Scale", std::initializer_list<float32>({ trs.scale.x, trs.scale.y, trs.scale.z }));

		node.emplace(TRSComponent::GetSerializableKey(), trs_node);

		if (HasComponent<SpriteComponent>()) {
			auto& sprite_component = GetComponent<SpriteComponent>();

			nlohmann::json sprite_component_node = nlohmann::json::object();

			std::array<float32, 4> color_tint({ 
				sprite_component.color.r, 
				sprite_component.color.g, 
				sprite_component.color.b, 
				sprite_component.color.a 
			});

			sprite_component_node.emplace("ColorTint", color_tint);
			sprite_component_node.emplace("Layer", sprite_component.layer);
			sprite_component_node.emplace("AspectRatio", sprite_component.aspect_ratio);

			if (sprite_component.texture == m_OwnerScene->GetRenderer()->GetDummyWhiteTexture())
				sprite_component_node.emplace("TextureID", nullptr);
			else 
				sprite_component_node.emplace("TextureID", sprite_component.texture.Get());

			node.emplace(sprite_component.GetSerializableKey(), sprite_component_node);
		}
		if (HasComponent<CameraComponent>()) {
			auto& camera_component = GetComponent<CameraComponent>();

			nlohmann::json camera_component_node = nlohmann::json::object();
			Shared<Camera> camera = camera_component.camera;
			camera_component_node.emplace("Primary", camera_component.primary);
			camera_component_node.emplace("ProjectionType", camera_component.camera->GetType());
			
			float32 fov = 0.0f;
			float32 orthographic_scale = 1.0f;
			camera_component_node.emplace("NearClip", camera->GetNearClip());
			camera_component_node.emplace("FarClip", camera->GetFarClip());

			if (camera->GetType() == CameraProjectionType::PROJECTION_3D) {
				Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera);
				fov = camera_3D->GetFOV();
			}
			else if (camera->GetType() == CameraProjectionType::PROJECTION_2D) {
				Shared<Camera2D> camera_2D = ShareAs<Camera2D>(camera);
				orthographic_scale = camera_2D->GetScale();
			}
			camera_component_node.emplace("FOV", glm::degrees(fov));
			camera_component_node.emplace("OrthographicScale", orthographic_scale);

			node.emplace(camera_component.GetSerializableKey(), camera_component_node);
		}
	}

	void Entity::Deserialize(nlohmann::json& node)
	{
		TagComponent& tag_component = GetComponent<TagComponent>();
		tag_component.tag = node[TagComponent::GetSerializableKey()];

		TRSComponent& trs_component = GetComponent<TRSComponent>();
		std::array<float32, 3> translation, rotation, scale;

		translation =	node[TRSComponent::GetSerializableKey()]["Translation"].get<std::array<float32, 3>>();
		rotation =		node[TRSComponent::GetSerializableKey()]["Rotation"].get<std::array<float32, 3>>();
		scale =			node[TRSComponent::GetSerializableKey()]["Scale"].get<std::array<float32, 3>>();

		trs_component.translation = { translation[0], translation[1], translation[2] };
		trs_component.rotation =	{ rotation[0], rotation[1], rotation[2] };
		trs_component.scale =		{ scale[0], scale[1], scale[2] };

		TransformComponent& transform_component = GetComponent<TransformComponent>();
		transform_component.matrix = glm::translate(glm::mat4(1.0f), trs_component.translation) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
			glm::scale(glm::mat4(1.0f), trs_component.scale);

		if (node.contains(SpriteComponent::GetSerializableKey())) {
			SpriteComponent& sprite_component = AddComponent<SpriteComponent>();

			std::array<float32, 4> color_tint =
				node[SpriteComponent::GetSerializableKey()]["ColorTint"].get<std::array<float32, 4>>();
			sprite_component.color = { color_tint[0], color_tint[1], color_tint[2],color_tint[3] };
			sprite_component.layer = node[SpriteComponent::GetSerializableKey()]["Layer"].get<uint64>();
			sprite_component.aspect_ratio = node[SpriteComponent::GetSerializableKey()]["AspectRatio"].get<float32>();

			if (node[SpriteComponent::GetSerializableKey()]["TextureID"].is_null())
				sprite_component.texture = m_OwnerScene->GetRenderer()->GetDummyWhiteTexture();
			else
				sprite_component.texture = node[SpriteComponent::GetSerializableKey()]["TextureID"].get<uint64>();
		}

		if (node.contains(CameraComponent::GetSerializableKey())) {
			CameraComponent& camera_component = AddComponent<CameraComponent>();
			if (node[CameraComponent::GetSerializableKey()]["ProjectionType"] == CameraProjectionType::PROJECTION_3D) {
				Shared<Camera3D> camera_3D = std::make_shared<Camera3D>();
				camera_3D->SetProjection(
					glm::radians(node[CameraComponent::GetSerializableKey()]["FOV"].get<float32>()),
					16.0f / 90.0f, // HACK: fix this later
					node[CameraComponent::GetSerializableKey()]["NearClip"].get<float32>(),
					node[CameraComponent::GetSerializableKey()]["FarClip"].get<float32>()
				);
				camera_component.camera = camera_3D;
			}
			else if (node[CameraComponent::GetSerializableKey()]["ProjectionType"] == CameraProjectionType::PROJECTION_2D) {
				Shared<Camera2D> camera_2D = std::make_shared<Camera2D>();
				float32 aspect_ratio = 16.0f / 9.0f; // HACK: fix it as well
				float32 scale = node[CameraComponent::GetSerializableKey()]["OrthographicScale"].get<float32>();
				camera_2D->SetScale(scale);
				camera_2D->SetProjection(
					-scale * aspect_ratio,
					scale * aspect_ratio,
					-scale,
					scale,
					node[CameraComponent::GetSerializableKey()]["NearClip"].get<float32>(),
					node[CameraComponent::GetSerializableKey()]["FarClip"].get<float32>()
				);
				camera_component.camera = camera_2D;
			}
			camera_component.primary = node[CameraComponent::GetSerializableKey()]["Primary"];
		}

	}

}