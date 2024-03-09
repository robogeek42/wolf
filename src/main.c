/*
  vim:ts=4
  vim:sw=4
*/
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

int gMode = 8; 
int gScreenWidth = 320;
int gScreenHeight = 240;

int gMapWidth = 16;
int gMapHeight = 9;

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

#define TILESIZE 16

#define COL(C) vdp_set_text_colour(C)
#define TAB(X,Y) vdp_cursor_tab(X,Y)

bool debug = false;
bool bShow2D = true;

typedef struct {
	float x;
	float y;
} FVEC;

// player
FVEC player_pos = {1.5f,1.5f};
float player_angle = M_PI/6.0f;


// counters
clock_t key_wait_ticks;

int key_wait = 12;

uint8_t basemap[16*9] = {
1,1,1,5,1,1,5,1,1,5,1,1,5,1,1,1,
1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
1,0,0,1,1,1,1,0,0,0,1,1,1,0,0,1,
1,0,0,0,0,0,1,0,0,0,0,0,2,0,0,1,
1,0,0,0,0,0,1,0,0,0,0,0,2,0,0,1,
1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,
1,0,0,0,0,0,0,0,0,0,0,4,0,0,0,1,
1,0,0,3,0,0,0,3,0,0,0,0,0,0,0,1,
3,3,4,3,3,3,4,3,3,4,3,3,3,3,3,3
};

// ----------------------------------
void load_images();
void game_loop();

void show_map2d();
void show_player2d();

void player_move(float x, float y);
void player_moveDir(int d);

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

	if (bShow2D)
	{
		show_map2d();
		show_player2d();
	}
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

			if (bShow2D)
			{
				show_map2d();
				show_player2d();
			}
		}

		if ( vdp_check_key_press( KEY_m ) ) // m
		{
			if ( key_wait_ticks < clock() )
			{
				key_wait_ticks = clock() + key_wait;
				bShow2D = !bShow2D;

				if (!bShow2D) {
					vdp_clear_screen();
				} else {
					show_map2d();
					show_player2d();
				}

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
    int scl = TILESIZE;
    
	for (int y = 0; y<gMapHeight; y++) {
		for (int x = 0; x<gMapWidth; x++) {
			vdp_gcol(0, basemap[y*gMapWidth + x]);
			draw_filled_box(x*scl, y*scl, scl, scl, 4, basemap[y*gMapWidth + x]*2);
		}
	}
}

void show_player2d()
{
    int scl = TILESIZE;

	draw_box2(player_pos.x*scl-2, player_pos.y*scl-2, 4, 4, 11);
	vdp_move_to(player_pos.x*scl, player_pos.y*scl);
	vdp_line_to(player_pos.x*scl+cos(player_angle)*16, player_pos.y*scl+sin(player_angle)*16);
}

void player_moveDir(int d)
{
	float c = 0.1 * cos(player_angle);
	float s = 0.1 * sin(player_angle);

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
	if (x>0 && player_pos.x<gMapWidth) { player_pos.x += x;} 
	if (y>0 && player_pos.y<gMapHeight) { player_pos.y += y;} 
	if (x<0 && player_pos.x>0) { player_pos.x += x;} 
	if (y<0 && player_pos.y>0) { player_pos.y += y;} 
}
