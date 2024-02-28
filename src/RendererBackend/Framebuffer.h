#pragma once

#include "Image.h"
#include <span>

namespace Renderer
{
	union ClearValue
	{
		hrs::math::glsl::vec4 color;
		float depth;
	};

	class Framebuffer
	{
	public:
		Framebuffer(std::span<Image *> _color_images = {}, Image *_depth_image = {});
		~Framebuffer() = default;
		Framebuffer(const Framebuffer &) = default;
		Framebuffer(Framebuffer &&) = default;
		Framebuffer & operator=(const Framebuffer &) = default;
		Framebuffer & operator=(Framebuffer &&) = default;

		void Destroy() noexcept;
		bool IsCreated() const noexcept;

		void ClearImage(const ClearValue &value, std::size_t index);
		void ClearDepthImage(float value);

		Image * GetColorImage(std::size_t index) noexcept;
		const Image * GetColorImage(std::size_t index) const noexcept;

		Image * GetDepthImage() noexcept;
		const Image * GetDepthImage() const noexcept;

	private:
		std::vector<Image *> color_images;
		Image * depth_image;
	};
};
