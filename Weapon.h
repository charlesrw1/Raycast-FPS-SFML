#pragma once
#include <SFML/Graphics.hpp>
#include "Assets.h"
#include "Level.h"
#include "Global.h"

struct WeaponData
{
	std::vector<sf::IntRect> GunRects;
	std::vector<int> GunFrames;
	void(*OnClickCallback)();
};

void CreateHitscanRay();

struct Weapon
{
	Weapon(sf::Texture& texture, int weapon_type)
	{
		mType = weapon_type;

		mSprite.setTexture(texture);
		mSprite.setOrigin({ 25,32 });
		mSprite.setPosition({ WIDTH * SCALE * 0.5, HEIGHT * SCALE * 0.7 });
		mSprite.setScale(SCALE * 1.5, SCALE * 1.5);
		bool InFiring = false;
		mCurrentFrame = 0;

		mSprite.setTextureRect(GAssets.Weapon_Frames.at(mType).GunRects.at(mCurrentFrame));
		mShootFrame = 0;
		mTimerMs = 0;
	}
	void Update(float elapsed)
	{
		if (InFiring) {
			mTimerMs += (int)elapsed;
			if (mTimerMs >= 100 * GAssets.Weapon_Frames.at(mType).GunRects.size()) {
				mCurrentFrame = 0;
				mSprite.setTextureRect(GAssets.Weapon_Frames.at(mType).GunRects.at(mCurrentFrame));
				mTimerMs = 0;
				InFiring = false;
			}
			else {
				//New frame every 100 ms
				mShootFrame = mTimerMs / 100;
				mSprite.setTextureRect(
					GAssets.Weapon_Frames.at(mType).GunRects.at(GAssets.Weapon_Frames.at(mType).GunFrames.at(mShootFrame))
				);
			}
		}
	}
	void Draw()
	{
		GInfo.window->draw(mSprite);
	}
	void OnClick()
	{
		printf("Clicked\n");
		if (!InFiring) {
			InFiring = true;
			GAssets.Weapon_Frames.at(mType).OnClickCallback();
		}
	}
private:
	int mType;

	bool InFiring = false;
	sf::Sprite mSprite;
	int mTimerMs;
	int mCurrentFrame;
	int mShootFrame;
};