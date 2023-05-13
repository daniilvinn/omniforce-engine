#include <Omniforce.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

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

		{
		if (Input::KeyPressed(KeyCode::KEY_W))
			m_Camera->Move({ 0.0f, 0.0f, 0.05f });
		if (Input::KeyPressed(KeyCode::KEY_S))
			m_Camera->Move({ 0.0f, 0.0f, -0.05f });
		if (Input::KeyPressed(KeyCode::KEY_A))
			m_Camera->Move({ -0.05f, 0.0f, 0.0f });
		if (Input::KeyPressed(KeyCode::KEY_D))
			m_Camera->Move({ 0.05f, 0.0f, 0.0f});

		if (Input::KeyPressed(KeyCode::KEY_Q))
			m_Camera->Rotate(-0.5f, 0.0f, 0.0f, true);
		if (Input::KeyPressed(KeyCode::KEY_E))
			m_Camera->Rotate(0.5f, 0.0f, 0.0f, true);
		}

		SceneRenderData render_data{ .target = swapchain_image, .camera = ShareAs<Camera>(m_Camera)};
		m_Renderer->BeginScene(render_data);
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

		return false;
	}

	void Destroy() override
	{
		Renderer::WaitDevice();
		m_Renderer->Destroy();
	}

	void Launch() override
	{

		JobSystem::Get()->Wait();

		SceneRendererSpecification renderer_spec = {};
		renderer_spec.anisotropic_filtering = 16;

		m_Renderer = SceneRenderer::Create(renderer_spec);

		m_Camera = std::make_shared<Camera3D>();
		m_Camera->SetProjection( glm::radians(90.0f), 16.0 / 9.0, 0.0f, 100.0f );

		m_Scene = new Scene(SceneType::SCENE_TYPE_2D);

	}

	/*
	*	DATA
	*/
	Shared<Camera3D> m_Camera;
	SceneRenderer* m_Renderer;
	Scene* m_Scene;
	uint32 m_SelectedEntity = 0;
	bool m_EntitySelected = false;
};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}