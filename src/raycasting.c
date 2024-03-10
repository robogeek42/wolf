/*
  vim:ts=4
  vim:sw=4
*/
#include "raycasting.h"

void cast(FVEC *player_pos, float player_angle, uint8_t *basemap) 
{
	float ox = player_pos->x; // position of player
	float oy = player_pos->y; 
	float x_map = floorf(ox);			// map position of top corner containing player
	float y_map = floorf(oy);
	
	uint8_t texture_vert = 1;
	uint8_t texture_hor = 1;
	//this.cast_result = [];
	
	float ray_angle = player_angle - HALF_FOV + 0.01;

	for (int ray=0; ray<NUM_RAYS; ray+=RAY_STEP) {
		float sin_a = sin(ray_angle);
		float cos_a = cos(ray_angle);

		// intersections with horizontals
		// x_hor and y_hot are point of intersection at next horizontal
		// dx and dy are steps required to reach next horizontal
		// depth_vert is distance along to intersection with next horizontal
		// and delta_depth is distance ray travels in next step

		float y_hor = y_map - 1e-6;
		float dy = -1;
		if (sin_a > 0) {
			y_hor = y_map + 1;
			dy = 1;
		}
		
		float depth_hor = (y_hor - oy) / sin_a;
		float x_hor = ox + depth_hor * cos_a;
		
		float delta_depth = dy / sin_a;
		float dx = delta_depth * cos_a;			
		
		for (int i=0; i<MAX_DEPTH; i++) {
			int mx = MAX(0,MIN(gMapWidth -1, (int)floorf(x_hor)));
			int my = MAX(0,MIN(gMapHeight -1, (int)floorf(y_hor)));
			if (basemap[my*gMapWidth + mx] > 0) {
				texture_hor = basemap[my*gMapWidth +mx];
				break;
			}
			x_hor += dx;
			y_hor += dy;
			depth_hor += delta_depth;
			
		}
		
		// intersections with verticals
		// x_vert and y_vert are point of intersection at next vertical
		// dx and dy are steps required to reach next vertical
		// depth_vert is distance along to intersection with next vertical
		// and delta_depth is distance ray travels in next step
		
		float x_vert = x_map - 1e-6;
		dx = -1;
		if (cos_a > 0) {
			x_vert = x_map + 1;
			dx = 1;
		}
		
		float depth_vert = (x_vert - ox) / cos_a;
		float y_vert = oy + depth_vert * sin_a;
		
		delta_depth = dx / cos_a;
		dy = delta_depth * sin_a;

		for (int i=0; i<MAX_DEPTH; i++) {
			int mx = MAX(0,MIN(gMapWidth -1, (int)floorf(x_vert)));
			int my = MAX(0,MIN(gMapHeight -1, (int)floorf(y_vert)));
			//text("lookup "+mx+","+my, 400,370+i*8);
			if (basemap[my*gMapWidth +mx] > 0) {
				texture_vert = basemap[my*gMapWidth +mx];
				break;
			}
			x_vert += dx;
			y_vert += dy;
			depth_vert += delta_depth;
			
		}
		
		// depth
		float depth;
		uint8_t tex;
		int8_t offset;
		if (depth_vert < depth_hor) {
			depth = depth_vert;
			//y_vert %= 1; // fractional part (textures cover wall size 1)
			y_vert = y_vert - floorf(y_vert);
			tex = texture_vert;
			if (cos_a > 0) {
				offset = y_vert;
			} else {
				offset = 1 - y_vert;
			}
				
		} else {
			depth = depth_hor;
			//x_hor %= 1; // fractional part (textures cover wall size 1)
			x_hor = x_hor - floorf(x_hor);
			tex = texture_hor;
			if (sin_a > 0) {
				offset = 1 - x_hor;
			} else {
				offset = x_hor;
			}
		}
		
		depth *= cos(player_angle - ray_angle);
		
		// projection
		float proj_height = PSCALEY * gScreenDist / (depth +0.001);
		//printf("%d: %d %f\n",ray, ray*PSCALE,  proj_height);

		//this.cast_result.push({r:ray, d:depth, p:abs(proj_height), t:tex, o:offset});
		
		// // 3D cast - simple shaded rectangle
		// int depth_col = 255 / (1+depth)*3;
		//fill(depth_col);noStroke(); //stroke(100);strokeWeight(1);
		int col = texture_vert*2;
		draw_filled_box_centre(ray * PSCALE, HALF_SH, PSCALE, proj_height,col,col);
		
		ray_angle += DELTA_ANGLE * RAY_STEP;
	}
}

/*
void getObjectsToRender() {
	this.objects = [];
	for (let cr of this.cast_result) {
		//print("cast: "+cr.r+" "+cr.d+" "+cr.p+" "+cr.t+" "+cr.o+" ");
		
		//let texindex;
		let wall_column;
		let wall_posx, wall_posy;
		
		//wall_column = tex.get(cr.o * (TSIZE-PSCALE/2), 0, PSCALE*RAY_STEP, TSIZE);
		wall_column = floor(cr.o * TSIZE/2);
		wall_posx = cr.r * PSCALE;
		wall_posy = HALF_SH - cr.p / 2;
		
		this.objects.push({d:cr.d, wc:wall_column, x:wall_posx, y:wall_posy, w:PSCALE*RAY_STEP, h:cr.p, t:cr.t});
		
	}
}
*/

