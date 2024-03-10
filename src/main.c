/*
  vim:ts=4
  vim:sw=4
*/
#include "globals.h"
#include "colmap.h"

#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "util.h"
#include "raycasting.h"

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

bool debug = false;
bool bShow2D = true;

int gMode = 8;//+128; 
int gScreenWidth = 320;
int gScreenHeight = 240;
int gTileSize = 8;
int gHalfScreenWidth;
int gHalfScreenHeight;

float gfPScale;
float gfPScaleY;
float gfScreenDist;
float gfDeltaAngle;
float gfFOV;
float gfHalfFOV;
int gNumRays;
int gHalfNumRays;
int gRayStep;
int gMaxDepth;

// player
FVEC player_pos = {1.5f,1.5f};
float player_angle = M_PI/6.0f;

// counters
clock_t key_wait_ticks;

int key_wait = 12;

int gMapWidth = 16;
int gMapHeight = 9;
uint8_t basemap[16*9] = {
1,1,1,5,1,1,5,1,1,5,1,1,5,1,1,1,
1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
1,0,0,2,1,1,1,0,0,0,1,1,1,0,0,1,
1,0,0,0,0,0,1,0,0,0,0,0,2,0,0,1,
1,0,0,0,0,0,1,0,0,0,0,0,2,0,0,1,
1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,
1,0,0,0,0,0,0,0,0,0,0,4,0,0,0,1,
1,0,0,3,0,0,0,3,0,0,0,0,0,0,0,1,
3,3,4,3,3,3,4,3,3,4,3,3,3,3,3,3
};
bool bMapRight = true;
bool bMapBottom = true;
int mapxoff = 0;
int mapyoff = 0;

// ----------------------------------
void load_images();
void game_loop();

void show_map2d();
void show_player2d();

void player_move(float x, float y);
void player_moveDir(int d);
void raycast_update();

// ----------------------------------
void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );


	// setup complete
	vdp_mode(gMode);
	vdp_logical_scr_dims(false);
	//vdu_set_graphics_viewport()

	gHalfScreenWidth = gScreenWidth/2;
	gHalfScreenHeight = gScreenHeight/2;
	gfPScale = 2.0;
	gfPScaleY = 1;
	gNumRays = gScreenWidth / gfPScale;
	gHalfNumRays = gNumRays / 2;
	gfFOV = M_PI/3.0;
	gfHalfFOV = gfFOV / 2;
	gfDeltaAngle = gfFOV / gNumRays;
	gfScreenDist = gHalfScreenWidth / tan(gfHalfFOV);
	gNumRays = gScreenWidth / gfPScale;
	gMaxDepth = 20;
	gRayStep = 1;

	//printf("%dx%d NUM_RAYS %d PSCALE %d\n",SWIDTH, SHEIGHT, NUM_RAYS, PSCALE);
	//wait();

	if (bMapRight)
	{
		mapxoff = gScreenWidth - (gTileSize*gMapWidth);
	}
	if (bMapBottom)
	{
		mapyoff = gScreenHeight - (gTileSize*gMapHeight);
	}
    
	load_images();

	game_loop();

	vdp_mode(0);
	vdp_logical_scr_dims(true);
	vdp_cursor_enable( true );
	return 0;
}

void game_loop()
{
	int exit=0;
	key_wait_ticks = clock();

	raycast_update();
	if (bShow2D)
	{
		show_map2d();
		show_player2d();
	}
	vdp_swap();
	do {
		int move_dir = -1;
		int view_dir = -1;

		// Check keys
		// Move position in map
		if ( vdp_check_key_press( KEY_w ) ) { move_dir=UP; }
		if ( vdp_check_key_press( KEY_a ) ) { move_dir=LEFT; }
		if ( vdp_check_key_press( KEY_s ) ) { move_dir=DOWN; }
		if ( vdp_check_key_press( KEY_d ) ) { move_dir=RIGHT; }

		// cursor movement - change view direction
		if ( vdp_check_key_press( KEY_LEFT ) )  {view_dir=LEFT; }
		if ( vdp_check_key_press( KEY_RIGHT ) ) {view_dir=RIGHT; }
		
		// action the movements
		if ((move_dir>=0 || view_dir>=0)  && ( key_wait_ticks < clock() ) ) {
			key_wait_ticks = clock() + key_wait;

			if (move_dir>=0)
			{
				player_moveDir(move_dir);
			}
			if (view_dir>=0)
			{
				if (view_dir==LEFT) {
					player_angle -= M_PI/36.0f;
					if (player_angle<0) player_angle += 2*M_PI;
				}
				if (view_dir==RIGHT) {
					player_angle += M_PI/36.0f;
					if (player_angle>2*M_PI) player_angle -= 2*M_PI;
				}
			}

			vdp_clear_screen();
			raycast_update();
			if (bShow2D)
			{
				show_map2d();
				show_player2d();
			}
			vdp_swap();
		}

		if ( vdp_check_key_press( KEY_m ) ) // m
		{
			if ( key_wait_ticks < clock() )
			{
				key_wait_ticks = clock() + key_wait;
				bShow2D = !bShow2D;

				vdp_clear_screen();
				raycast_update();
				if (bShow2D)
				{
					show_map2d();
					show_player2d();
				}
				vdp_swap();
			}
		}
		if ( vdp_check_key_press( KEY_x ) ) { // x
			exit=1;
		}

		vdp_update_key_state();
	} while (exit==0);

}


void load_images() 
{
	/*
	char fname[40];
	for (int fn=1; fn<=NUM_FILES; fn++)
	{
		sprintf(fname, "file%d.rgb2", fn);
		load_bitmap_file(fname, 1, 16, fn-1);
	}
	*/
}

void show_map2d()
{
	for (int y = 0; y<gMapHeight; y++) {
		for (int x = 0; x<gMapWidth; x++) {
			draw_filled_box(
					mapxoff+x*gTileSize, 
					mapyoff+y*gTileSize, 
					gTileSize, gTileSize, 4, basemap[y*gMapWidth + x]);
		}
	}
}

void show_player2d()
{
	int boxsize = MAX(1,gTileSize/8);
	int linesize = MAX(8,gTileSize);

	draw_box2(
			mapxoff+player_pos.x*gTileSize-boxsize/2, 
			mapyoff+player_pos.y*gTileSize-boxsize/2, 
			boxsize, boxsize, 11);
	// draw view direction
	vdp_gcol(0, 6);
	vdp_move_to(
			mapxoff+player_pos.x*gTileSize, 
			mapyoff+player_pos.y*gTileSize);
	vdp_line_to(
			mapxoff+player_pos.x*gTileSize+cos(player_angle)*linesize, 
			mapyoff+player_pos.y*gTileSize+sin(player_angle)*linesize);
}

void player_moveDir(int d)
{
	float c = 0.3 * cos(player_angle);
	float s = 0.3 * sin(player_angle);

	switch (d) {
		case UP: // forward
			player_move(c,s);
			break;
		case LEFT: // left
			player_move(s,-1.0*c);
			break;
		case DOWN: // backward
			player_move(-1.0*c,-1.0*s);
			break;
		case RIGHT: // right
			player_move(-1.0*s,c);
			break;
	}
}

void player_move(float x, float y)
																			   {
	if (x>0 && player_pos.x < (gMapWidth-1)) { player_pos.x += x;} 
	if (y>0 && player_pos.y < (gMapHeight-1)) { player_pos.y += y;} 
	if (x<0 && player_pos.x > 1) { player_pos.x += x;} 
	if (y<0 && player_pos.y > 1) { player_pos.y += y;} 
}

void raycast_update()
{
	cast(&player_pos, player_angle, basemap);
}
