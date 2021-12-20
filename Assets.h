#pragma once
#include <vector>
#include <SFML/Audio.hpp>
#include <SFML/graphics.hpp>

//Stores game assets
//Textures
//Images
//Sounds

class Assets
{
public:
	//For 2d render
	sf::Texture player2dtexture;

	//TF2
	std::vector<sf::Image> TF2_Images;
	std::vector<sf::Image> Sprites;

	//Wolfenstein
	sf::Image WL_WallImage;
	std::vector<sf::IntRect> WL_WallRects;
	sf::Texture WL_WeaponTexture;
	sf::Image WL_GuardImage;

	//Skybox
	sf::Image skybox;
	sf::Image ground;

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
};
extern Assets GAssets;