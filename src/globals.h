/*
  vim:ts=4
  vim:sw=4
*/
#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "stdbool.h"

extern int gMode;
extern int gScreenWidth;
extern int gScreenHeight;
extern int gMapWidth;
extern int gMapHeight;
extern float gScreenDist;
extern float gDeltaAngle;
extern int gHalfScreenWidth;
extern int gHalfScreenHeight;

extern float gfPScale;
extern float gfPScaleY;
extern float gfScreenDist;
extern float gfDeltaAngle;
extern float gfFOV;
extern float gfHalfFOV;
extern int gNumRays;
extern int gHalfNumRays;
extern int gRayStep;
extern int gMaxDepth;

extern bool bTextured;

extern int gMinTexHeight;
extern int gMaxTexHeight;

typedef struct {
	float x;
	float y;
} FVEC;

#endif
