#include <SFML/graphics.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <assert.h>

struct GameInfo
{
	std::vector<std::string> GameMap;
	sf::Vector2f player_pos;
	float player_angle;
	sf::RenderWindow* window;
	sf::View camera;
	sf::Texture player2dtexture;
	sf::Sprite player2dsprite;

	//Rendering
	//Wolfenstein---
	sf::Image wall_image;
	std::vector<sf::IntRect> wall_rects;
	//TF2-----------
	std::vector<sf::Image> game_images;
	//--------------

	//Used for 2d viewcone
	sf::VertexArray dot_array;

	sf::Uint8* buffer;
	//Storing depth of line segments to render sprites
	float* depth;

	sf::Texture render_texture;
	sf::VertexArray render_tex_array;

	sf::Image skybox;
	
	enum Wolfenstein 
	{
		STONE_WALL,
		WOOD_WALL,
		BLUE_METAL,
		BLUE_WALL,
		RED_BRICK,
		BRICK_EMBLEM,
		MOSS_WALL,
	};
	enum TF2
	{
		WOOD1,
		METAL1,
		CONCRETE1,
		CONCRETE2,
		WOOD2,
	};

}GInfo;
//How many pixels to render
const float WIDTH = 250;
const float HEIGHT = 250;
//distance to view plane
//width and dist being the same means 90 fov
float DIST = 250.0f;
const int TEX_WIDTH = 64;
const int tile_width = 8;
//Scale of window
const float SCALE = 4.0f;
const float MOVE_SPEED = 0.1f;
const float MAX_SPEED = 0.1f;

sf::Vector2f operator*(sf::Vector2f vec1, sf::Vector2f vec2)
{
	return sf::Vector2f({ vec1.x * vec2.x, vec1.y * vec2.y });
}
sf::Vector2f operator*(sf::Vector2f vec1, float num)
{
	return sf::Vector2f({ vec1.x * num, vec1.y * num });
}
sf::Color operator/(sf::Color col, int div)
{
	return sf::Color(col.r / div, col.g / div, col.b / div);
}
char& GetTile(sf::Vector2i vec) 
{ 
	assert(vec.y >= 0 && vec.y < GInfo.GameMap.size());
	assert(vec.x >= 0 && vec.x < GInfo.GameMap.at(vec.y).size());

	return GInfo.GameMap.at(vec.y).at(vec.x); 
}
float deg_to_rad(float deg)
{
	return deg * (3.1415 / 180);
}
sf::Vector2f unit_vector(float deg_angle)
{
	deg_angle -= 90;	//Just works
	return sf::Vector2f(cos(deg_to_rad(deg_angle)), sin(deg_to_rad(deg_angle)));
}
sf::Vector2f normalize(sf::Vector2f vec)
{
	float w = sqrt(vec.x * vec.x + vec.y * vec.y);
	if (w != 0) {
		vec.x /= w;
		vec.y /= w;
	}
	return vec;
}
void InitGame()
{
	GInfo.player2dtexture.loadFromFile("2d_player.png");
	GInfo.player_angle = 90.0f;
	GInfo.player_pos = { 3, 3 };
	GInfo.player2dsprite.setPosition(GInfo.player_pos);
	GInfo.player2dsprite.setRotation(GInfo.player_angle);
	GInfo.player2dsprite.setTexture(GInfo.player2dtexture);
	GInfo.player2dsprite.setOrigin({ 16,16 });
	GInfo.dot_array.setPrimitiveType(sf::TrianglesFan);

	GInfo.render_texture.create(WIDTH, HEIGHT);

	GInfo.buffer = new sf::Uint8[WIDTH * HEIGHT * 4];
	GInfo.depth = new float[WIDTH];

	GInfo.wall_image.loadFromFile("wall_textures.png");
	GInfo.skybox.loadFromFile("Sky_hydro_01.jpg");

	//For drawing our buffer texture
	GInfo.render_tex_array.setPrimitiveType(sf::Quads);
	GInfo.render_tex_array.resize(4);
	sf::Vertex* temp = &GInfo.render_tex_array[0];
	temp[0].position = { 0,0 };
	temp[0].texCoords = { 0,0 };
	temp[1].position = { WIDTH*SCALE,0 };
	temp[1].texCoords = { WIDTH,0 };
	temp[2].position = { WIDTH * SCALE, HEIGHT * SCALE };
	temp[2].texCoords = { WIDTH,HEIGHT };
	temp[3].position = { 0, HEIGHT * SCALE };
	temp[3].texCoords = { 0,HEIGHT };

	//Load wall info
	GInfo.wall_rects.push_back({ 0,0,64,64 });
	GInfo.wall_rects.push_back({ 256,192,64,64 });
	GInfo.wall_rects.push_back({ 256,256,64,64 });
	GInfo.wall_rects.push_back({ 256,128,64,64 });
	GInfo.wall_rects.push_back({ 128,320,64,64 });
	GInfo.wall_rects.push_back({ 256,320,64,64 });
	GInfo.wall_rects.push_back({ 256,448,64,64 });

	//Tf2 images
	sf::Image img;
	img.loadFromFile("wood1.png");
	GInfo.game_images.push_back(img);
	img.loadFromFile("metal1.png");
	GInfo.game_images.push_back(img);
	img.loadFromFile("concrete1.png");
	GInfo.game_images.push_back(img);
	img.loadFromFile("concrete2.png");
	GInfo.game_images.push_back(img);
	img.loadFromFile("wood2.png");
	GInfo.game_images.push_back(img);
}
sf::Vector2f CalculateVector(float angle, float magnitude)
{
	angle -= 90;
	sf::Vector2f velocity;
	velocity.y = std::sin(angle * 3.1415 / 180) * magnitude;
	velocity.x = std::sin((90.0 - angle) * 3.1415 / 180) * magnitude;

	return velocity;
}
bool LoadMap(const char* file)
{
	std::ifstream infile(file);
	std::string line;
	if (!infile)
		return false;
	while (std::getline(infile, line))
{
		GInfo.GameMap.push_back(line);
	}
	infile.close();
	return true;
}
sf::Color GetColor(int x, int y, int texture)
{
	return GInfo.game_images.at(texture).getPixel(x, y);
}
enum
{
	RENDER2D = 1,
	RENDER3D = 2,
};
void CastRays(uint8_t flags)
{
	sf::Vector2f pos = GInfo.player_pos;
	sf::Vector2f dir = { unit_vector(GInfo.player_angle) };
	
	sf::Vertex vert;
	if (flags & RENDER2D) {
		vert.position = pos * tile_width;
		GInfo.dot_array.append(vert);
	}

	for (int x = 0; x < WIDTH; x++)
	{
		//calculate ray position and directon
		//This gives angle in radians
		float angle = atan((x - WIDTH/2) / DIST);
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
		
		if (flags & RENDER2D) {
			vert.position = (pos + ray_dir * perp_wall_dist) * tile_width;
			GInfo.dot_array.append(vert);
		}
		
		if (flags & RENDER3D) {
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
			case '#': texture = GInfo.STONE_WALL; break;
			case '$': texture = GInfo.WOOD_WALL; break;
			case '@': texture = GInfo.BLUE_METAL; break;
			case '&': texture = GInfo.BLUE_WALL; break;
			case '%': texture = GInfo.RED_BRICK; break;
			case '!': texture = GInfo.MOSS_WALL; break;
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
				assert(y < HEIGHT && y >= 0);
				int tex_y = (int)tex_pos & (63);
				tex_pos += step;
				sf::IntRect image_rect = GInfo.wall_rects.at(texture);
				
				//Get shaded texture if on y side
				if (side) image_rect = { image_rect.left + 64, image_rect.top, 64, 64 };
				sf::Uint8* temp = &GInfo.buffer[int(y * WIDTH + x) * 4];

				sf::Color color = GInfo.wall_image.getPixel(image_rect.left + tex_x, image_rect.top + tex_y);

				temp[0] = color.r;
				temp[1] = color.g;
				temp[2] = color.b;
				temp[3] = color.a;
			}
			
		}
	}
}

sf::Color GetSkyBoxColor(int x, int y, int x_coord)
{
	//2048 and 256 are just the width of the skybox image I choose, this must be changed
	//if using a different image
	int y_coord = y * (256 / HEIGHT);
	y_coord = int(256 - y_coord)%256;

	sf::Color color = GInfo.skybox.getPixel((x_coord + x)%2048, y_coord);
	return color;
}
void FloorCasting()
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
			sf::IntRect tex = GInfo.wall_rects.at(GInfo.STONE_WALL);
			sf::Uint8* temp = &GInfo.buffer[int(y * WIDTH + x) * 4];
			sf::Color color = GInfo.wall_image.getPixel(tex.left + tex_coord.x, tex.top + tex_coord.y);
			temp[0] = color.r;
			temp[1] = color.g;
			temp[2] = color.b;
			temp[3] = color.a;
			//Ceiling/skybox
			tex = GInfo.wall_rects.at(GInfo.BLUE_WALL);
			temp = &GInfo.buffer[int((HEIGHT - 1 - y) * WIDTH + x) * 4];
			color = GInfo.wall_image.getPixel(tex.left + tex_coord.x, tex.top + tex_coord.y);
			color = GetSkyBoxColor(x, y, x_coord);
			temp[0] = color.r;
			temp[1] = color.g;
			temp[2] = color.b;
			temp[3] = color.a;
		}
	}
}
void Render2d()
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

	CastRays(RENDER2D);

	GInfo.window->draw(GInfo.dot_array);
	GInfo.dot_array.clear();
	GInfo.window->draw(verticies);
	GInfo.window->draw(GInfo.player2dsprite);
}
void Render3d()
{
	FloorCasting();
	CastRays(RENDER3D);
	GInfo.render_texture.update(GInfo.buffer, WIDTH, HEIGHT, 0, 0);
	
	sf::RenderStates state;
	state.texture = &GInfo.render_texture;
	GInfo.window->draw(GInfo.render_tex_array, state);
}
bool MapCollision(sf::Vector2f position)
{
	sf::Vector2f center = { float(position.x - 0.5f), float(position.y - 0.5f) };

	if (GetTile({ (int)center.x,(int)center.y }) != ' ' ||
		(GetTile({ int(center.x + 0.9),(int)center.y }) != ' ') ||
		(GetTile({ int(center.x + 0.9), int(center.y + 0.9) }) != ' ') ||
		(GetTile({ (int)center.x,int(center.y + 0.9) }) != ' ')) {
		return true;
	}
	return false;
}
void PlayerMovement()
{
	sf::Vector2f step(0, 0);
	
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		step += CalculateVector(GInfo.player_angle,	1);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		step += CalculateVector(GInfo.player_angle-90, 1);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		step += CalculateVector(GInfo.player_angle+90, 1);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		step += CalculateVector(GInfo.player_angle + 180, 1);

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
		GInfo.player_angle -= 3;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
		GInfo.player_angle += 3;

	//Adjust step to move speed
	step = normalize(step) * MOVE_SPEED;


	if(!MapCollision({ GInfo.player_pos.x + step.x, GInfo.player_pos.y }))
		GInfo.player_pos.x += step.x;
	if (!MapCollision({ GInfo.player_pos.x, GInfo.player_pos.y + step.y }))
		GInfo.player_pos.y += step.y;

	//For 2d rendering
	GInfo.player2dsprite.setPosition({ GInfo.player_pos.x * tile_width, GInfo.player_pos.y * tile_width });
	GInfo.player2dsprite.setRotation(GInfo.player_angle);
}
void FrameTimer(float elapsed)
{
	static double accumulated = 0;
	static int amt = 0;
	accumulated += elapsed;
	amt++;
	if (accumulated > 1000)
	{
		printf("FPS: %f\n", 1 / (accumulated / amt) * 1000);
		accumulated = 0;
		amt = 0;
	}
}
void HandleMouseMovement(sf::Event& event)
{
	float x_diff = event.mouseMove.x - (WIDTH * SCALE * 0.5f) + 8;
	//printf("X diff: %f\n", x_diff);
	float angle = sin(x_diff / DIST);
	//To degrees
	angle = angle * 180 / 3.14159f;
	GInfo.player_angle += angle;
	sf::Mouse::setPosition({ int(GInfo.window->getPosition().x + (WIDTH*SCALE)/2), 
		int(GInfo.window->getPosition().y + (HEIGHT * SCALE) / 2) });
}
int main(int argc, char** argv)
{
	sf::RenderWindow window(sf::VideoMode(WIDTH*SCALE, HEIGHT*SCALE), "Raycast FPS");
	window.setMouseCursorGrabbed(true);
	window.setMouseCursorVisible(false);
	//window.setFramerateLimit(60);
	GInfo.window = &window;

	if(!LoadMap("Map-1.txt"))
		return 1;

	bool In3d = true;
	InitGame();
	sf::Clock clock;
	float ms_elapsed;
	while (window.isOpen()) {
		sf::Event event;
		ms_elapsed = clock.restart().asMilliseconds();
		FrameTimer(ms_elapsed);
		while (window.pollEvent(event)) {
			switch(event.type)
			{
			case sf::Event::Closed: window.close(); break;
			case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Space)
					In3d = !In3d;
				if (event.key.code == sf::Keyboard::Escape)
					window.close();
				if (event.key.code == sf::Keyboard::Num1)
					DIST -= 20;
				if (event.key.code == sf::Keyboard::Num2)
					DIST += 20;
				break;
			case sf::Event::MouseMoved: HandleMouseMovement(event); break;
			default: break;
			}
		}
		PlayerMovement();

		window.clear({0xf7,0x00,0xf7});
		if (In3d)
			Render3d();
		else
			Render2d();
		window.display();
	}

	return 0;
}
