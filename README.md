# BFLEngine_MLDemo
Code from when I was trying to figure out machine learning techniques.  Used as a 3D animation generator.
Note : BFL are my initials, not a reference to something else!


The engine is an old game engine that I wrote many years ago.  A lot of things are old and could use updating, but I mostly just use it to let me quickly prototype things so it serves its purpose as it is.

Projects that use the engine exist in the projects folder.  The only real one in this repository is MachineLearning.  In order to prove to myself that I understood ML techniques I figured I should choose a project and work on it, and I had some knowledge of 3D animations so I figured trying to generate them was a good choice.  I stapled LibTorch to the engine for a tensor library.  The neural network architecture ended up largely based on Stable Diffusion, with a couple of tweaks.

I made a video demonstrating the final product here : https://www.youtube.com/watch?v=hiUWEErujEc

Compiling everything depends on having LibTorch, CUDA, and FBX installed.

I used LibTorch version 2.1.2_cu118, and the project looks for environment variables "LIBTORCH_DEBUG" and "LIBTORCH_RELEASE".

The project also looks for the environment variable "FBXSDK" when compiling some portion that depends on FBX.

I excluded the assets that I was using as the intention here is a just as a code reference, but generally assets are placed in the project's "resources" directory and then when building the project the AssetBuilder in the engine processes them and places the product in the generated "data" folder.