# BFLEngine_MLDemo
Code from when I was trying to figure out machine learning techniques.  Used as a 3D animation generator.
Note : BFL are my initials, not a reference to something else!


The engine is an old game engine that I wrote many years ago.  A lot of things are old and could use updating (the renderer in particular probably needs a complete rewrite), but I mostly just use it to let me quickly prototype things so it serves its purpose as it is.

Projects that use the engine exist in the projects folder.  The only real one in this repository is MachineLearning.  In order to prove to myself that I understood ML techniques I figured I should choose a project and work on it, and I had some knowledge of 3D animations so I figured trying to generate them was a good choice.  I stapled LibTorch to the engine for a tensor library.  The neural network architecture ended up largely based on Stable Diffusion, with a couple of tweaks.

I made a video demonstrating the final product here : https://www.youtube.com/watch?v=hiUWEErujEc