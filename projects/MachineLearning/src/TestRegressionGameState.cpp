#include "TestRegressionGameState.h"

#include "GameFlow.h"
#include "Input.h"
#include "Renderer.h"
#include "DefaultShaderProgram.h"

namespace
{
	const float kZFar = 5000.0f;
	const float kZNear = 0.1f;

	const char*	s_font = "arial.ttf";

	enum
	{
		kPolyDegree = 4
	};

	std::string PolyDesc(torch::Tensor W, torch::Tensor b) 
	{
		s64					size = W.size(0);
		std::ostringstream	stream;

		stream << "y = ";
		for(s64 i = 0; i < size; ++i)
		{
			stream << W[i].item<float>() << " x^" << size - i << " ";
		}
		stream << "+ " << b[0].item<float>();

		return stream.str();
	}
}

TestRegressionGameState::TestRegressionGameState():
	m_font(s_font, 1 << 10),
	//	randomly generate a polynomial of kPolyDegree to try to approximate using a linear layer
	m_weightsTarget(torch::randn({kPolyDegree, 1}) * 5),
	m_biasesTarget(torch::randn({1}) * 5),
	//	init the linear layer we'll try to train to approximate the generated polynomial
	m_fc(torch::nn::Linear(m_weightsTarget.size(0), 1)),
	//	other stuff
	m_optim(m_fc->parameters(), 0.1),
	m_loss(0),
	m_batchIdx(0),
	m_trainingFinished(false)
{
	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}

TestRegressionGameState::~TestRegressionGameState()
{
}

void TestRegressionGameState::Update(f64 deltaTime)
{
	if( !m_trainingFinished )
	{
		TrainingIteration();

		if( m_trainingFinished )
		{
			DebugPrintf("Loss: %f after %lld batches\n", m_loss, m_batchIdx);
			DebugPrintf("==> Learned function:\t%s\n", PolyDesc(m_fc->weight.view({-1}), m_fc->bias).c_str());
			DebugPrintf("==> Actual function:\t%s\n", PolyDesc(m_weightsTarget.view({-1}), m_biasesTarget).c_str());
		}
	}

	DrawStuff();
}

void TestRegressionGameState::ResizeCallback(s32 width, s32 height)
{
	GameState::ResizeCallback(width, height);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}

void TestRegressionGameState::DrawStuff()
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

	sprintf(labelBuffer, "Check debug output\n");
	m_font.Draw(labelBuffer, currentEntryPos, *currentColor, OrthoFont::kJustify_Left);
}

//	Regression stuff
torch::Tensor MakeFeatures(torch::Tensor x) 
{
	x = x.unsqueeze(1);	//	unsqueeze adds a new dimension (of size 1) at the index specified (as written, this makes a tensor of {kBatchSize, 1}
	std::vector<torch::Tensor>	xs;
	for(s64 i = 0; i < kPolyDegree; ++i)
	{
		xs.push_back(x.pow(i + 1));
	}

	return torch::cat(xs, 1);	//	concatenates the tensor list along the specified dimension (as written, this makes a tensor of {kBatchSize, kPolyDegree}
}

torch::Tensor f(const torch::Tensor x, const torch::Tensor weightTargets, const torch::Tensor biasTargets) 
{
	return x.mm(weightTargets) + biasTargets.item();
}

std::pair<torch::Tensor, torch::Tensor> GetBatch(const torch::Tensor weightTargets, const torch::Tensor biasTargets) 
{
	const s64		kBatchSize = 32;
	torch::Tensor	random = torch::randn({kBatchSize});			//	{kBatchSize}
	torch::Tensor	features = MakeFeatures(random);				//	{kBatchSize, kPolyDegree}
	torch::Tensor	y = f(features, weightTargets, biasTargets);	//	calculate the target values by feeding the batch (features) through the real equation

	return std::make_pair(features, y);
}

void TestRegressionGameState::TrainingIteration()
{
	++m_batchIdx;

	// Get data
	torch::Tensor	batchX;
	torch::Tensor	batchY;
	std::tie(batchX, batchY) = GetBatch(m_weightsTarget, m_biasesTarget);

	// Reset gradients
	m_optim.zero_grad();

	// Forward pass
	torch::Tensor	output = torch::smooth_l1_loss(m_fc(batchX), batchY);	//	feed the batch through the linear layer and compare to the results from the real equation to get loss
	m_loss = output.item().toFloat();

	// Backward pass
	output.backward();

	// Apply gradients
	m_optim.step();

    // Stop criterion
	m_trainingFinished = m_loss < 1e-3f;
}