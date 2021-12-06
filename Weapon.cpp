#include "Weapon.h"
#include "Utilities.h"
void CreateHitscanRay()
{
	//For now assumes ray comes from player
	//pasted from raycasting function below
	sf::Vector2f pos = GInfo.player_pos;
	sf::Vector2f dir = { unit_vector(GInfo.player_angle) };
	float angle = atan((WIDTH / 2 - WIDTH / 2) / DIST);
	angle = angle * 180 / 3.14159f;
	angle = GInfo.player_angle + angle;
	sf::Vector2f ray_dir = { unit_vector(angle) };
	sf::Vector2i map_tile = { int(pos.x), int(pos.y) };
	sf::Vector2f side_dist;
	sf::Vector2f delta_dist;
	delta_dist.x = sqrt(1 + pow(ray_dir.y / ray_dir.x, 2));
	delta_dist.y = sqrt(1 + pow(ray_dir.x / ray_dir.y, 2));
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

	sf::Vertex vert;
	vert.position = pos * tile_width;
	GInfo.hitscan_array.append(vert);
	vert.position = (pos + ray_dir * perp_wall_dist) * tile_width;
	GInfo.hitscan_array.append(vert);
}