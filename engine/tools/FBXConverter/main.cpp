#include "stdafx.h"

#include "FBX.h"
#include "WriteModel.h"
#include "WriteAnim.h"

int _tmain(int argc, _TCHAR *argv[])
{
	assert(argc >= 4);
	if( argc < 4 )
	{
		printf("FBXConverter usage : FBXConverter.exe -[a or m] inputFilename outputFilename\n");
		return 0;
	}

	char*	mode = argv[1];
	char*	inputFilename = argv[2];
	char*	outputFilename = argv[3];

	if( strcmp(mode, "-m") == 0 )
	{
		ModelData		modelData;

		FBX	fbxFile(inputFilename);

		fbxFile.ExportToModel(modelData);

		//	write the model to file
		WriteModelToFile(modelData, outputFilename);

		//	delete the model
		FBX::CleanupModel(modelData);
	}
	else
	if( strcmp(mode, "-a") == 0 )
	{
		FBX	fbxFile(inputFilename);

		if( fbxFile.HasCharAnimation() )
		{
			AnimationData	animData;

			bool	boneLocalSpace = true;
			fbxFile.ExportToAnimation(animData, boneLocalSpace);
			
			//	write the animation to file
			WriteAnimToFile(animData, outputFilename);

			//	delete the animation
			FBX::CleanupAnim(animData);
		}	
	}
	else
	{
		printf("FBXConverter usage : FBXConverter.exe -[a or m] inputFilename outputFilename\n");
		return 0;
	}

	return 1;
}

