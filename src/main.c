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
bool bTextured = false;

// Settings
int gMode = 8; 
int gScreenWidth = 320;
int gScreenHeight = 240;
int gTileSize = 8;

float gfPScale = 4.0;  // This is the width of the vertical slices

float gfPScaleY = 1;
int gMaxDepth = 20;
int gRayStep = 1;

// calculated from settings
int gHalfScreenWidth;
int gHalfScreenHeight;
float gfScreenDist;
float gfDeltaAngle;
float gfFOV;
float gfHalfFOV;
int gNumRays;
int gHalfNumRays;

// player - starting position and direction
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

bool loaded2wide = false;
bool loaded4wide = false;
int gMinTexHeight = 7;
int gMaxTexHeight = 256;

// ----------------------------------
bool load_images(int width, int bmOffset);
void game_loop();

void show_map2d();
void show_player2d();

void player_move(float x, float y);
void player_moveDir(int d);
void raycast_update();
void test_images();

// ----------------------------------
void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

void calculate_globals()
{
	gHalfScreenWidth = gScreenWidth/2;
	gHalfScreenHeight = gScreenHeight/2;
	gNumRays = gScreenWidth / gfPScale;
	gHalfNumRays = gNumRays / 2;
	gfFOV = M_PI/3.0;
	gfHalfFOV = gfFOV / 2;
	gfDeltaAngle = gfFOV / gNumRays;
	gfScreenDist = gHalfScreenWidth / tan(gfHalfFOV);

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

	TAB(0,0); COL(11);
	printf("Wolf3D demo\n\n");
	COL(15);
	printf("W/S - move forward/backward\n");
	printf("A/D - slide left/right\n");
	printf("Arrows - turn to face left/right\n");
	printf("\n");
	printf("M - turn off/on mini-map\n");
	printf("T - turn on/off texturing\n");
	printf("P - choose 4 wide or 2 wide slices\n");
	printf("\n");

	calculate_globals();
	//printf("%dx%d rays %d slice-width %f\n",gScreenWidth, gScreenHeight, gNumRays, gfPScaleY);
	//wait();

	if (bMapRight)
	{
		mapxoff = gScreenWidth - (gTileSize*gMapWidth);
	}
	if (bMapBottom)
	{
		mapyoff = gScreenHeight - (gTileSize*gMapHeight);
	}
    
	int imgWidth = (int)gfPScale;
	int offset = 256*((imgWidth/2)-1);
	COL(1);
	printf("Loading images scale %f (w=%d, off=%d)\n",gfPScale, imgWidth, offset);
	if( ! load_images(imgWidth, offset ))
	{
		printf("Failed to load images scale %f\n",gfPScale);
		wait();
		return -1;
	}
	//printf("\ngpfScale=%f\n",gfPScale);
	//if (gfPScale == 2.0f) // weird that this and the other if evaluates as true!
	if (imgWidth == 2)
	{
		loaded2wide=true; 
		//printf("loaded 2-wide\n");
	}
	//if (gfPScale == 4.0f) // also true!
	if (imgWidth == 4)
	{
		loaded4wide=true; 
		//printf("loaded 4-wide\n");
	}

	COL(15);
	printf("\nPress any key\n");
	wait();
	vdp_clear_screen();
	//test_images();
	//wait();

	// go double buffered
	vdp_mode(gMode+128);

	game_loop();

	vdp_mode(0);
	vdp_logical_scr_dims(true);
	vdp_cursor_enable( true );
	return 0;
}

void game_loop()
{
	int exitLoop=0;
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
		if ( vdp_check_key_press( KEY_t ) ) // to to switch to "textured" mode
		{
			if ( key_wait_ticks < clock() )
			{
				key_wait_ticks = clock() + key_wait;
				bTextured = !bTextured;
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
		if ( vdp_check_key_press( KEY_p ) ) // change slice width
		{
			if ( key_wait_ticks < clock() )
			{
				key_wait_ticks = clock() + key_wait;
				TAB(0,0);
				printf("Switch\n");
				vdp_swap();
				int imgWidth = (int)gfPScale;
				if (imgWidth == 2) 
				{
					gfPScale = 4.0;
					calculate_globals();
					if (!loaded4wide)
					{
						printf("LOAD 4 WIDE IMAGES\n");
						vdp_swap();
						if (!load_images(4,256))
						{
							printf("Failed to load images scale %f\n",gfPScale);
							vdp_swap();
							wait();
							exitLoop=true;
						}
						loaded4wide=true;	
					}
				} else {
					gfPScale = 2.0;
					calculate_globals();
					if (!loaded2wide)
					{
						printf("LOAD 2 WIDE IMAGES\n");
						vdp_swap();
					
						if (!load_images(2,0))
						{
							printf("Failed to load images scale %f\n",gfPScale);
							vdp_swap();
							wait();
							exitLoop=true;
						}
						loaded2wide=true;	
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
		}
		if ( vdp_check_key_press( KEY_x ) ) { // x
			exitLoop=1;
		}

		vdp_update_key_state();
	} while (exitLoop==0);

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
	TAB(37,0);printf("P=%.0f",gfPScale);
}

bool load_images(int width, int bmOffset) 
{
	if (width != 2 && width !=4) return false;
	int ret = 0;
	char fname[60];

	for (int fn=gMinTexHeight; fn<=gMaxTexHeight; fn++)
	{
		sprintf(fname, "img/tex1_%dwide/gradblue%dx%03d.rgb2", width, width, fn);
		ret = load_bitmap_file(fname, width, fn, fn+bmOffset);
		if (ret <0) return false;
		printf(".");
	}
	return true;
}

void test_images()
{
	int bmOffset = 0;
	int imgWidth = (int)gfPScale;
	if (imgWidth == 4)
	{
		bmOffset=256;
	}

	for (int height=gMinTexHeight; height<=(gScreenWidth-gMinTexHeight); height++)
	{
		vdp_adv_select_bitmap(height+bmOffset);
		vdp_draw_bitmap(height*imgWidth-imgWidth/2,120 - (height/2));
	}
}

