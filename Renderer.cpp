#include "Renderer.h"
#include "Utilities.h"
#include <assert.h>
#include "Assets.h"

Renderer::Renderer(uint8_t flags)
{
	EntitySprite.setPosition(GInfo.player_pos);
	EntitySprite.setRotation(GInfo.player_angle);
	EntitySprite.setTexture(GAssets.player2dtexture);
	EntitySprite.setOrigin({ 16,16 });

	mDotArray.setPrimitiveType(sf::TrianglesFan);

	mRenderTexture.create(WIDTH, HEIGHT);

	mBuffer = new sf::Uint8[WIDTH * HEIGHT * 4];
	mDepth = new float[WIDTH];

	mRenderState.texture = &mRenderTexture;

	//For drawing our buffer texture
	mRenderArray.setPrimitiveType(sf::Quads);
	mRenderArray.resize(4);
	sf::Vertex* temp = &mRenderArray[0];
	temp[0].position = { 0,0 };
	temp[0].texCoords = { 0,0 };
	temp[1].position = { WIDTH * SCALE,0 };
	temp[1].texCoords = { WIDTH,0 };
	temp[2].position = { WIDTH * SCALE, HEIGHT * SCALE };
	temp[2].texCoords = { WIDTH,HEIGHT };
	temp[3].position = { 0, HEIGHT * SCALE };
	temp[3].texCoords = { 0,HEIGHT };

	mFlags = flags;
}
Renderer::~Renderer()
{
	delete mBuffer;
	delete mDepth;
}
sf::Color Renderer::GetColor(int x, int y, int texture)
{
	return GAssets.TF2_Images.at(texture).getPixel(x, y);
}
void Renderer::CastRays()
{
	sf::Vector2f pos = GInfo.player_pos;
	sf::Vector2f dir = { unit_vector(GInfo.player_angle) };

	sf::Vertex vert;
	if (mFlags & RENDER2D) {
		vert.position = pos * tile_width;
		mDotArray.append(vert);
	}

	for (int x = 0; x < WIDTH; x++)
	{
		//calculate ray position and directon
		//This gives angle in radians
		float angle = atan((x - WIDTH / 2) / DIST);
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

		if (mFlags & RENDER2D) {
			vert.position = (pos + ray_dir * perp_wall_dist) * tile_width;
			mDotArray.append(vert);
		}

		if (mFlags & RENDER3D) {
			//store distance of each column
			mDepth[x] = perp_wall_dist;
			
			//removes fish eye effect
			ray_length = ray_length * cos(deg_to_rad(angle - GInfo.player_angle));

			int lineheight = (int)(HEIGHT / ray_length);
			int draw_start = -lineheight / 2 + HEIGHT / 2;
			if (draw_start < 0) draw_start = 0;
			int draw_end = lineheight / 2 + HEIGHT / 2;
			if (draw_end >= HEIGHT) draw_end = HEIGHT;

			int texture = 0;

			//Texture calculations wolfenstein
			switch (GetTile(map_tile))
			{
			case '#': texture = GAssets.STONE_WALL; break;
			case '$': texture = GAssets.WOOD_WALL; break;
			case '@': texture = GAssets.BLUE_METAL; break;
			case '&': texture = GAssets.BLUE_WALL; break;
			case '%': texture = GAssets.RED_BRICK; break;
			case '!': texture = GAssets.MOSS_WALL; break;
			}


			float wall_x;
			if (side == 0) wall_x = pos.y + perp_wall_dist * ray_dir.y;
			else		   wall_x = pos.x + perp_wall_dist * ray_dir.x;
			wall_x -= (int)wall_x;

			int tex_x = int(wall_x * TEX_WIDTH);
			if (side == 0 && ray_dir.x > 0) tex_x = TEX_WIDTH - tex_x - 1;
			if (side == 1 && ray_dir.y < 0) tex_x = TEX_WIDTH - tex_x - 1;

			float step = 64 / float(lineheight);
			float tex_pos = (draw_start - HEIGHT / 2 + lineheight / 2) * step;
			for (int y = draw_start; y < draw_end; y++)
			{
				assert(y < HEIGHT&& y >= 0);
				int tex_y = (int)tex_pos & (63);
				tex_pos += step;
				sf::IntRect image_rect = GAssets.WL_WallRects.at(texture);

				//Get shaded texture if on y side
				if (side) image_rect = { image_rect.left + 64, image_rect.top, 64, 64 };
				sf::Uint8* temp = &mBuffer[int(y * WIDTH + x) * 4];

				sf::Color color = GAssets.WL_WallImage.getPixel(image_rect.left + tex_x, image_rect.top + tex_y);

				temp[0] = color.r;
				temp[1] = color.g;
				temp[2] = color.b;
				temp[3] = color.a;
			}

		}
	}
}
sf::Color Renderer::GetSkyBoxColor(int x, int y, int x_coord)
{
	//2048 and 256 are just the width of the skybox image I choose, this must be changed
	//if using a different image
	int y_coord = y * (256 / HEIGHT);
	y_coord = int(256 - y_coord) % 256;

	sf::Color color = GAssets.skybox.getPixel((x_coord + x) % 2048, y_coord);
	return color;
}
void Renderer::FloorCasting()
{
	sf::Vector2f ray_dir0;
	sf::Vector2f ray_dir1;
	float pos_z = HEIGHT * 0.5f;
	sf::Vector2f pos = GInfo.player_pos;

	//Left angle
	float angle0 = atan((0 - WIDTH / 2) / DIST);
	angle0 = angle0 * 180 / 3.14159f;
	angle0 = GInfo.player_angle + angle0;
	ray_dir0 = unit_vector(angle0);

	//Right angle 
	float angle1 = atan((WIDTH - WIDTH / 2) / DIST);
	angle1 = angle1 * 180 / 3.14159f;
	angle1 = GInfo.player_angle + angle1;
	ray_dir1 = unit_vector(angle1);

	//Calculations for skybox
	float sky_angle = angle0;
	sky_angle -= floor(sky_angle / 360) * 360;
	//2048 and 256 are just the width of the skybox image I choose, this must be changed
	//if using a different image
	int x_coord = sky_angle * (2048.0f / 360.0f); //starting x coord in skybox

	for (int y = 0; y < HEIGHT; y++)
	{
		//Relative to horizon
		int p = y - HEIGHT / 2;
		float row_distance = pos_z / p;

		sf::Vector2f floor_step = row_distance * (ray_dir1 - ray_dir0) / WIDTH;

		sf::Vector2f floor = pos + row_distance * ray_dir0;

		for (int x = 0; x < WIDTH; x++)
		{
			sf::Vector2i cell = { (int)floor.x, (int)floor.y };

			sf::Vector2i tex_coord;
			tex_coord.x = (int)(64 * (floor.x - cell.x)) & (63);
			tex_coord.y = (int)(64 * (floor.y - cell.y)) & (63);

			floor += floor_step;

			//floor
			sf::IntRect tex = GAssets.WL_WallRects.at(GAssets.BLUE_METAL);
			sf::Uint8* temp = &mBuffer[int(y * WIDTH + x) * 4];
			sf::Color color = GAssets.WL_WallImage.getPixel(tex.left + tex_coord.x, tex.top + tex_coord.y);
			temp[0] = color.r;
			temp[1] = color.g;
			temp[2] = color.b;
			temp[3] = color.a;
			//Ceiling/skybox
			tex = GAssets.WL_WallRects.at(GAssets.BLUE_WALL);
			temp = &mBuffer[int((HEIGHT - 1 - y) * WIDTH + x) * 4];
			color = GAssets.WL_WallImage.getPixel(tex.left + tex_coord.x, tex.top + tex_coord.y);
			color = GetSkyBoxColor(x, y, x_coord);
			temp[0] = color.r;
			temp[1] = color.g;
			temp[2] = color.b;
			temp[3] = color.a;
		}
	}
}
void Renderer::Render2d()
{
	const sf::Color tile_color = sf::Color(200, 200, 200);
	sf::VertexArray verticies;
	verticies.setPrimitiveType(sf::Quads);
	for (int y = 0; y < GInfo.GameMap.size(); y++) {
		for (int x = 0; x < GInfo.GameMap.at(y).size(); x++) {
			if (GetTile({ x,y }) == ' ')
				continue;
			sf::Vertex vert;
			vert.position = { float(x * tile_width), float(y * tile_width) };
			vert.color = tile_color;
			verticies.append(vert);
			vert.position = { float(x * tile_width) + tile_width, float(y * tile_width) };
			vert.color = tile_color;
			verticies.append(vert);
			vert.position = { float(x * tile_width) + tile_width, float(y * tile_width) + tile_width };
			vert.color = tile_color;
			verticies.append(vert);
			vert.position = { float(x * tile_width), float(y * tile_width) + tile_width };
			vert.color = tile_color;
			verticies.append(vert);
		}
	}

	CastRays();

	GInfo.window->draw(mDotArray);
	mDotArray.clear();

	GInfo.window->draw(verticies);

	EntitySprite.setPosition({ GInfo.player_pos.x * tile_width, GInfo.player_pos.y * tile_width });
	EntitySprite.setRotation(GInfo.player_angle);
	GInfo.window->draw(EntitySprite);

	GInfo.window->draw(GInfo.hitscan_array);
}
void Renderer::Render3d()
{
	FloorCasting();
	CastRays();
	mRenderTexture.update(mBuffer, WIDTH, HEIGHT, 0, 0);

	//Draws scene
	GInfo.window->draw(mRenderArray, mRenderState);

	//FIX ME: Draw the gun
	//Draw gun
	//GInfo.pWeapon->Draw();
}
void Renderer::Draw()
{
	//For now, just render 3d, adding in more support later
	if (mFlags & RENDER3D)
		Render3d();
	else if (mFlags & RENDER2D)
		Render2d();
}