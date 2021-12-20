#ifndef UTILITIES
#define UTILITIES
#include "SFML/Graphics.hpp"

inline sf::Vector2f operator*(sf::Vector2f vec1, sf::Vector2f vec2)
{
	return sf::Vector2f({ vec1.x * vec2.x, vec1.y * vec2.y });
}
inline sf::Vector2f operator*(sf::Vector2f vec1, float num)
{
	return sf::Vector2f({ vec1.x * num, vec1.y * num });
}
inline sf::Color operator/(sf::Color col, int div)
{
	return sf::Color(col.r / div, col.g / div, col.b / div);
}
inline float deg_to_rad(float deg)
{
	return deg * (3.1415926 / 180);
}
inline sf::Vector2f unit_vector(float deg_angle)
{
	deg_angle -= 90;	//Just works
	return sf::Vector2f(cos(deg_to_rad(deg_angle)), sin(deg_to_rad(deg_angle)));
}
inline sf::Vector2f normalize(sf::Vector2f vec)
{
	float w = sqrt(vec.x * vec.x + vec.y * vec.y);
	if (w != 0) {
		vec.x /= w;
		vec.y /= w;
	}
	return vec;
}
inline void FrameTimer(float elapsed)
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
inline sf::Vector2f CalculateVector(float angle, float magnitude)
{
	angle -= 90;
	sf::Vector2f velocity;
	velocity.y = std::sin(angle * 3.1415 / 180) * magnitude;
	velocity.x = std::sin((90.0 - angle) * 3.1415 / 180) * magnitude;

	return velocity;
}

#endif // !UTILITIES
