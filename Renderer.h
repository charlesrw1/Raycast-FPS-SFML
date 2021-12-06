#pragma once
#include "SFML/Graphics.hpp"
#include "Level.h"
#include "Global.h"
//Renders 3d image scene
class Renderer
{
public:
	Renderer(uint8_t flags = 0);
	~Renderer();
	void Draw();
	void CycleRenderers()
	{
		if (mFlags & RENDER2D)
			SetFlags(RENDER3D);
		else
			SetFlags(RENDER2D);
	}
	//Rendering flags
	enum RenderFlags
	{
		RENDER2D = 1,
		RENDER3D = 2,
		RENDERGUN = 4,
		RENDERSKYBOX = 8,
	};
	void SetFlags(uint8_t flags)
	{
		mFlags = 0;
		mFlags |= flags;
	}
private:
	void CastRays();
	sf::Color GetSkyBoxColor(int x, int y, int x_coord);
	sf::Color GetColor(int x, int y, int texture);
	void FloorCasting();
	void DrawSprites();
	void Render2d();
	void Render3d();

	//Buffer for drawing pixels
	sf::Uint8* mBuffer;
	//Texture that is updated from buffer
	sf::Texture mRenderTexture;

	//Used for 2d viewcone
	sf::VertexArray mDotArray;
	//Used for drawing player and other entities
	sf::Sprite EntitySprite;

	//Depth of vertical lines drawn, used to draw sprites
	float* mDepth;

	//Used for drawing texture to window
	sf::VertexArray mRenderArray;
	sf::RenderStates mRenderState;

	//Flags for rendering
	uint8_t mFlags;
};
