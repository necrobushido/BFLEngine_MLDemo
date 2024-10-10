#pragma once

enum eDrawMode
{
	kPoints,
	kLineStrip,
	kLineLoop,
	kLines,
	kTriangleStrip,
	kTriangleFan,
	kTriangles,
	kQuadStrip,
	kQuads,
	kPolygons,

	kNumDrawModes
};

extern const int	s_glDrawEnum[];
inline int GetGLDrawMode(eDrawMode drawMode){ return s_glDrawEnum[drawMode]; }