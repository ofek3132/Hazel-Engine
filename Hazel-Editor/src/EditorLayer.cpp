#include "EditorLayer.h"

#include <Platform/OpenGL/OpenGLShader.h>

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Hazel {

	EditorLayer::EditorLayer()
		: Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f, true)
	{
	}

	void EditorLayer::OnAttach()
	{
		HZ_PROFILE_FUNCTION();

		m_HmmTexture = Texture2D::Create("assets/textures/thinking_smol.png");
		m_SpriteSheet = Texture2D::Create("assets/game/textures/RPGpack_sheet.png");
		m_RoofTexture = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 0, 4 }, { 128, 128 }, { 2, 3 });
		m_EntranceTexture = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 9 }, { 128, 128 }, { 2, 1 });

		FramebufferSpecification frameBufferSpec = { 1280, 720 };
		m_Framebuffer = Framebuffer::Create(frameBufferSpec);
	}

	void EditorLayer::OnDetach()
	{
		HZ_PROFILE_FUNCTION();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		HZ_PROFILE_FUNCTION();

		// Update
		if (m_ViewportFocused)
			m_CameraController.OnUpdate(ts);

		// Render
		Renderer2D::ResetStats();
		{
			HZ_PROFILE_SCOPE("Render Prep");
			m_Framebuffer->Bind();
			RenderCommand::SetClearColor({ 0.15f, 0.15f, 0.15f, 1.0f });
			RenderCommand::Clear();
		}

		{
			HZ_PROFILE_SCOPE("Render Draw");
			Renderer2D::BeginScene(m_CameraController.GetCamera());

			Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 30.0f, 30.0f }, glm::radians(m_Rotation), m_HmmTexture, m_TextureRepeatCount, m_ColorTint);

			for (int y = 0; y < m_SquaresPerLine; y++) {
				for (int x = 0; x < m_SquaresPerLine; x++) {
					float sizeF = 0.2f;
					float num = sizeF + m_Margin;
					glm::vec2 pos(x * num - num * (m_SquaresPerLine - 1) / 2.0f, y * num - num * (m_SquaresPerLine - 1) / 2.0f);
					glm::vec2 size(sizeF, sizeF);
					glm::vec4 color;
					if (m_SquaresPerLine != 1)
						color = glm::vec4(m_StartColor.r + (m_EndColor.r - m_StartColor.r) * ((float)x / (m_SquaresPerLine - 1)),
							m_StartColor.g + (m_EndColor.g - m_StartColor.g) * ((float)(x + y) / (m_SquaresPerLine * 2 - 2)),
							m_StartColor.b + (m_EndColor.b - m_StartColor.b) * ((float)y / (m_SquaresPerLine - 1)),
							m_Alpha);
					else
						color = glm::vec4(m_StartColor.r, m_StartColor.g, m_StartColor.b, m_Alpha);
					if (m_DrawSquares)
						Renderer2D::DrawQuad(glm::vec3{ pos, m_PositionZ }, size, glm::radians(m_Rotation), color);
					if (m_DrawTextures)
						Renderer2D::DrawQuad(glm::vec3(pos, 0.5f), size, glm::radians(m_Rotation), m_HmmTexture, m_TextureRepeatCount, m_ColorTint);
				}
			}

			Renderer2D::DrawQuad({ 0.0f, 1.99f, 0.6f }, { 2.0f, 3.0f }, glm::radians(m_Rotation), m_RoofTexture);
			Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.7f }, { 2.0f, 1.0f }, glm::radians(m_Rotation), m_EntranceTexture);

			// Middle of screen
			Renderer2D::DrawQuad({ 0.0f, 0.0f, 1.0f }, { 0.01f, 0.01f }, glm::radians(45.0f), { 1.0f, 1.0f, 1.0f, 1.0f });

			Renderer2D::EndScene();
			m_Framebuffer->Unbind();
		}
	}

	void EditorLayer::OnImGuiRender()
	{
		HZ_PROFILE_FUNCTION();

		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Hazel", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit")) Application::Get().Close();
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::Begin("Settings 2D");
		if (m_DrawSquares)
		{
			ImGui::ColorEdit3("Square start color: ", glm::value_ptr(m_StartColor));
			ImGui::ColorEdit3("Square end color: ", glm::value_ptr(m_EndColor));
		}
		if (m_DrawTextures)
			ImGui::ColorEdit4("Texture tint color: ", glm::value_ptr(m_ColorTint));
		ImGui::DragFloat("z: ", &m_PositionZ, 0.05f, -1.0f, 1.0f);
		ImGui::DragFloat("Alpha: ", &m_Alpha, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Rotation: ", &m_Rotation, 1.0f, -180.0f, 180.0f);
		ImGui::DragInt("Squares per line: ", &m_SquaresPerLine, 1.0f, 1, 100);
		ImGui::DragFloat("Margin: ", &m_Margin, 0.01f, 0.0f, 2.0f);
		if (m_DrawTextures)
			ImGui::DragInt("Texture repeat count: ", &m_TextureRepeatCount, 1.0f, 1, 10);
		ImGui::Checkbox("Draw squares", &m_DrawSquares);
		ImGui::Checkbox("Draw textures", &m_DrawTextures);

		auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer2D stats:");
		ImGui::Text("Draw calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		uint32_t textureId = m_HmmTexture->GetRendererID();
		ImGui::Image((void*)textureId, ImVec2{ 128, 128 });

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused || !m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		if (m_ViewportSize != *((glm::vec2*) & viewportPanelSize))
		{
			m_Framebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

			m_CameraController.OnResize(viewportPanelSize.x, viewportPanelSize.y);
		}
		textureId = m_Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image((void*)textureId, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);
	}

}