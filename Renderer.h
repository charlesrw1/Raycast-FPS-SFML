#pragma once
#include "SFML/Graphics.hpp"
#include "Level.h"
#include "Settings.h"
//Renders 3d image scene
struct RayHit
{
	float angle;
	float ray_length;
	float perp_wall_dist;
	bool side;
	sf::Vector2i map_tile;
	sf::Vector2f ray_dir;
};
class Renderer
{
public:
	Renderer();
	~Renderer();
	void Draw();
	void InitMapVerticies(int tileWidth);
private:
	void CastRays(float bufWidth, float bufHeight, float displayDist);
	sf::Color GetSkyBoxColor(int x, int y);
	sf::Color GetColor(int x, int y, int texture);
	sf::IntRect GetSpriteScreenPos(const Entity& sprite);
	void FloorCasting(float bufWidth, float bufHeight, float displayDist);
	void DrawSprites();
	void DrawMap();
	void Render3d();
	void DrawSkybox();
	void DrawWalls(float bufWidth, float bufHeight, float displayDist, int tileWidth, int textureSize);

	//Buffer for drawing pixels
	sf::Uint8* mBuffer;
	//Texture that is updated from buffer
	sf::Texture mRenderTexture;

	//Used for 2d viewcone
	sf::VertexArray mDotArray;
	sf::VertexArray mMiniMap;
	//Used for drawing player and other entities
	sf::Sprite EntitySprite;

	//Depth of vertical lines drawn, used to draw sprites
	float* mDepth;

	//Used for drawing texture to window
	sf::VertexArray mRenderArray;
	sf::RenderStates mRenderState;

	std::vector<RayHit> mRayHits;

};
