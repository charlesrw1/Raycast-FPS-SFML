#include "Renderer.h"
#include "Utilities.h"
#include <assert.h>
#include "Assets.h"
#include <cmath>

Renderer::Renderer()
{
	EntitySprite.setPosition(GInfo.player_pos);
	EntitySprite.setRotation(GInfo.player_angle);
	EntitySprite.setTexture(GAssets.player2dtexture);
	EntitySprite.setOrigin({ 16,16 });

	mDotArray.setPrimitiveType(sf::TrianglesFan);
	float width = GUser.bWidth, height = GUser.bHeight;
	float scale = GUser.winScale;

	mRenderTexture.create(width, height);

	mBuffer = new sf::Uint8[width * height * 4];
	mDepth = new float[width];

	mRenderState.texture = &mRenderTexture;

	//For drawing our buffer texture
	mRenderArray.setPrimitiveType(sf::Quads);
	mRenderArray.resize(4);
	sf::Vertex* temp = &mRenderArray[0];
	temp[0].position = { 0,0 };
	temp[0].texCoords = { 0,0 };
	temp[1].position = { width * scale,0 };
	temp[1].texCoords = { width,0 };
	temp[2].position = { width * scale, height * scale };
	temp[2].texCoords = { width,height };
	temp[3].position = { 0, height * scale };
	temp[3].texCoords = { 0,height };

	mMiniMap.setPrimitiveType(sf::Quads);
	InitMapVerticies(GUser.tileWidth);

	mRayHits.resize(GUser.bWidth);
}
Renderer::~Renderer()
{
	delete mBuffer;
	delete mDepth;
}
void Renderer::InitMapVerticies(int tileWidth)
{
	mMiniMap.clear();
	const sf::Color tile_color = sf::Color(200, 200, 200);
	for (int y = 0; y < GInfo.GameMap.size(); y++) {
		for (int x = 0; x < GInfo.GameMap.at(y).size(); x++) {
			if (GetTile({ x,y }) == ' ')
				continue;
			sf::Vertex vert;
			vert.position = { float(x * tileWidth), float(y * tileWidth) };
			vert.color = tile_color;
			mMiniMap.append(vert);
			vert.position = { float(x * tileWidth) + tileWidth, float(y * tileWidth) };
			vert.color = tile_color;
			mMiniMap.append(vert);
			vert.position = { float(x * tileWidth) + tileWidth, float(y * tileWidth) + tileWidth };
			vert.color = tile_color;
			mMiniMap.append(vert);
			vert.position = { float(x * tileWidth), float(y * tileWidth) + tileWidth };
			vert.color = tile_color;
			mMiniMap.append(vert);
		}
	}
}
sf::Color Renderer::GetColor(int x, int y, int texture)
{
	return GAssets.TF2_Images.at(texture).getPixel(x, y);
}

void Renderer::CastRays(float bufWidth, float bufHeight, float displayDist)
{
	sf::Vector2f pos = GInfo.player_pos;
	sf::Vector2f dir = { unit_vector(GInfo.player_angle) };

	sf::Vertex vert;

	for (int x = 0; x < bufWidth; x++)
	{
		RayHit& curRay = mRayHits[x];
		
		//calculate ray position and directon
		//This gives angle in radians
		float angle = atan((x - bufWidth / 2) / displayDist);
		//To degrees
		angle = angle * 180 / 3.14159f;
		//Relative to player
		angle = GInfo.player_angle + angle;

		//Old way, gives warping effect on edges
		//float ray_dir_1d = GInfo.player_angle - FOV_HORIZONTAL * (WIDTH * 0.5 - x) / WIDTH - 1;

		sf::Vector2f ray_dir = { unit_vector(angle) };

		//coordinates of square ray is in
		sf::Vector2i map_tile = { int(pos.x), int(pos.y) };

		//inital distance to closest x/y side, accumulated
		sf::Vector2f side_dist;

		//distance ray travels to x/y side
		sf::Vector2f delta_dist;
		delta_dist.x = sqrt(1 + pow(ray_dir.y / ray_dir.x, 2));
		delta_dist.y = sqrt(1 + pow(ray_dir.x / ray_dir.y, 2));

		//direction to step map tile
		sf::Vector2i step;

		if (ray_dir.x < 0)
		{
			step.x = -1;
			side_dist.x = (pos.x - float(map_tile.x)) * delta_dist.x;
		}
		else
		{
			step.x = 1;
			side_dist.x = (float(map_tile.x + 1) - pos.x) * delta_dist.x;
		}
		if (ray_dir.y < 0)
		{
			step.y = -1;
			side_dist.y = (pos.y - float(map_tile.y)) * delta_dist.y;
		}
		else
		{
			step.y = 1;
			side_dist.y = (float(map_tile.y + 1) - pos.y) * delta_dist.y;
		}
		bool hit = false;
		//0 for x, 1 for y
		bool side = 0;
		float max = 20.0f;
		float ray_length = 0;
		while (!hit)
		{
			if (side_dist.x < side_dist.y)
			{
				ray_length = side_dist.x;
				map_tile.x += step.x;
				side_dist.x += delta_dist.x;
				side = 0;
			}
			else
			{
				ray_length = side_dist.y;
				map_tile.y += step.y;
				side_dist.y += delta_dist.y;
				side = 1;
			}
			hit = (GetTile(map_tile) != ' ');
		}
		float perp_wall_dist;
		if (side == 0)  perp_wall_dist = side_dist.x - delta_dist.x;
		else			perp_wall_dist = side_dist.y - delta_dist.y;

		ray_length = ray_length * cos(deg_to_rad(angle - GInfo.player_angle));

		curRay.angle = angle;
		curRay.map_tile = map_tile;
		curRay.perp_wall_dist = perp_wall_dist;
		curRay.ray_dir = ray_dir;
		curRay.ray_length = ray_length;
		curRay.side = side;	
	}
}
sf::Color Renderer::GetSkyBoxColor(int x, int y)
{
	//2048 and 256 are just the width of the skybox image I choose, this must be changed
	//if using a different image
	int y_coord = y * 256 / GUser.bHeight;
	//y_coord = int(256 - y_coord) % 256;

	return GAssets.skybox.getPixel((x) % 2040, y_coord);
}

void Renderer::FloorCasting(float bufWidth, float bufHeight, float displayDist)
{
	//no floor casting, reset pixels
	if (!GUser.drawFloor) {
		for (int y = bufHeight/2; y < bufHeight; y++) {
			
			for (int x = 0; x < bufWidth; x++) {
				sf::Uint8* temp = &mBuffer[(int(y * bufWidth) + x) * 4];
				temp[0] = 110;
				temp[1] = 110;
				temp[2] = 110;
				temp[3] = 255;
			}
		}
		return;
	}
	
	sf::Vector2f ray_dir0;
	sf::Vector2f ray_dir1;
	float pos_z = bufHeight * 0.5f;
	sf::Vector2f pos = GInfo.player_pos;

	//Left angle
	float angle0 = GUser.FOV / 2;
	angle0 = GInfo.player_angle - angle0;
	ray_dir0 = unit_vector(angle0);

	//Right angle 
	float angle1 = GUser.FOV / 2;
	angle1 = GInfo.player_angle + angle1;
	ray_dir1 = unit_vector(angle1);

	//Calculations for skybox
	float sky_angle = angle0;
	sky_angle -= floor(sky_angle / 360) * 360;
	//2048 and 256 are just the width of the skybox image I choose, this must be changed
	//if using a different image
	int x_coord = (sky_angle/360.0f) * 2040; //starting x coord in skybox

	for (int y = bufHeight/2; y < bufHeight; y++)
	{
		//Relative to horizon
		int p = y - bufHeight/2;
		float row_distance = pos_z / p;
		//hack to fix scaling issue
		row_distance *= 90.0f/GUser.FOV;
		sf::Vector2f floor_step = row_distance * (ray_dir1 - ray_dir0) / bufWidth;

		sf::Vector2f floor = pos + row_distance * ray_dir0;

		for (int x = 0; x < bufWidth; x++)
		{
			sf::Vector2i cell = { (int)floor.x, (int)floor.y };

			sf::Vector2i tex_coord;
			tex_coord.x = (int)(64 * (floor.x - cell.x)) & (63);
			tex_coord.y = (int)(64 * (floor.y - cell.y)) & (63);

			floor += floor_step;

			//floor
			sf::IntRect tex = GAssets.WL_WallRects.at(GAssets.BLUE_METAL);
			sf::Uint8* temp = &mBuffer[int(y * bufWidth + x) * 4];
			sf::Color color= GAssets.WL_WallImage.getPixel(tex.left + tex_coord.x, tex.top + tex_coord.y);
			color = GAssets.ground.getPixel(0 + tex_coord.x, 0 + tex_coord.y);
			temp[0] = color.r*0.9;
			temp[1] = color.g*0.9;
			temp[2] = color.b*0.9;
			temp[3] = color.a;
			
			
			//Ceiling/skybox 
			/*
			tex = GAssets.WL_WallRects.at(GAssets.BLUE_WALL);
			temp = &mBuffer[int((bufHeight - 1 - y) * bufWidth + x) * 4];
			color = GAssets.WL_WallImage.getPixel(tex.left + tex_coord.x, tex.top + tex_coord.y);
			color = GetSkyBoxColor(x, y, x_coord);
			temp[0] = color.r;
			temp[1] = color.g;
			temp[2] = color.b;
			temp[3] = color.a;
			*/
		}
	}
}
void Renderer::DrawSkybox()
{
	float angle0 = GInfo.player_angle;
	int bufHeight = GUser.bHeight;
	int bufWidth = GUser.bWidth;
	angle0 -= floor(angle0 / 360) * 360;
	//2048 and 256 are just the width of the skybox image I choose, this must be changed
	//if using a different image
	int x_coord = (angle0 / 360.0f) * 2048; //starting x coord in skybox

	for (int x = 0; x < bufWidth; x++)
	{
		float angle = mRayHits.at(x).angle;
		angle -= floor(angle / 360) * 360;
		int xpos = (angle / 360.0f) * 2048.0f;
		for (int y = 0; y < bufHeight / 2; y++) 
		{
			sf::Color color = GetSkyBoxColor(xpos, y);
			sf::Uint8* temp = &mBuffer[int(y * bufWidth + x) * 4];
			temp[0] = color.r;
			temp[1] = color.g;
			temp[2] = color.b;
			temp[3] = color.a;
		}
	}
}
//Temporary
float GetIntensity(float distance)
{
	float intens[7] = { 1.0,0.8,0.6,0.4,0.2,0.1,0.0 };
	int index = distance / 8.0f;
	index = std::min(index, 6);
	return intens[index];
}

void Renderer::DrawWalls(float bufWidth, float bufHeight, float displayDist, int tileWidth, int textureSize)
{
	sf::Vector2f pos = GInfo.player_pos;
	for (int x = 0; x < GUser.bWidth; x++)
	{
		const RayHit& ray = mRayHits.at(x);

		mDepth[x] = ray.perp_wall_dist;

		//removes fish eye effect

		int lineheight = round(displayDist / ray.ray_length);
		int draw_start = -lineheight / 2 + bufHeight / 2;
		if (draw_start < 0) draw_start = 0;
		int draw_end = lineheight / 2 + bufHeight / 2;
		if (draw_end >= bufHeight) draw_end = bufHeight;

		int texture = 0;

		//Texture calculations wolfenstein
		switch (GetTile(ray.map_tile))
		{
		case '#': texture = GAssets.STONE_WALL; break;
		case '$': texture = GAssets.WOOD_WALL; break;
		case '@': texture = GAssets.BLUE_METAL; break;
		case '&': texture = GAssets.BLUE_WALL; break;
		case '%': texture = GAssets.RED_BRICK; break;
		case '!': texture = GAssets.MOSS_WALL; break;
		}


		float wall_x;
		if (ray.side == 0) wall_x = pos.y + ray.perp_wall_dist * ray.ray_dir.y;
		else		   wall_x = pos.x + ray.perp_wall_dist * ray.ray_dir.x;
		wall_x -= (int)wall_x;

		int tex_x = int(wall_x * textureSize);
		if (ray.side == 0 && ray.ray_dir.x > 0) tex_x = textureSize - tex_x - 1;
		if (ray.side == 1 && ray.ray_dir.y < 0) tex_x = textureSize - tex_x - 1;

		float texture_step = 64 / float(lineheight);
		float tex_pos = (draw_start - bufHeight / 2 + lineheight / 2) * texture_step;
		
		float inten = GetIntensity(ray.perp_wall_dist);
		for (int y = draw_start; y < draw_end; y++)
		{
			assert(y < bufHeight&& y >= 0);
			int tex_y = (int)tex_pos & (63);
			tex_pos += texture_step;
			sf::IntRect image_rect = GAssets.WL_WallRects.at(texture);

			//Get shaded texture if on y side
			if (ray.side) image_rect = { image_rect.left + 64, image_rect.top, 64, 64 };
			sf::Uint8* temp = &mBuffer[int(y * bufWidth + x) * 4];

			sf::Color color = GAssets.WL_WallImage.getPixel(image_rect.left + tex_x, image_rect.top + tex_y);
			temp[0] = color.r * inten;
			temp[1] = color.g * inten;
			temp[2] = color.b * inten;
			temp[3] = color.a;
		}
	}

}
SpriteData Renderer::GetSpriteScreenPos(const Entity& sprite)
{
	sf::RectangleShape shape;
	shape.setFillColor(sf::Color::Red);

	SpriteData sData{};
	sf::Vector2f playerPos = GInfo.player_pos;
	float playerAngle = GInfo.player_angle;

	float dx = sprite.pos.x - playerPos.x;
	float dy = sprite.pos.y - playerPos.y;

	float dist = sqrt(dx * dx + dy * dy);

	float spriteAngle = atan2(dy, dx)-deg_to_rad(playerAngle-90);
	float size = GUser.displayDist / (cos(spriteAngle) * dist);
	int x = tan(spriteAngle) * GUser.displayDist;
	spriteAngle *= (180 / 3.14159);
	float halved = GUser.FOV / 2;
	//Angle is within viewcone
	if (spriteAngle <= -180)
		spriteAngle += 360;

	printf("x: %i, Sprite angle: %f, playerangle: %f\r", x, spriteAngle, playerAngle);
	if (spriteAngle >= -80 && spriteAngle <= 80) {
		sData.visible = true;
		sData.distance = dist;
		sData.x_pos = x;

		shape.setSize({ size,size });
		shape.setPosition({ (GUser.bWidth/2 + x)*GUser.winScale,(GUser.bHeight / 2.0f)*GUser.winScale - size*0.2f });
		GInfo.window->draw(shape);
	}
	return sData;
}
void Renderer::DrawSprites()
{
	for (const auto& sprite : GInfo.entity_list) {
		SpriteData sData = GetSpriteScreenPos(sprite);
		if (!sData.visible)
			continue;
		for (int x = sData.x_pos - 32; x < sData.x_pos + 32; x++) {
			if (x < -(GUser.bWidth / 2) || x >(GUser.bWidth / 2))
				continue;
			
		}
	}
}
void Renderer::DrawMap()
{
	sf::Vertex vert;
	mDotArray.clear();
	vert.position = GInfo.player_pos * GUser.tileWidth;
	mDotArray.append(vert);
	
	for (const auto& ray : mRayHits) {
		vert.position = (GInfo.player_pos + ray.ray_dir * ray.perp_wall_dist) * GUser.tileWidth;
		mDotArray.append(vert);
	}
	
	GInfo.window->draw(mDotArray);
	mDotArray.clear();

	GInfo.window->draw(mMiniMap);

	EntitySprite.setPosition({ GInfo.player_pos.x * GUser.tileWidth, GInfo.player_pos.y * GUser.tileWidth });
	EntitySprite.setRotation(GInfo.player_angle);
	GInfo.window->draw(EntitySprite);

	GInfo.window->draw(GInfo.hitscan_array);
	sf::CircleShape circle;
	circle.setFillColor(sf::Color::Red);
	circle.setRadius(2);
	for (const auto& e : GInfo.entity_list) {
		circle.setPosition(e.pos*GUser.tileWidth);
		GInfo.window->draw(circle);
	}
}
void Renderer::Render3d()
{
	CastRays(GUser.bWidth, GUser.bHeight, GUser.displayDist);
	FloorCasting(GUser.bWidth, GUser.bHeight, GUser.displayDist);
	DrawSkybox();
	DrawWalls(GUser.bWidth, GUser.bHeight, GUser.displayDist, GUser.tileWidth, GUser.texWidth);
	mRenderTexture.update(mBuffer, GUser.bWidth, GUser.bHeight, 0, 0);
	GInfo.window->draw(mRenderArray, mRenderState);
	DrawSprites();
	//DrawUI
}
void Renderer::Draw()
{
	Render3d();
	if (GUser.drawMiniMap) {
		DrawMap();
	}
}