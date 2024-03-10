/*
  vim:ts=4
  vim:sw=4
*/
#ifndef _GLOBALS_H
#define _GLOBALS_H


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

typedef struct {
	float x;
	float y;
} FVEC;

#endif
