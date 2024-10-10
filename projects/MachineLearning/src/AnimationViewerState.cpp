#include "AnimationViewerState.h"

#include "GameFlow.h"
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

	const char*		s_font = "arial.ttf";
}

AnimationViewerState::AnimationViewerState():
	m_font(s_font, 1 << 10),
	m_hoverText(s_font),
	m_modelIdx(0),
	m_animIdx(0),
	m_currentAnimTime(0.0),
	m_playingAnimation(false),
	m_drawingBones(true),
	m_atdfFileType(AnimTrainingDataFiles::kFileType_Training)
{
	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);

	DefaultShaderProgram*	pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();
	pDefaultShaderProgram->SetAmbientLight(&ambientLightIntensity);
	pDefaultShaderProgram->SetPointLight(&pointLightPos, &lightIntensity);
	pDefaultShaderProgram->SetDirectionLight(&lightDir, &lightIntensity);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
	m_perspectiveCamera.SetPerspective(kFovY, m_screenWidth / m_screenHeight, kZNear, kZFar);
	m_perspectiveCamera.SetViewFocus(cameraFocusPos);

	m_atdf.FindFiles();

	UpdateAnimation();
}

AnimationViewerState::~AnimationViewerState()
{
}

void AnimationViewerState::Update(f64 deltaTime)
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
			f64	sourceAnimDuration = m_testAnimationChannel.GetDuration();

			m_currentAnimTime += kAdvanceTimeRate;
			if( m_currentAnimTime > sourceAnimDuration )
			{
				m_currentAnimTime = sourceAnimDuration;
			}
		}
		if( Input::KeyPressed(VK_OEM_4) )	//	'['
		{
			if( m_testAnimRef->GetData()->numKeyFrames > 2 )
			{
				f32	generatedKeyFrameDuration = m_testAnimRef->GetData()->keyFrames[1].time - m_testAnimRef->GetData()->keyFrames[0].time;

				m_currentAnimTime -= (f64)generatedKeyFrameDuration;
				if( m_currentAnimTime < 0.0 )
				{
					m_currentAnimTime = 0.0;
				}
			}
		}
		if( Input::KeyPressed(VK_OEM_6) )	//	']'
		{
			if( m_testAnimRef->GetData()->numKeyFrames > 2 )
			{
				f32	generatedKeyFrameDuration = m_testAnimRef->GetData()->keyFrames[1].time - m_testAnimRef->GetData()->keyFrames[0].time;
				f64	generatedAnimDuration = m_testAnimRef->GetData()->keyFrames[m_testAnimRef->GetData()->numKeyFrames-1].time;

				m_currentAnimTime += (f64)generatedKeyFrameDuration;
				if( m_currentAnimTime > generatedAnimDuration )
				{
					m_currentAnimTime = generatedAnimDuration;
				}
			}
		}
		if( Input::KeyPressed(VK_OEM_PLUS) )
		{
			m_modelIdx++;
			if( m_modelIdx >= m_atdf.GetModelCount(m_atdfFileType) )
			{
				m_modelIdx = 0;
			}

			m_animIdx = 0;
			m_currentAnimTime = 0.0;

			UpdateAnimation();
		}
		if( Input::KeyPressed(VK_OEM_MINUS) )
		{
			m_modelIdx--;
			if( m_modelIdx < 0 )
			{
				m_modelIdx = m_atdf.GetModelCount(m_atdfFileType) - 1;
			}

			m_animIdx = 0;
			m_currentAnimTime = 0.0;

			UpdateAnimation();
		}
		if( Input::KeyPressed('N') )
		{
			m_animIdx--;
			if( m_animIdx < 0 )
			{
				m_animIdx = m_atdf.GetAnimCount(m_atdfFileType, m_modelIdx) - 1;
			}

			m_currentAnimTime = 0.0;

			UpdateAnimation();
		}
		if( Input::KeyPressed('M') )
		{
			m_animIdx++;
			if( m_animIdx >= m_atdf.GetAnimCount(m_atdfFileType, m_modelIdx) )
			{
				m_animIdx = 0;
			}

			m_currentAnimTime = 0.0;

			UpdateAnimation();
		}
		if( Input::KeyPressed('T') )
		{
			int	nextFileType = m_atdfFileType + 1;
			if( nextFileType >= AnimTrainingDataFiles::kFileType_Count )
			{
				nextFileType = 0;
			}
			m_atdfFileType = (AnimTrainingDataFiles::eFileType)nextFileType;

			m_modelIdx = 0;
			m_animIdx = 0;
			m_currentAnimTime = 0.0;

			UpdateAnimation();
		}
	}

	m_perspectiveCamera.Update(deltaTime);

	if( m_playingAnimation )
	{
		f64	sourceAnimDuration = m_testAnimationChannel.GetDuration();

		m_currentAnimTime += deltaTime;
		if( m_currentAnimTime > sourceAnimDuration )
		{
			m_currentAnimTime = 0.0;
		}
	}

	m_testAnimationChannel.SetTime(m_currentAnimTime, true);

	DefaultShaderProgram*	pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();	
	
	m_perspectiveCamera.Apply();
	Vector3	modelPos(0.0f, 0.0f, 0.0f);
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

		transform.SetTranslation3(modelPos);
		Renderer::SetModelMatrix(&transform);
		if( m_drawingBones )
		{
			m_testAnimationChannel.DrawBonePositions();
		}
		else
		{
			m_testAnimationChannel.Draw();
		}
	}

	{
		//	draw hover text
		Renderer::EnableDepthTest(false);
		DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
		pDefaultShaderProgram->Apply();

		Vector3	drawPos = modelPos;
		drawPos.y += 200.0f;

		f32		playTime = (f32)m_currentAnimTime;
		f32		textScale = 4.0f;
		char	s_hoverTextBuffer[256];
		sprintf(s_hoverTextBuffer, "%f", playTime);
		m_hoverText.Draw(s_hoverTextBuffer, drawPos, Color4::WHITE, textScale, PerspectiveFont::kJustify_Left);

		Renderer::EnableDepthTest(true);
	}

	DrawStuff();
}

void AnimationViewerState::ResizeCallback(s32 width, s32 height)
{
	GameState::ResizeCallback(width, height);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}

void AnimationViewerState::DrawStuff()
{
	//	draw the menu
	m_orthoCamera.Apply();
	
	Vector3		numberPos(0.0f, (f32)m_screenHeight - m_font.GetTextHeight(), -10.0f);
	Vector3		currentEntryPos = numberPos;
	currentEntryPos.x += 32.0f;

	const Color4	white(1.0f, 1.0f, 1.0f, 1.0f);
	const Color4	red(1.0f, 0.0f, 0.0f, 1.0f);

	DefaultShaderProgram*	pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();

	const Color4*	currentColor = &white;
	char			labelBuffer[256];

	sprintf(labelBuffer, "Animation viewer for training data");
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "+- = target model, NM = target animation, B = toggle bone view, T = toggle training/validation");
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "[] = advance by frame, <> = advance by time, / = toggle continuous play");
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "target model = %s", m_atdf.GetModelName(m_atdfFileType, m_modelIdx).c_str());
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "target anim = %s    (%d of %d)", m_atdf.GetAnimName(m_atdfFileType, m_modelIdx, m_animIdx).c_str(), m_animIdx+1, m_atdf.GetAnimCount(m_atdfFileType, m_modelIdx));
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

	currentEntryPos.y -= 32.0f;
	sprintf(labelBuffer, "anim desc = %s", m_animDesc.c_str());
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);
}

void AnimationViewerState::UpdateAnimation()
{
	std::string	modelPath = m_atdf.GetModelPath(m_atdfFileType, m_modelIdx);
	std::string	animPath = m_atdf.GetAnimPath(m_atdfFileType, m_modelIdx, m_animIdx);
	std::string	animDescPath = m_atdf.GetAnimDescPath(m_atdfFileType, m_modelIdx, m_animIdx);

	m_testModelRef = Model::MakeRef(modelPath.c_str());
	m_testAnimRef = Animation::MakeRef(animPath.c_str());

	m_testAnimationChannel.Init(m_testAnimRef->GetData(), m_testModelRef->GetData());
	m_testAnimationChannel.Start(0);

	//	fill out the description text
	{
		FileRef<char>		descFileRef = g_fileManager->MakeRef(animDescPath.c_str());
		const char*			descFileData = (*descFileRef);
		const size_t		descFileSize = descFileRef.FileSize();

		char*				descProcessData = new char[descFileSize+1];
		memset(descProcessData, 0, sizeof(char) * descFileSize+1);
		strncpy(descProcessData, descFileData, descFileSize);

		m_animDesc = descProcessData;

		delete [] descProcessData;
	}
}