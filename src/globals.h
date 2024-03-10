/*
  vim:ts=4
  vim:sw=4
*/
#ifndef _GLOBALS_H
#define _GLOBALS_H

#define SWIDTH 320
#define SHEIGHT 240
#define HALF_SW SWIDTH / 2
#define HALF_SH SHEIGHT / 2

extern int gMode;
extern int gScreenWidth;
extern int gScreenHeight;
extern int gMapWidth;
extern int gMapHeight;
extern float gScreenDist;

typedef struct {
	float x;
	float y;
} FVEC;

#define TILESIZE 6
#define FOV M_PI/3.0
#define HALF_FOV FOV/2.0
#define NUM_RAYS SWIDTH/2
#define HALF_NUM_RAYS NUM_RAYS/2
#define DELTA_ANGLE FOV / NUM_RAYS
#define MAX_DEPTH 20
#define RAY_STEP 1

#define PSCALE 2
#define PSCALEY 1


#endif
