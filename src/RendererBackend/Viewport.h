#pragma once

#include <cstdint>

namespace Renderer
{
	struct Viewport
	{
		std::uint32_t width;
		std::uint32_t height;
		std::uint32_t x;
		std::uint32_t y;
		float min_depth;
		float max_depth;

		Viewport(std::uint32_t _width = {},
				 std::uint32_t _height = {},
				 std::uint32_t _x = {},
				 std::uint32_t _y = {},
				 float _min_depth = {},
				 float _max_depth = {}) noexcept;

		Viewport(const Viewport &) = default;
		Viewport & operator=(const Viewport &) = default;

		std::uint32_t GetWidth() const noexcept;
		std::uint32_t GetHeight() const noexcept;
		std::uint32_t GetX() const noexcept;
		std::uint32_t GetY() const noexcept;
		float GetMinDepth() const noexcept;
		float GetMaxDepth() const noexcept;
	};
};
