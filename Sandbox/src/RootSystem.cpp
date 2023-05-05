#include <Omniforce.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace Omni;

class RootSubsystem : public Subsystem 
{
public:
	~RootSubsystem() override 
	{
		Destroy();
	}

	void OnUpdate() override
	{
		Shared<Image> swapchain_image = Renderer::GetSwapchainImage();

		Renderer::ClearImage(swapchain_image, { 0.420, 0.69, 0.14 });

		static fvec4 clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };

		if (Input::KeyPressed(KeyCode::KEY_W))
			m_Camera->Move({ 0.0f, 0.0f, 0.005f });
		if (Input::KeyPressed(KeyCode::KEY_S))
			m_Camera->Move({ 0.0f, 0.0f, -0.005f });
		if (Input::KeyPressed(KeyCode::KEY_A))
			m_Camera->Move({ -0.005f, 0.0f, 0.0f });
		if (Input::KeyPressed(KeyCode::KEY_D))
			m_Camera->Move({ 0.005f, 0.0f, 0.0f});

		if (Input::KeyPressed(KeyCode::KEY_Q))
			m_Camera->Rotate(-0.5f, 0.0f, 0.0f, true);
		if (Input::KeyPressed(KeyCode::KEY_E))
			m_Camera->Rotate(0.5f, 0.0f, 0.0f, true);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)) *
								glm::rotate(glm::mat4(1.0f), glm::radians(29.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
								glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 0.5f, 1.0f));

		SceneRenderData render_data{ .target = swapchain_image, .camera = ShareAs<Camera>(m_Camera)};
		m_Renderer->BeginScene(render_data);
		m_Renderer->RenderSpriteTextured(glm::mat4(1.0f), m_Image);
		//m_Renderer->RenderSpriteOpaque(transform, { 0.5f, 0.2f, 0.8f, 1.0f });
		m_Renderer->EndScene();
		
	}

	void OnEvent(Event* e) override
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnWindowResize));
	}

	bool OnWindowResize(WindowResizeEvent* e) {
		if (e->GetResolution().x == 0 || e->GetResolution().y == 0) return false;

		float32 aspect_ratio = (float32)e->GetResolution().x / (float32)e->GetResolution().y;
		m_Camera->SetProjection(glm::radians(90.0f), aspect_ratio, 0.0f, 100.0f);

		OMNIFORCE_CORE_WARNING("aspect ratio: {}", aspect_ratio);

		return false;
	}

	void Destroy() override
	{
		Renderer::WaitDevice();
		m_Renderer->Destroy();
		m_Image->Destroy();
	}

	void Launch() override
	{
		ImageSpecification image_spec = ImageSpecification::Default();
		image_spec.path = "assets/textures/sprite.png";
		m_Image = Image::Create(image_spec);

		JobSystem::Get()->Wait();

		SceneRendererSpecification renderer_spec = {};
		renderer_spec.anisotropic_filtering = 16;

		m_Renderer = SceneRenderer::Create(renderer_spec);
		m_Renderer->AcquireTextureIndex(m_Image, SamplerFilteringMode::LINEAR);

		m_Camera = std::make_shared<Camera3D>();
		m_Camera->SetProjection( glm::radians(90.0f), 16.0 / 9.0, 0.0f, 100.0f );
	}

	/*
	*	DATA
	*/

	Shared<Image> m_Image;
	Shared<Camera3D> m_Camera;
	SceneRenderer* m_Renderer;
};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}