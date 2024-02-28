#include "Viewport.h"

namespace Renderer
{
	Viewport::Viewport(std::uint32_t _width,
					   std::uint32_t _height,
					   std::uint32_t _x,
					   std::uint32_t _y,
					   float _min_depth,
					   float _max_depth) noexcept
		: width(_width),
		  height(_height),
		  x(_x),
		  y(_y),
		  min_depth(_min_depth),
		  max_depth(_max_depth) {}

	std::uint32_t Viewport::GetWidth() const noexcept
	{
		return width;
	}

	std::uint32_t Viewport::GetHeight() const noexcept
	{
		return height;
	}

	std::uint32_t Viewport::GetX() const noexcept
	{
		return x;
	}

	std::uint32_t Viewport::GetY() const noexcept
	{
		return y;
	}

	float Viewport::GetMinDepth() const noexcept
	{
		return min_depth;
	}

	float Viewport::GetMaxDepth() const noexcept
	{
		return max_depth;
	}
};
