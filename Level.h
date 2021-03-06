#pragma once
#include <vector>
#include "SFML/Graphics.hpp"
#include <assert.h>

//Contains global information about level state
//Game map, refrence to render window
//Player position, angle, weapon

struct Entity
{
	sf::Vector2f pos;
	float angle;
};
struct GameInfo
{
	std::vector<std::string> GameMap;
	sf::RenderWindow* window;

	sf::VertexArray hitscan_array;

	//Player data
	sf::Vector2f player_pos;
	float player_angle;
	
	float camera_z;

	//Entity data
	//Temporary for now
	std::vector<Entity> entity_list;

};
extern GameInfo GInfo;

inline char& GetTile(sf::Vector2i vec)
{
	assert(vec.y >= 0 && vec.y < GInfo.GameMap.size());
	assert(vec.x >= 0 && vec.x < GInfo.GameMap.at(vec.y).size());

	return GInfo.GameMap.at(vec.y).at(vec.x);
}