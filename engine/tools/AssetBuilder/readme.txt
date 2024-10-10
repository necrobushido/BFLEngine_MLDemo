AssetBuilder parses the "resources" directory and all subdirectories, then executes a predetermined command to convert each of those resources (which exist in a human-useful format) into an engine-useful format and copy them to the engine's "data" directory.  It also executes DotHGenerator to create the header files used for entities in the engine.

Improvements to be made :
	get output to print while processing
	multithread
	make recognized file types configurable without compiling