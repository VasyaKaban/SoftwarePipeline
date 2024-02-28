#include "Image.h"

namespace Renderer
{
	Image::Image(std::size_t _width, std::size_t _height, Format _format)
		: width(_width), height(_height), format(_format)
	{
		if(width * height != 0)
			data.resize(width * height * GetFormatTexelSize(format));
	}

	void Image::Destroy() noexcept
	{
		if(!IsCreated())
			return;

		data.clear();
	}

	bool Image::IsCreated() const noexcept
	{
		return !data.empty();
	}

	void Image::Resize(std::size_t _width, std::size_t _height, Format _format)
	{
		width = _width;
		height = _height;
		format = _format;

		data.clear();
		if(width * height != 0)
			data.resize(width * height * GetFormatTexelSize(format));
	}

	std::size_t Image::GetWidth() const noexcept
	{
		return width;
	}

	std::size_t Image::GetHeight() const noexcept
	{
		return height;
	}

	Format Image::GetFormat() const noexcept
	{
		return format;
	}

	std::byte * Image::GetMappedPtr() noexcept
	{
		return data.data();
	}

	const std::byte * Image::GetMappedPtr() const noexcept
	{
		return data.data();
	}

	hrs::math::glsl::vec4 Image::GetValueColor(std::size_t i, std::size_t j) const noexcept
	{
		if(i >= width || j >= height)
			return {0, 0, 0, 0};

		const std::byte *target = &data[j * width * GetFormatTexelSize(format) + i * GetFormatTexelSize(format)];


		//switch format -> now we are working only with 32 packed formats!!!
		return GetFormatImageColor(format, target);
	}

	float Image::GetValueDepth(std::size_t i, std::size_t j) const noexcept
	{
		if(i >= width || j >= height)
			return NAN;

		const std::byte *target = &data[j * width * GetFormatTexelSize(format) + i * GetFormatTexelSize(format)];
		return GetFormatImageDepth(format, target);
	}

	void Image::SetValueColor(std::size_t i, std::size_t j, const hrs::math::glsl::vec4 &color) noexcept
	{
		if(i >= width || j >= height)
			return;

		std::byte *target = &data[j * width * GetFormatTexelSize(format) + i * GetFormatTexelSize(format)];
		SetFormatImageColor(format, target, color);
	}

	void Image::SetValueDepth(std::size_t i, std::size_t j, float depth) noexcept
	{
		if(i >= width || j >= height)
			return;

		std::byte *target = &data[j * width * GetFormatTexelSize(format) + i * GetFormatTexelSize(format)];
		SetFormatImageDepth(format, target, depth);
	}
};
