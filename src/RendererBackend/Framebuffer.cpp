#include "Framebuffer.h"
#include <execution>

namespace Renderer
{
	Framebuffer::Framebuffer(std::span<Image *> _color_images, Image * _depth_image)
	{
		if(!_color_images.empty())
		{
			color_images.reserve(_color_images.size());
			for(auto &img : _color_images)
				color_images.push_back(img);
		}

		depth_image = _depth_image;
	}

	void Framebuffer::Destroy() noexcept
	{
		if(!IsCreated())
			return;

		color_images.clear();
		depth_image = nullptr;
	}

	bool Framebuffer::IsCreated() const noexcept
	{
		return !color_images.empty() || !depth_image;
	}

	void Framebuffer::ClearImage(const ClearValue &value, std::size_t index)
	{
		if(index >= color_images.size())
			return;

		Image *image = color_images[index];
		if(!image)
			return;

		if(image->GetFormat() == Format::DEPTH32_SFLOAT)
		{
			float depth = value.depth;
			for(std::size_t i = 0; i < image->GetWidth(); i++)
				for(std::size_t j = 0; j < image->GetHeight(); j++)
					image->SetValueDepth(i, j, depth);
		}
		else
		{
			const hrs::math::glsl::vec4 &color = value.color;
			for(std::size_t i = 0; i < image->GetWidth(); i++)
				for(std::size_t j = 0; j < image->GetHeight(); j++)
					image->SetValueColor(i, j, color);
		}
	}

	void Framebuffer::ClearDepthImage(float value)
	{
		if(!depth_image)
			return;

		for(std::size_t i = 0; i < depth_image->GetWidth(); i++)
			for(std::size_t j = 0; j < depth_image->GetHeight(); j++)
				depth_image->SetValueDepth(i, j, value);
	}

	Image * Framebuffer::GetColorImage(std::size_t index) noexcept
	{
		if(index >= color_images.size())
			return nullptr;

		return color_images[index];
	}

	const Image * Framebuffer::GetColorImage(std::size_t index) const noexcept
	{
		if(index >= color_images.size())
			return nullptr;

		return color_images[index];
	}

	Image * Framebuffer::GetDepthImage() noexcept
	{
		return depth_image;
	}

	const Image * Framebuffer::GetDepthImage() const noexcept
	{
		return depth_image;
	}
};
