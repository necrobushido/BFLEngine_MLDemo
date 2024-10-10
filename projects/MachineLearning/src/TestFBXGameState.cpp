#include "TestFBXGameState.h"

#include "GameFlow.h"
#include "Input.h"
#include "Renderer.h"
#include "DefaultShaderProgram.h"
#include "Sprite.h"

#include "AnimGenSeqProcThread.h"

namespace
{
	const float kZFar = 5000.0f;
	const float kZNear = 0.1f;
	const float	kFovY = 45.0f;

	const Vector3	lightDir(0.0f, -1.0f, 0.0f);
	const Color4	lightIntensity(1.0f, 1.0f, 1.0f, 1.0f);
	const Color4	ambientLightIntensity(0.15f, 0.15f, 0.15f, 1.0f);
	const Vector3	pointLightPos(0.0f, 0.0f, 0.0f);

	const Vector3	cameraFocusPos(0.0f, 100.0f, 0.0f);

	const char*	s_font = "arial.ttf";

	const char* s_modelTestFileNameConverted = "TrainingData\\DefaultBot\\DefaultBot.mdl";
}

class SpriteUpdateData
{
public:
	SpriteUpdateData(u32* imageData, u32 imageDataSize, u32 width, u32 height, u32 bpp):
		m_imageData(nullptr),
		m_width(width),
		m_height(height),
		m_bpp(bpp)
	{
		m_imageData = new u32[imageDataSize];
		memcpy(m_imageData, imageData, imageDataSize*sizeof(u32));
	}

	~SpriteUpdateData()
	{
		delete [] m_imageData;
	}

public:
	u32*	m_imageData;
	u32		m_width;
	u32		m_height;
	u32		m_bpp;
};

Sprite* g_debugSourceSprite = nullptr;
Sprite* g_debugTargetSprite = nullptr;
Sprite* g_debugSupplementSprite = nullptr;

SpriteUpdateData*	g_debugSourceSpriteUpdateData = nullptr;
SpriteUpdateData*	g_debugTargetSpriteUpdateData = nullptr;
SpriteUpdateData*	g_debugSupplementSpriteUpdateData = nullptr;

Mutex	g_debugSourceSpriteUpdateDataMutex;
Mutex	g_debugTargetSpriteUpdateDataMutex;
Mutex	g_debugSupplementSpriteUpdateDataMutex;

void InitSourceSprite(u32* imageData, u32 imageDataSize, u32 width, u32 height, u32 bpp)
{
	ScopedLock	mLock(g_debugSourceSpriteUpdateDataMutex);
	if( g_debugSourceSpriteUpdateData == nullptr )
	{
		g_debugSourceSpriteUpdateData = new SpriteUpdateData(imageData, imageDataSize, width, height, bpp);
	}
}

void InitTargetSprite(u32* imageData, u32 imageDataSize, u32 width, u32 height, u32 bpp)
{
	ScopedLock	mLock(g_debugTargetSpriteUpdateDataMutex);
	if( g_debugTargetSpriteUpdateData == nullptr )
	{
		g_debugTargetSpriteUpdateData = new SpriteUpdateData(imageData, imageDataSize, width, height, bpp);
	}
}

void InitSupplementSprite(u32* imageData, u32 imageDataSize, u32 width, u32 height, u32 bpp)
{
	ScopedLock	mLock(g_debugSupplementSpriteUpdateDataMutex);
	if( g_debugSupplementSpriteUpdateData == nullptr )
	{
		g_debugSupplementSpriteUpdateData = new SpriteUpdateData(imageData, imageDataSize, width, height, bpp);
	}
}

TestFBXGameState::TestFBXGameState():
	m_font(s_font, 1 << 10),
	m_hoverText(s_font),
	m_pAnimGenProcThread(nullptr),
	m_currentAnimTime(0.0),
	m_playingAnimation(false),
	m_drawingBones(true),
	m_learningRate(1.0e-5f),
	m_currentTestAnimsActive(0)
{
	//srand((u32)time(nullptr));

	Assert(g_debugSourceSprite == nullptr);
	g_debugSourceSprite = new Sprite();

	Assert(g_debugTargetSprite == nullptr);
	g_debugTargetSprite = new Sprite();

	Assert(g_debugSupplementSprite == nullptr);
	g_debugSupplementSprite = new Sprite();

	m_procThreadState.SetState(NNProcessingThreadBase::kProcessState_Idle);

	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();
	pDefaultShaderProgram->SetAmbientLight(&ambientLightIntensity);
	pDefaultShaderProgram->SetPointLight(&pointLightPos, &lightIntensity);
	pDefaultShaderProgram->SetDirectionLight(&lightDir, &lightIntensity);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
	m_perspectiveCamera.SetPerspective(kFovY, m_screenWidth / m_screenHeight, kZNear, kZFar);
	m_perspectiveCamera.SetViewFocus(cameraFocusPos);

	m_pAnimGenProcThread = new AnimGenSeqProcessingThread();
	//m_pAnimGenProcThread = new AnimGenContProcessingThread();

	m_pAnimGenProcThread->SetLogFunc(DebugPrint);
	//m_pAnimGenProcThread->InitProject(m_mlProject);

	//
	m_pAnimGenProcThread->Start();

	m_targetModelRef = Model::MakeRef(s_modelTestFileNameConverted);
}

TestFBXGameState::~TestFBXGameState()
{
	m_pAnimGenProcThread->Stop();

	delete m_pAnimGenProcThread;
	m_pAnimGenProcThread = nullptr;

	delete g_debugSupplementSprite;
	g_debugSupplementSprite = nullptr;

	delete g_debugTargetSprite;
	g_debugTargetSprite = nullptr;

	delete g_debugSourceSprite;
	g_debugSourceSprite = nullptr;
}

void TestFBXGameState::Update(f64 deltaTime)
{
	//	update debug sprites
	{
		{
			ScopedLock	mLock(g_debugSourceSpriteUpdateDataMutex);
			if( g_debugSourceSpriteUpdateData != nullptr )
			{
				g_debugSourceSprite->Init(
					g_debugSourceSpriteUpdateData->m_imageData, 
					g_debugSourceSpriteUpdateData->m_width,
					g_debugSourceSpriteUpdateData->m_height, 
					g_debugSourceSpriteUpdateData->m_bpp );

				delete g_debugSourceSpriteUpdateData;
				g_debugSourceSpriteUpdateData = nullptr;
			}
		}

		{
			ScopedLock	mLock(g_debugTargetSpriteUpdateDataMutex);
			if( g_debugTargetSpriteUpdateData != nullptr )
			{
				g_debugTargetSprite->Init(
					g_debugTargetSpriteUpdateData->m_imageData, 
					g_debugTargetSpriteUpdateData->m_width,
					g_debugTargetSpriteUpdateData->m_height, 
					g_debugTargetSpriteUpdateData->m_bpp );

				delete g_debugTargetSpriteUpdateData;
				g_debugTargetSpriteUpdateData = nullptr;
			}
		}

		{
			ScopedLock	mLock(g_debugSupplementSpriteUpdateDataMutex);
			if( g_debugSupplementSpriteUpdateData != nullptr )
			{
				g_debugSupplementSprite->Init(
					g_debugSupplementSpriteUpdateData->m_imageData, 
					g_debugSupplementSpriteUpdateData->m_width,
					g_debugSupplementSpriteUpdateData->m_height, 
					g_debugSupplementSpriteUpdateData->m_bpp );

				delete g_debugSupplementSpriteUpdateData;
				g_debugSupplementSpriteUpdateData = nullptr;
			}
		}		
	}

	//	update state
	{
		int	processingThreadState = m_pAnimGenProcThread->GetState();
		int	currentState = m_procThreadState.GetState();

		if( currentState != processingThreadState )
		{
			if( currentState == NNProcessingThreadBase::kProcessState_Generating &&
				processingThreadState == NNProcessingThreadBase::kProcessState_Idle )
			{
				//	an animation was just generated and the thread went to idle, so we should be good to copy the animation over here and use it
				//m_pAnimGenProcThread->ExportGeneratedAnim(&m_testGeneratedAnim);
				m_currentTestAnimsActive = m_pAnimGenProcThread->ExportGeneratedAnims(m_testGeneratedAnim, kNumAnimationsToSample);

				m_targetModelRef = Model::MakeRef(s_modelTestFileNameConverted);

				for(int i = 0; i < m_currentTestAnimsActive; ++i)
				{
					m_testGeneratedAnimationChannel[i].Init(&m_testGeneratedAnim[i], m_targetModelRef->GetData());
					m_testGeneratedAnimationChannel[i].Start(0);
				}
			}

			m_procThreadState.SetState(processingThreadState);
		}

		m_procThreadState.Update(deltaTime);

		if( Input::KeyPressed('1') )
		{
			m_pAnimGenProcThread->ChangeState(NNProcessingThreadBase::kProcessState_Idle);
		}
		if( Input::KeyPressed('2') )
		{
			m_pAnimGenProcThread->ChangeState(NNProcessingThreadBase::kProcessState_Training);
		}		
		if( Input::KeyPressed('3') )
		{
			m_pAnimGenProcThread->UpdateTargetModel(s_modelTestFileNameConverted);
			m_pAnimGenProcThread->SetSampleCount(kNumAnimationsToSample);

			m_pAnimGenProcThread->ChangeState(NNProcessingThreadBase::kProcessState_Generating);
		}
		if( Input::KeyPressed('4') )
		{
			m_pAnimGenProcThread->ChangeState(NNProcessingThreadBase::kProcessState_Save);
		}
		if( Input::KeyPressed('5') )
		{
			m_pAnimGenProcThread->ChangeState(NNProcessingThreadBase::kProcessState_Load);
		}
		if( Input::KeyPressed('6') )
		{
			m_pAnimGenProcThread->ChangeState(NNProcessingThreadBase::kProcessState_EvalLearningRate);
		}
		if( Input::KeyPressed('B') )
		{
			m_drawingBones = !m_drawingBones;
		}
		if( Input::KeyPressed(VK_OEM_2) )	//	forward slash
		{
			m_playingAnimation = !m_playingAnimation;
		}
		//const f64 kAdvanceTimeRate = 0.005;
		const f64 kAdvanceTimeRate = 0.1;
		if( Input::KeyPressed(VK_OEM_COMMA) )
		{
			m_currentAnimTime -= kAdvanceTimeRate;
			if( m_currentAnimTime < 0.0 )
			{
				m_currentAnimTime = 0.0;
			}
		}
		if( Input::KeyPressed(VK_OEM_PERIOD) )
		{
			f64	sourceAnimDuration = m_testGeneratedAnimationChannel[0].GetDuration();

			m_currentAnimTime += kAdvanceTimeRate;
			if( m_currentAnimTime > sourceAnimDuration )
			{
				m_currentAnimTime = sourceAnimDuration;
			}
		}
		if( Input::KeyPressed(VK_OEM_4) )	//	'['
		{
			if( m_testGeneratedAnim[0].numKeyFrames > 2 )
			{
				f32	generatedKeyFrameDuration = m_testGeneratedAnim[0].keyFrames[1].time - m_testGeneratedAnim[0].keyFrames[0].time;

				m_currentAnimTime -= (f64)generatedKeyFrameDuration;
				if( m_currentAnimTime < 0.0 )
				{
					m_currentAnimTime = 0.0;
				}
			}
		}
		if( Input::KeyPressed(VK_OEM_6) )	//	']'
		{
			if( m_testGeneratedAnim[0].numKeyFrames > 2 )
			{
				f32	generatedKeyFrameDuration = m_testGeneratedAnim[0].keyFrames[1].time - m_testGeneratedAnim[0].keyFrames[0].time;
				f64	generatedAnimDuration = m_testGeneratedAnim[0].keyFrames[m_testGeneratedAnim[0].numKeyFrames-1].time;

				m_currentAnimTime += (f64)generatedKeyFrameDuration;
				if( m_currentAnimTime > generatedAnimDuration )
				{
					m_currentAnimTime = generatedAnimDuration;
				}
			}
		}
		/*if( Input::KeyPressed(VK_OEM_PLUS) )
		{
			m_targetModelIdx++;
			if( m_targetModelIdx >= ARRAY_SIZE(s_targetModels) )
			{
				m_targetModelIdx = 0;
			}
		}
		if( Input::KeyPressed(VK_OEM_MINUS) )
		{
			m_targetModelDescIdx++;
			if( m_targetModelDescIdx >= ARRAY_SIZE(s_modelTags) )
			{
				m_targetModelDescIdx = 0;
			}
		}*/
		if( Input::KeyPressed(VK_OEM_PLUS) )
		{
			m_learningRate = m_learningRate * 2.0f;
			m_pAnimGenProcThread->SetLearningRate(m_learningRate);
		}
		if( Input::KeyPressed(VK_OEM_MINUS) )
		{
			m_learningRate = m_learningRate * 0.5f;
			m_pAnimGenProcThread->SetLearningRate(m_learningRate);
		}		
		if( Input::KeyPressed('0') )
		{			
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
	Vector3	modelPos(-225.0f, 0.0f, 0.0f);
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

		Vector3	drawPos = modelPos;
		drawPos.y += 200.0f;

		//f32		playTime = (f32)m_testGeneratedAnimationChannel.GetInterpTime();
		f32		playTime = (f32)m_currentAnimTime;
		f32		textScale = 4.0f;
		char	s_hoverTextBuffer[256];
		sprintf(s_hoverTextBuffer, "%f", playTime);
		m_hoverText.Draw(s_hoverTextBuffer, drawPos, Color4::WHITE, textScale, PerspectiveFont::kJustify_Left);

		Renderer::EnableDepthTest(true);
	}

	Draw2DStuff();
}

void TestFBXGameState::ResizeCallback(s32 width, s32 height)
{
	GameState::ResizeCallback(width, height);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}

void TestFBXGameState::Draw2DStuff()
{
	//	draw the menu
	m_orthoCamera.Apply();
	
	Vector3		numberPos(0.0f, (f32)m_screenHeight - m_font.GetTextHeight(), -10.0f);
	Vector3		currentEntryPos = numberPos;
	currentEntryPos.x += 32.0f;

	const Color4	white(1.0f, 1.0f, 1.0f, 1.0f);
	const Color4	red(1.0f, 0.0f, 0.0f, 1.0f);

	const Color4*	currentColor = &white;
	char			labelBuffer[256];

	sprintf(labelBuffer, "Current thread state = %s", NNProcessingThreadBase::GetStateName(m_procThreadState.GetState()));
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "1 = idle, 2 = training, 3 = generate, 4 = save, 5 = load, 6 = eval learning rate");
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "+ = increase learning rate, - = decrease learning rate, 0 = target anim");
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	/*currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "target model = %s", s_targetModels[m_targetModelIdx]);
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "target model desc = %s", s_modelTags[m_targetModelDescIdx]);
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "target anim = %s", s_animTypeTags[0]);
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);*/

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "learning rate = %.9g", m_learningRate);
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);
	

	{
		f32			xOffset = m_screenWidth;
		f32			yOffset = 0.0f;
		f32			baseScale = 8.0f;
		f32			xScale = baseScale;
		f32			yScale = baseScale;
		const f32	kMargin = 20.0f;

		Mtx44	transform;
		transform.Identity();
		Vector3	scale(xScale, yScale, 1.0f);
		transform.SetScale(scale);

		if( g_debugTargetSprite->IsInitialized() )
		{
			xOffset = m_screenWidth - (g_debugTargetSprite->GetWidth() * 0.5f * xScale + kMargin);
			yOffset = g_debugTargetSprite->GetHeight() * 0.5f * yScale + kMargin;
			Vector3	drawPosition(xOffset, yOffset, -2000.0f);
			transform.SetTranslation3(drawPosition);
			Renderer::SetModelMatrix(&transform);
			g_debugTargetSprite->Draw();

			xOffset -= g_debugTargetSprite->GetWidth() * 0.5f * xScale;
		}

		if( g_debugSourceSprite->IsInitialized() )
		{
			//xOffset -= g_debugSourceSprite->GetWidth() * 0.5f * xScale + kMargin;
			xOffset = g_debugSourceSprite->GetWidth() * 0.5f * xScale + kMargin;
			yOffset = g_debugSourceSprite->GetHeight() * 0.5f * yScale + kMargin;
			Vector3	drawPosition(xOffset, yOffset, -2000.0f);
			transform.SetTranslation3(drawPosition);
			Renderer::SetModelMatrix(&transform);
			g_debugSourceSprite->Draw();

			xOffset -= g_debugSourceSprite->GetWidth() * 0.5f * xScale;
		}

		if( g_debugSupplementSprite->IsInitialized() )
		{
			xOffset = m_screenWidth - (g_debugSupplementSprite->GetWidth() * 0.5f * xScale + kMargin);
			yOffset = (f32)m_screenHeight - g_debugSupplementSprite->GetHeight() * 0.5f * yScale - kMargin;
			Vector3	drawPosition(xOffset, yOffset, -2000.0f);
			transform.SetTranslation3(drawPosition);
			Renderer::SetModelMatrix(&transform);
			g_debugSupplementSprite->Draw();
		}
	}
}