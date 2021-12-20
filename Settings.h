#ifndef SETTINGS_H
#define SETTINGS_H
#include "Utilities.h"
struct Settings{
	int bWidth			= 300;
	int bHeight			= 200;
	float winScale		= 4.0f;
	int winWidth;		
	int winHeight;
	float FOV			= 80.0f;
	float displayDist;
	int texWidth		= 64;
	int tileWidth		= 8;
	float moveSpeed		= 0.1f;
	float maxSpeed		= 0.1f;
	int stripWidth		= 2;

	bool drawMiniMap	= true;
	bool drawFloor		= false;
	bool drawSkybox		= true;
	bool drawWalls		= true;
};

inline float GetDisplayDist(float fov, int width) {
	return (width / 2 / tan(deg_to_rad(fov) / 2));
}

//How many pixels to render
const float WIDTH = 300;
const float HEIGHT = 200;
//distance to view plane
//width and dist being the same means 90 fov
const float DIST = 250.0f;
const int TEX_WIDTH = 64;
const int tile_width = 4;
//Scale of window
const float SCALE = 4.0f;
const float MOVE_SPEED = 0.1f;
const float MAX_SPEED = 0.1f;

extern Settings GUser;

#endif // !SETTINGS_H