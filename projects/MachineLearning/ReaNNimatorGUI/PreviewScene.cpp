#include "PreviewScene.h"

#include "Input.h"
#include "Renderer.h"
#include "DefaultShaderProgram.h"

namespace
{
	const float		kZFar = 5000.0f;
	const float		kZNear = 0.1f;
	const float		kFovY = 45.0f;

	const Vector3	lightDir(0.0f, -1.0f, 0.0f);
	const Color4	lightIntensity(1.0f, 1.0f, 1.0f, 1.0f);
	const Color4	ambientLightIntensity(0.15f, 0.15f, 0.15f, 1.0f);
	const Vector3	pointLightPos(0.0f, 0.0f, 0.0f);

	const Vector3	cameraFocusPos(0.0f, 100.0f, 0.0f);

	const char*		s_modelTestFileNameConverted = "TrainingData\\DefaultBot\\DefaultBot.mdl";

	const char*		kFont = "arial.ttf";
}

PreviewScene*	g_PreviewScene = nullptr;

PreviewScene::PreviewScene():
	m_currentAnimTime(0.0),
	m_playingAnimation(true),
	m_drawingBones(true),
	m_hoverText(kFont)
{
	Assert(g_PreviewScene == nullptr);
	g_PreviewScene = this;

	//srand((u32)time(nullptr));

	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();
	pDefaultShaderProgram->SetAmbientLight(&ambientLightIntensity);
	pDefaultShaderProgram->SetPointLight(&pointLightPos, &lightIntensity);
	pDefaultShaderProgram->SetDirectionLight(&lightDir, &lightIntensity);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
	m_perspectiveCamera.SetPerspective(kFovY, m_screenWidth / m_screenHeight, kZNear, kZFar);
	m_perspectiveCamera.SetViewFocus(cameraFocusPos);

	m_targetModelRef = Model::MakeRef(s_modelTestFileNameConverted);
}

PreviewScene::~PreviewScene()
{
	Assert(g_PreviewScene == this);
	g_PreviewScene = nullptr;
}

void PreviewScene::Update(f64 deltaTime)
{
	//	update state
	{
		if( Input::KeyPressed('B') )
		{
			m_drawingBones = !m_drawingBones;
		}
		if( Input::KeyPressed(VK_OEM_2) )	//	forward slash
		{
			m_playingAnimation = !m_playingAnimation;
		}
	}

	m_perspectiveCamera.Update(deltaTime);

	if( m_playingAnimation )
	{
		f64	sourceAnimDuration = m_testGeneratedAnimationChannel[0].GetDuration();

		m_currentAnimTime += deltaTime;
		if( m_currentAnimTime > sourceAnimDuration )
		{
			m_currentAnimTime = 0.0;
		}
	}

	for(int i = 0; i < m_currentTestAnimsActive; ++i)
	{
		m_testGeneratedAnimationChannel[i].SetTime(m_currentAnimTime, true);
	}

	DefaultShaderProgram*	pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();	
	
	m_perspectiveCamera.Apply();
	//Vector3	modelPos(-225.0f, 0.0f, 0.0f);
	Vector3	modelPos(-100.0f, 0.0f, 0.0f);
	{
		if( m_drawingBones )
		{
			pDefaultShaderProgram->EnableTexture(false);
			pDefaultShaderProgram->EnableVertexColor(false);
			pDefaultShaderProgram->EnableLighting(false);
			pDefaultShaderProgram->EnableSkinning(false);
		}

		//pDefaultShaderProgram->ApplyLight();

		//	draw the model
		Mtx44	transform;
		transform.Identity();

		for(int i = 0; i < m_currentTestAnimsActive; ++i)
		{
			Vector3	thisModelPos = modelPos;
			thisModelPos.x += (i / 2) * 150.0f;
			thisModelPos.y += (i % 2) * 200.0f;

			transform.SetTranslation3(thisModelPos);
			Renderer::SetModelMatrix(&transform);
			if( m_drawingBones )
			{
				m_testGeneratedAnimationChannel[i].DrawBonePositions();
			}
			else
			{
				m_testGeneratedAnimationChannel[i].Draw();
			}
		}
	}

	{
		//	draw hover text
		Renderer::EnableDepthTest(false);
		DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
		pDefaultShaderProgram->Apply();

		Vector3	drawPos(-200.0f, 0.0f, 0.0f);

		f32		playTime = (f32)m_currentAnimTime;
		f32		textScale = 4.0f;
		char	s_hoverTextBuffer[256];
		sprintf(s_hoverTextBuffer, "%f", playTime);
		m_hoverText.Draw(s_hoverTextBuffer, drawPos, Color4::WHITE, textScale, PerspectiveFont::kJustify_Left);

		Renderer::EnableDepthTest(true);
	}
}

void PreviewScene::ResizeCallback(s32 width, s32 height)
{
	GameState::ResizeCallback(width, height);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}

void PreviewScene::SetupGeneratedAnims()
{
	for(int i = 0; i < m_currentTestAnimsActive; ++i)
	{
		m_testGeneratedAnimationChannel[i].Init(&m_testGeneratedAnim[i], m_targetModelRef->GetData());
		m_testGeneratedAnimationChannel[i].Start(0);
	}	
}