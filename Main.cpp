#include <SFML/graphics.hpp>
#include <SFML/Audio.hpp>

#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <assert.h>

#include "Utilities.h"
#include "Assets.h"
#include "Level.h"
#include "Settings.h"
#include "Renderer.h"
#include "Weapon.h"

Settings GUser;
GameInfo GInfo;
Assets GAssets;
Renderer GRender;

void InitWeapons()
{
	WeaponData temp;
	//Pistol
	temp.GunRects = {
		{ 0,44,35,64 },
		{ 35,44,49,64 },
		{ 84,44,49,64 },
		{ 133,44,49,64 }
	};
	temp.GunFrames = { 1,2,3,1 };
	temp.OnClickCallback = &CreateHitscanRay;
	GAssets.Weapon_Frames.push_back(temp);
}
void InitAssets()
{
	GAssets.player2dtexture.loadFromFile("images/2d_player.png");

	GAssets.WL_WallImage.loadFromFile("images/wall_textures.png");
	GAssets.skybox.loadFromFile("images/Sky_trainyard_01.jpg");

	//Load wall info
	GAssets.WL_WallRects.push_back({ 0,0,64,64 });
	GAssets.WL_WallRects.push_back({ 256,192,64,64 });
	GAssets.WL_WallRects.push_back({ 256,256,64,64 });
	GAssets.WL_WallRects.push_back({ 256,128,64,64 });
	GAssets.WL_WallRects.push_back({ 128,320,64,64 });
	GAssets.WL_WallRects.push_back({ 256,320,64,64 });
	GAssets.WL_WallRects.push_back({ 256,448,64,64 });

	//Tf2 images
	sf::Image img;
	img.loadFromFile("images/wood1.png");
	GAssets.TF2_Images.push_back(img);
	img.loadFromFile("images/metal1.png");
	GAssets.TF2_Images.push_back(img);
	img.loadFromFile("images/concrete1.png");
	GAssets.TF2_Images.push_back(img);
	img.loadFromFile("images/concrete2.png");
	GAssets.TF2_Images.push_back(img);
	img.loadFromFile("images/wood2.png");
	GAssets.TF2_Images.push_back(img);

	img.loadFromFile("images/weapons.png");
	img.createMaskFromColor({ 163,73,164 });
	GAssets.WL_WeaponTexture.loadFromImage(img);

	img.loadFromFile("images/wl_guard.png");
	img.createMaskFromColor({ 152,0,136 });
	GAssets.WL_GuardImage = img;
}
void InitGame()
{
	GInfo.player_angle = 90.0f;
	GInfo.player_pos = { 3, 3 };
	GInfo.hitscan_array.setPrimitiveType(sf::Lines);

	//Add entities, will add option for texture later
	GInfo.entity_list.push_back({ {5,5},90 });

	InitAssets();
	InitWeapons();
	GInfo.pWeapon = new Weapon(GAssets.WL_WeaponTexture, GInfo.WL_PISTOL);


	GUser.displayDist = GetDisplayDist(GUser.FOV, GUser.bWidth);
	GUser.winWidth = GUser.bWidth * GUser.winScale;
	GUser.winHeight = GUser.winHeight * GUser.winScale;
	GRender.InitMapVerticies(GUser.tileWidth);
}
void UpdateFOV(float new_fov) {
	GUser.FOV = new_fov;
	GUser.displayDist = GetDisplayDist(GUser.FOV, GUser.bWidth);
	printf("New fov: %f\nNew dist: %f\n", GUser.FOV, GUser.displayDist);
}
void HandleMouseMovement(sf::Event& event)
{
	float x_diff = event.mouseMove.x - (GUser.winWidth * 0.5) + 8;
	float angle = sin(x_diff / GUser.displayDist);
	angle = angle * 180 / 3.14159f;
	GInfo.player_angle += angle;
	sf::Mouse::setPosition({ int(GInfo.window->getPosition().x + (GUser.winWidth) / 2),
		int(GInfo.window->getPosition().y + (GUser.winWidth) / 2) });
}
void HandleMouseClick(sf::Event& event)
{
	if (GInfo.pWeapon != nullptr)
		GInfo.pWeapon->OnClick();
}
void HandleEvents(sf::Event& event)
{
	//Currently this is just for game scene, Ill add a menu scene later
	switch (event.type)
	{
	case sf::Event::KeyPressed:
		if (event.key.code == sf::Keyboard::Space);

		else if (event.key.code == sf::Keyboard::Escape)
			GInfo.window->close();
		else if (event.key.code == sf::Keyboard::Num1)
			UpdateFOV(GUser.FOV - 4);
		else if (event.key.code == sf::Keyboard::Num2)
			UpdateFOV(GUser.FOV + 4);
		break;
	case sf::Event::MouseMoved: 
		//HandleMouseMovement(event); 
		break;
	case sf::Event::MouseButtonPressed:
		HandleMouseClick(event);
	default: break;
	}
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

	if (GInfo.player_angle > 360.0f)
		GInfo.player_angle -= 360;
	else if (GInfo.player_angle < 0)
		GInfo.player_angle += 360;

}
void GameUpdate(float ms_elapsed)
{
	PlayerMovement();
	GInfo.pWeapon->Update(ms_elapsed);
}
void OnInterval(float ms_elasped)
{
	static float accumulated = 0;
	accumulated += ms_elasped;
	if (accumulated > 1000)
	{
		//Debugging----

		sf::Vector2f camera;
		sf::Vector2f dir = unit_vector(GInfo.player_angle);
		camera = dir * 0.66;
		camera = { camera.y*-1, camera.x*-1 };

		accumulated = 0;
	}
}
int main(int argc, char** argv)
{
	sf::RenderWindow window(sf::VideoMode(WIDTH*SCALE, HEIGHT*SCALE), "Raycast FPS");
	//window.setMouseCursorGrabbed(true);
	//window.setMouseCursorVisible(false);
	//window.setFramerateLimit(60);
	GInfo.window = &window;
	
	if(!LoadMap("Map-1.txt"))
		return 1;
	InitGame();

	sf::Clock clock;
	float ms_elapsed;

	while (window.isOpen()) {
		sf::Event event;
		ms_elapsed = clock.restart().asMilliseconds();
		while (window.pollEvent(event)) {
			switch (event.type)
			{
			case sf::Event::Closed: window.close(); break;
			default: HandleEvents(event); break;
			}
		}

		//FrameTimer(ms_elapsed);
		GameUpdate(ms_elapsed);

		window.clear({0xf7,0x00,0xf7});
		GRender.Draw();
		window.display();
	}

	return 0;
}
