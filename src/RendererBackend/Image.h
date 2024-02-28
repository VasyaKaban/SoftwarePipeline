#pragma once

#include <algorithm>
#include <vector>
#include "../hrs/math/vector.hpp"

namespace Renderer
{
	enum class Format
	{
		RGBA32_PACKED,
		BGRA32_PACKED,
		ARGB32_PACKED,
		ABGR32_PACKED,
		DEPTH32_SFLOAT
	};

	constexpr std::size_t GetFormatTexelSize(Format format) noexcept
	{
		switch(format)
		{
			case Format::RGBA32_PACKED:
			case Format::BGRA32_PACKED:
			case Format::ARGB32_PACKED:
			case Format::ABGR32_PACKED:
			case Format::DEPTH32_SFLOAT:
				return 4;
				break;
		}
	}

	constexpr void SetFormatImageColor(Format format, std::byte *data, const hrs::math::glsl::vec4 &color) noexcept
	{
		constexpr auto float_to_byte = [](float value) noexcept
		{
			return static_cast<std::byte>(std::clamp(value, 0.0f, 1.0f) * 255);
		};

		switch(format)
		{
			case Format::RGBA32_PACKED:
				{
					std::uint32_t packed_color =
						(static_cast<std::uint32_t>(float_to_byte(color[0])) << 24) |
						(static_cast<std::uint32_t>(float_to_byte(color[1])) << 16) |
						(static_cast<std::uint32_t>(float_to_byte(color[2])) << 8) |
						(static_cast<std::uint32_t>(float_to_byte(color[3])) << 0);

					*reinterpret_cast<std::uint32_t *>(data) = packed_color;
				}
				break;
			case Format::BGRA32_PACKED:
				{
					std::uint32_t packed_color =
						(static_cast<std::uint32_t>(float_to_byte(color[0])) << 8) |
						(static_cast<std::uint32_t>(float_to_byte(color[1])) << 16) |
						(static_cast<std::uint32_t>(float_to_byte(color[2])) << 24) |
						(static_cast<std::uint32_t>(float_to_byte(color[3])) << 0);

					*reinterpret_cast<std::uint32_t *>(data) = packed_color;
				}
				break;
			case Format::ARGB32_PACKED:
				{
					std::uint32_t packed_color =
						(static_cast<std::uint32_t>(float_to_byte(color[0])) << 16) |
						(static_cast<std::uint32_t>(float_to_byte(color[1])) << 8) |
						(static_cast<std::uint32_t>(float_to_byte(color[2])) << 0) |
						(static_cast<std::uint32_t>(float_to_byte(color[3])) << 24);

					*reinterpret_cast<std::uint32_t *>(data) = packed_color;
				}
				break;
			case Format::ABGR32_PACKED:
				{
					std::uint32_t packed_color =
						(static_cast<std::uint32_t>(float_to_byte(color[0])) << 0) |
						(static_cast<std::uint32_t>(float_to_byte(color[1])) << 8) |
						(static_cast<std::uint32_t>(float_to_byte(color[2])) << 16) |
						(static_cast<std::uint32_t>(float_to_byte(color[3])) << 24);

					*reinterpret_cast<std::uint32_t *>(data) = packed_color;
				}
				break;
			case Format::DEPTH32_SFLOAT:
				{
					float depth = color[0];
					*reinterpret_cast<float *>(data) = depth;
				}
				break;
		}
	}

	constexpr hrs::math::glsl::vec4 GetFormatImageColor(Format format, const std::byte *data) noexcept
	{
		constexpr auto byte_to_float = [](std::byte value) noexcept
		{
			return (1.0f / 255) * static_cast<float>(value);
		};

		hrs::math::glsl::vec4 out_color;
		switch(format)
		{
			case Format::RGBA32_PACKED:
				{
					std::uint32_t packed_color = *reinterpret_cast<const std::uint32_t *>(data);
					out_color[0] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 24)) >> 24));
					out_color[1] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 16)) >> 16));
					out_color[2] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 8)) >> 8));
					out_color[3] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 0)) >> 0));
				}
				break;
			case Format::BGRA32_PACKED:
				{
					std::uint32_t packed_color = *reinterpret_cast<const std::uint32_t *>(data);
					out_color[0] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 8)) >> 8));
					out_color[1] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 16)) >> 16));
					out_color[2] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 24)) >> 24));
					out_color[3] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 0)) >> 0));
				}
				break;
			case Format::ARGB32_PACKED:
				{
					std::uint32_t packed_color = *reinterpret_cast<const std::uint32_t *>(data);
					out_color[0] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 16)) >> 16));
					out_color[1] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 8)) >> 8));
					out_color[2] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 0)) >> 0));
					out_color[3] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 24)) >> 24));
				}
				break;
			case Format::ABGR32_PACKED:
				{
					std::uint32_t packed_color = *reinterpret_cast<const std::uint32_t *>(data);
					out_color[0] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 0)) >> 0));
					out_color[1] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 8)) >> 8));
					out_color[2] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 16)) >> 16));
					out_color[3] = byte_to_float(static_cast<std::byte>((packed_color & (0xFF << 24)) >> 24));
				}
				break;
			case Format::DEPTH32_SFLOAT:
				{
					float depth = *reinterpret_cast<const float *>(data);
					out_color = {depth};
				}
				break;
		}

		return out_color;
	}

	constexpr void SetFormatImageDepth(Format format, std::byte *data, float depth) noexcept
	{
		switch(format)
		{
			case Format::RGBA32_PACKED:
			case Format::BGRA32_PACKED:
			case Format::ARGB32_PACKED:
			case Format::ABGR32_PACKED:
			case Format::DEPTH32_SFLOAT:
				*reinterpret_cast<float *>(data) = depth;
				break;
		}
	}

	constexpr float GetFormatImageDepth(Format format, const std::byte *data) noexcept
	{
		switch(format)
		{
			case Format::RGBA32_PACKED:
			case Format::BGRA32_PACKED:
			case Format::ARGB32_PACKED:
			case Format::ABGR32_PACKED:
			case Format::DEPTH32_SFLOAT:
				return *reinterpret_cast<const float *>(data);
				break;
		}
	}

	class Image
	{
	public:
		Image(std::size_t _width = {}, std::size_t _height = {}, Format _format = {});
		~Image() = default;
		Image(const Image &) = default;
		Image(Image &&) = default;
		Image & operator=(const Image &) = default;
		Image & operator=(Image &&) = default;

		void Destroy() noexcept;
		bool IsCreated() const noexcept;
		void Resize(std::size_t _width = {}, std::size_t _height = {}, Format _format = {});

		std::size_t GetWidth() const noexcept;
		std::size_t GetHeight() const noexcept;
		Format GetFormat() const noexcept;

		std::byte * GetMappedPtr() noexcept;
		const std::byte * GetMappedPtr() const noexcept;

		hrs::math::glsl::vec4 GetValueColor(std::size_t i, std::size_t j) const noexcept;
		float GetValueDepth(std::size_t i, std::size_t j) const noexcept;
		void SetValueColor(std::size_t i, std::size_t j, const hrs::math::glsl::vec4 &color) noexcept;
		void SetValueDepth(std::size_t i, std::size_t j, float depth) noexcept;

	private:
		std::size_t width;
		std::size_t height;
		Format format;

		std::vector<std::byte> data;
	};
};
