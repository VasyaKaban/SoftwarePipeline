#pragma once

#include "../hrs/math/vector.hpp"
#include <cassert>

namespace Renderer
{
	template<typename D>
	concept LinearInterpolatable = requires(D &&data0)
	{
		{std::forward<D>(data0) *= float{}} -> std::same_as<std::add_lvalue_reference_t<std::remove_cvref_t<D>>>;
		{std::forward<D>(data0) * float{}} -> std::same_as<std::remove_cvref_t<D>>;
		{std::forward<D>(data0) / std::int64_t{}} -> std::same_as<std::remove_cvref_t<D>>;
		{std::forward<D>(data0) - std::forward<D>(data0)} -> std::same_as<std::remove_cvref_t<D>>;
		{std::forward<D>(data0) + std::forward<D>(data0)} -> std::same_as<std::remove_cvref_t<D>>;
		{std::forward<D>(data0) += std::forward<D>(data0)} -> std::same_as<std::add_lvalue_reference_t<std::remove_cvref_t<D>>>;
	};

	template<LinearInterpolatable D>
	struct Vertex
	{
		hrs::math::glsl::vec4 vertex;
		D attributes;

		Vertex operator+(const Vertex &vert) const noexcept
		{
			return Vertex{.vertex = vertex + vert.vertex, .attributes = attributes + vert.attributes};
		}

		Vertex operator-(const Vertex &vert) const noexcept
		{
			return Vertex{.vertex = vertex - vert.vertex, .attributes = attributes - vert.attributes};
		}

		Vertex operator*(float val) const noexcept
		{
			return Vertex{.vertex = vertex * val, .attributes = attributes * val};
		}

		Vertex & operator*=(float val) noexcept
		{
			vertex *= val;
			attributes *= val;
			return *this;
		}

		constexpr static Vertex Lerp(const Vertex &a, const Vertex &b, float t) noexcept
		{
			return Vertex{.vertex = a.vertex + (b.vertex - a.vertex) * t,
						  .attributes = a.attributes + (b.attributes - a.attributes) * t};
		}
	};

	enum class ClipResult
	{
		InsidePlane,
		OutsidePlane,
		OneResult,
		TwoResult
	};

	enum class ClipPlane
	{
		POSITIVE_W = 1 << 0,
		POSITIVE_X = 1 << 1,
		NEGATIVE_X = 1 << 2,
		POSITIVE_Y = 1 << 3,
		NEGATIVE_Y = 1 << 4,
		POSITIVE_Z = 1 << 5,
		NEGATIVE_Z = 1 << 6,
		MAX_PLANE = 1 << 7
	};

	constexpr bool IsVertexOutside(ClipPlane plane, const hrs::math::glsl::vec4 &vertex) noexcept
	{
		switch(plane)
		{
			case ClipPlane::POSITIVE_W:
				return vertex[3] < std::numeric_limits<float>::epsilon();
				break;
			case ClipPlane::POSITIVE_X:
				return vertex[0] > vertex[3];
				break;
			case ClipPlane::NEGATIVE_X:
				return vertex[0] < -vertex[3];
				break;
			case ClipPlane::POSITIVE_Y:
				return vertex[1] > vertex[3];
				break;
			case ClipPlane::NEGATIVE_Y:
				return vertex[1] < -vertex[3];
				break;
			case ClipPlane::POSITIVE_Z:
				return vertex[2] > vertex[3];
				break;
			case ClipPlane::NEGATIVE_Z:
				return vertex[2] < -vertex[3];
				break;
			default:
				assert(false);
				break;
		}
	}

	constexpr float GetPlaneLerpFactor(ClipPlane plane,
									  const hrs::math::glsl::vec4 &start,
									  const hrs::math::glsl::vec4 &end) noexcept
	{
		switch(plane)
		{
			case ClipPlane::POSITIVE_W:
				return (std::numeric_limits<float>::epsilon() - start[3]) / (end[3] - start[3]);
				break;
			case ClipPlane::POSITIVE_X:
				return (start[0] - start[3]) / ((end[3] - start[3]) - (end[0] - start[0]));
				break;
			case ClipPlane::NEGATIVE_X:
				return -(start[3] + start[0]) / ((end[0] - start[0]) + (end[3] - start[3]));
				break;
			case ClipPlane::POSITIVE_Y:
				return (start[1] - start[3]) / ((end[3] - start[3]) - (end[1] - start[1]));
				break;
			case ClipPlane::NEGATIVE_Y:
				return -(start[3] + start[1]) / ((end[1] - start[1]) + (end[3] - start[3]));
				break;
			case ClipPlane::POSITIVE_Z:
				return (start[2] - start[3]) / ((end[3] - start[3]) - (end[2] - start[2]));
				break;
			case ClipPlane::NEGATIVE_Z:
				return -(start[3] + start[2]) / ((end[2] - start[2]) + (end[3] - start[3]));
				break;
			default:
				assert(false);
				break;
		}
	}

	template<LinearInterpolatable D>
	struct Polygon
	{
		Vertex<D> vertices[3];

		Polygon(const Vertex<D> &v0 = {},
				const Vertex<D> &v1 = {},
				const Vertex<D> &v2 = {}) noexcept
			: vertices{v0, v1, v2} {}

		Polygon(const Polygon &) = default;
		Polygon & operator=(const Polygon &) = default;

		ClipResult ClipAgainstPlane(ClipPlane plane, std::pair<Polygon<D>, Polygon<D>> &output) noexcept
		{
			std::uint32_t outside_mask = 0;
			for(int i = 0; i < 3; i++)
				if(IsVertexOutside(plane, vertices[i].vertex))
					outside_mask |= 1 << i;

			if(std::popcount(outside_mask) == 0)
				return ClipResult::InsidePlane;
			else if(std::popcount(outside_mask) == 3)
				return ClipResult::OutsidePlane;
			else if(std::popcount(outside_mask) == 2)
			{
				int prev_index;
				int target_index;
				int post_index;
				if(!(outside_mask & (1 << 0)))
				{
					target_index = 0;
					prev_index = 2;
					post_index = 1;
				}
				else if(!(outside_mask & (1 << 1)))
				{
					target_index = 1;
					prev_index = 0;
					post_index = 2;
				}
				else
				{
					target_index = 2;
					prev_index = 1;
					post_index = 0;
				}

				float prev_t = GetPlaneLerpFactor(plane, vertices[prev_index].vertex, vertices[target_index].vertex);
				float post_t = GetPlaneLerpFactor(plane, vertices[post_index].vertex, vertices[target_index].vertex);

				output.first.vertices[0] = Vertex<D>::Lerp(vertices[prev_index], vertices[target_index], prev_t);
				output.first.vertices[1] = vertices[target_index];
				output.first.vertices[2] = Vertex<D>::Lerp(vertices[post_index], vertices[target_index], post_t);

				return ClipResult::OneResult;
			}
			else
			{
				int prev_index;
				int target_index;
				int post_index;
				if(outside_mask & (1 << 0))
				{
					target_index = 0;
					prev_index = 2;
					post_index = 1;
				}
				else if(outside_mask & (1 << 1))
				{
					target_index = 1;
					prev_index = 0;
					post_index = 2;
				}
				else
				{
					target_index = 2;
					prev_index = 1;
					post_index = 0;
				}

				float prev_t = GetPlaneLerpFactor(plane, vertices[target_index].vertex, vertices[prev_index].vertex);
				float post_t = GetPlaneLerpFactor(plane, vertices[target_index].vertex, vertices[post_index].vertex);

				output.first.vertices[0] = vertices[prev_index];
				output.first.vertices[1] = Vertex<D>::Lerp(vertices[target_index], vertices[prev_index], prev_t);
				output.first.vertices[2] = vertices[post_index];

				output.second.vertices[0] = vertices[post_index];
				output.second.vertices[1] = output.first.vertices[1];
				output.second.vertices[2] = Vertex<D>::Lerp(vertices[target_index], vertices[post_index], post_t);

				return ClipResult::TwoResult;
			}
		}
	};
};
