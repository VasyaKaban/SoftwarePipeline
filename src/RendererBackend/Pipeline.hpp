#pragma once

#include "Framebuffer.h"
#include "Viewport.h"
#include "Polygon.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <cassert>
#include "../hrs/flags.hpp"
#include "../hrs/math/vector.hpp"

#include <iostream>

namespace Renderer
{
	template<std::size_t N>
	struct FragmentOutput
	{
		std::array<hrs::math::glsl::vec4, N> attachments;
	};

	enum class RasterizationTopology
	{
		Line,
		Fill
	};

	enum class CullSide
	{
		None,
		Back,
		Front
	};

	enum class CullOrder
	{
		ClockWise,
		CounterClockWise
	};

	struct State
	{
		RasterizationTopology topology;
		bool depth_test_enable;
		Viewport viewport;
		CullSide cull_side;
		CullOrder cull_order;

		constexpr State(RasterizationTopology _topology = {},
						bool _depth_test_enable = {},
						const Viewport &_viewport = {},
						CullSide _cull_side = {},
						CullOrder _cull_order = {}) noexcept
			: topology(_topology),
			  depth_test_enable(_depth_test_enable),
			  viewport(_viewport),
			  cull_side(_cull_side),
			  cull_order(_cull_order) {}
	};

	template<LinearInterpolatable VO/*vertex output*/, std::size_t ATTACHMENT_COUNT, typename SD>
	class Pipeline
	{
	public:

		using VertexShader = hrs::math::glsl::vec4(std::uint32_t vertex_index,
												   const std::byte */*vertex input*/,
												   VO &/*vertex output*/,
												   SD &/*shader_data*/);

		using FragmentShader = void(const VO &/*vertex output*/,
									const hrs::math::vector<std::int64_t, 2> &/*frag_position*/,
									float /*frag_depth*/,
									FragmentOutput<ATTACHMENT_COUNT> &/*fragment output*/,
									SD &/*shader_data*/);

		template<std::invocable<std::uint32_t, const std::byte *, VO &, SD &> V,
				 std::invocable<const VO &,
								 const hrs::math::vector<std::int64_t, 2> &,
								 float,
								 FragmentOutput<ATTACHMENT_COUNT> &,
								 SD &> F>
		Pipeline(std::size_t _vertex_data_stride,
				 V &&_vertex_shader,
				 F &&_fragment_shader);

		Pipeline(const Pipeline &) = delete;
		Pipeline(Pipeline &&ppl) noexcept;
		Pipeline & operator=(const Pipeline &) = delete;
		Pipeline & operator=(Pipeline &&ppl) noexcept;

		void Draw(Framebuffer &fb,
				  const std::byte *vertex_data,
				  std::size_t count,
				  const State &state,
				  SD &shader_data);

		void DrawIndexed(Framebuffer &fb,
						 const std::byte *vertex_data,
						 const std::uint32_t *index_data,
						 std::size_t count,
						 const State &state,
						 SD &shader_data);
	private:

		Polygon<VO> vertex_shader_evaluation(const std::byte *vertex_data,
											 const std::uint32_t *index_data,
											 std::size_t index,
											 SD &shader_data);

		void clipping_evaluation(Polygon<VO> polygon,
								 hrs::flags<ClipPlane> planes,
								 Framebuffer &fb,
								 const State &state,
								 SD &shader_data);

		void homogenous_division(Polygon<VO> &polygon);

		void viewport_transform(Polygon<VO> &polygon, const Viewport &viewport);

		bool culling_evaluation(CullSide cull_side, CullOrder cull_order, const Polygon<VO> &polygon) noexcept;

		bool is_depth_test_passed(const Image *depth_image,
								  const hrs::math::vector<std::int64_t, 2> &position,
								  float test_z) const noexcept;

		void rasterization_line_brezenham(const Polygon<VO> &polygon,
										  Framebuffer &fb,
										  bool depth_test_enable,
										  SD &shader_data);

		void rasterization_fill(const Polygon<VO> &polygon,
								Framebuffer &fb,
								bool depth_test_enable,
								SD &shader_data);

		void set_framebuffer_output(Framebuffer &fb,
									const hrs::math::vector<std::int64_t, 2> &position,
									const FragmentOutput<ATTACHMENT_COUNT> &output,
									float depth);


		std::size_t vertex_data_stride;
		std::function<VertexShader> vertex_shader;
		std::function<FragmentShader> fragment_shader;
	};

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	template<std::invocable<std::uint32_t, const std::byte *, VO &, SD &> V,
			  std::invocable<const VO &,
							 const hrs::math::vector<std::int64_t, 2> &,
							 float,
							 FragmentOutput<ATTACHMENT_COUNT> &,
							 SD &> F>
	Pipeline<VO, ATTACHMENT_COUNT, SD>::Pipeline(std::size_t _vertex_data_stride,
				 V &&_vertex_shader,
				 F &&_fragment_shader)
		: vertex_data_stride(_vertex_data_stride),
		  vertex_shader(_vertex_shader),
		  fragment_shader(_fragment_shader) {}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	Pipeline<VO, ATTACHMENT_COUNT, SD>::Pipeline(Pipeline &&ppl) noexcept
		: vertex_data_stride(ppl.vertex_data_stride),
		  vertex_shader(std::move(ppl.vertex_shader)),
		  fragment_shader(std::move(ppl.fragment_shader)) {}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	Pipeline<VO, ATTACHMENT_COUNT, SD> & Pipeline<VO, ATTACHMENT_COUNT, SD>::operator=(Pipeline &&ppl) noexcept
	{
		vertex_data_stride = ppl.vertex_data_stride;
		vertex_shader = std::move(ppl.vertex_shader);
		fragment_shader = std::move(ppl.fragment_shader);

		return *this;
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void Pipeline<VO, ATTACHMENT_COUNT, SD>::Draw(Framebuffer &fb,
												  const std::byte *vertex_data,
												  std::size_t count,
												  const State &state,
												  SD &shader_data)
	{
		assert(count % 3 == 0);
		for(std::size_t i = 0; i < count; i += 3)
		{
			auto polygon = vertex_shader_evaluation(vertex_data, nullptr, i, shader_data);
			clipping_evaluation(polygon, {}, fb, state, shader_data);
		}
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void Pipeline<VO, ATTACHMENT_COUNT, SD>::DrawIndexed(Framebuffer &fb,
														 const std::byte *vertex_data,
														 const std::uint32_t *index_data,
														 std::size_t count,
														 const State &state,
														 SD &shader_data)
	{
		assert(count % 3 == 0);
		for(std::size_t i = 0; i < count; i += 3)
		{
			auto polygon = vertex_shader_evaluation(vertex_data, index_data, i, shader_data);
			clipping_evaluation(polygon, {}, fb, state, shader_data);
		}
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	Polygon<VO> Pipeline<VO, ATTACHMENT_COUNT, SD>::vertex_shader_evaluation(const std::byte *vertex_data,
																			 const std::uint32_t *index_data,
																			 std::size_t index,
																			 SD &shader_data)
	{
		Polygon<VO> polygon;
		for(std::uint32_t i = 0; i < 3; i++)
		{
			std::uint32_t vertex_index = (index_data ? index_data[index + i] : index + i);
			polygon.vertices[i].vertex = vertex_shader(index + i,
													   vertex_data + vertex_index * vertex_data_stride,
													   polygon.vertices[i].attributes,
													   shader_data);
		}

		return polygon;
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void Pipeline<VO, ATTACHMENT_COUNT, SD>::clipping_evaluation(Polygon<VO> polygon,
																 hrs::flags<ClipPlane> planes,
																 Framebuffer &fb,
																 const State &state,
																 SD &shader_data)
	{
		for(hrs::flags<ClipPlane> plane = ClipPlane::POSITIVE_W; plane != ClipPlane::MAX_PLANE; plane <<= 1)
		{
			if(plane & planes)
				continue;

			//clip
			std::pair<Polygon<VO>, Polygon<VO>> clip_polygons;
			auto clip_res = polygon.ClipAgainstPlane(plane, clip_polygons);
			planes |= plane;
			switch(clip_res)
			{
				case ClipResult::InsidePlane:
					break;
				case ClipResult::OutsidePlane:
					return;
					break;
				case ClipResult::OneResult:
					clipping_evaluation(clip_polygons.first,
										planes,
										fb,
										state,
										shader_data);
					return;
					break;
				case ClipResult::TwoResult:
					clipping_evaluation(clip_polygons.first,
										planes,
										fb,
										state,
										shader_data);
					clipping_evaluation(clip_polygons.second,
										planes,
										fb,
										state,
										shader_data);
					return;
					break;
			}

		}

		homogenous_division(polygon);
		viewport_transform(polygon, state.viewport);
		if(culling_evaluation(state.cull_side, state.cull_order, polygon))
			return;

		if(state.topology == RasterizationTopology::Line)
			rasterization_line_brezenham(polygon, fb, state.depth_test_enable, shader_data);
		else
			rasterization_fill(polygon, fb, state.depth_test_enable, shader_data);
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void Pipeline<VO, ATTACHMENT_COUNT, SD>::homogenous_division(Polygon<VO> &polygon)
	{
		for(int i = 0; i < 3; i++)
		{
			float inv_w = 1.0f / polygon.vertices[i].vertex[3];
			polygon.vertices[i].vertex[0] *= inv_w;
			polygon.vertices[i].vertex[1] *= inv_w;
			polygon.vertices[i].vertex[2] *= inv_w;
			polygon.vertices[i].vertex[3] = inv_w;
			polygon.vertices[i].attributes *= inv_w;
		}
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void Pipeline<VO, ATTACHMENT_COUNT, SD>::viewport_transform(Polygon<VO> &polygon, const Viewport &viewport)
	{
#warning ASSUME THAT Z-COMPONENT IS ALREADY IN [0, 1] RANGE!!!
		float half_width = static_cast<float>(viewport.GetWidth()) / 2;
		float half_height = static_cast<float>(viewport.GetHeight()) / 2;
		float depth_delta = viewport.GetMaxDepth() - viewport.GetMinDepth();

		for(auto &vert : polygon.vertices)
		{
			vert.vertex[0] = (vert.vertex[0] + 1) * half_width + viewport.GetX();
			vert.vertex[1] = (vert.vertex[1] - 1) * -half_height + viewport.GetY();
			vert.vertex[2] = depth_delta * vert.vertex[2] + viewport.GetMinDepth();
		}
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	bool Pipeline<VO, ATTACHMENT_COUNT, SD>::culling_evaluation(CullSide cull_side,
																CullOrder cull_order,
																const Polygon<VO> &polygon) noexcept
	{
		/*if(cull_side == CullSide::None)
			return false;

		hrs::math::vector<std::int64_t, 2> v10(polygon.vertices[1].vertex[0] - polygon.vertices[0].vertex[0],
											   polygon.vertices[1].vertex[1] - polygon.vertices[0].vertex[1]);

		hrs::math::vector<std::int64_t, 2> v20(polygon.vertices[2].vertex[0] - polygon.vertices[0].vertex[0],
											   polygon.vertices[2].vertex[1] - polygon.vertices[0].vertex[1]);

		float screen_z = v20[0] * v10[1] - v20[1] * v10[0];

		if(cull_side == CullSide::Back)
		{
			if(cull_order == CullOrder::ClockWise)
				return screen_z >= 0;
			else
				return screen_z <= 0;
		}
		else
		{
			if(cull_order == CullOrder::ClockWise)
				return screen_z <= 0;
			else
				return screen_z >= 0;
		}*/

		return false;
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	bool Pipeline<VO, ATTACHMENT_COUNT, SD>::is_depth_test_passed(const Image *depth_image,
																  const hrs::math::vector<std::int64_t, 2> &position,
																  float test_z) const noexcept
	{
		float ref_z = depth_image->GetValueDepth(position[0], position[1]);
		if(std::isnan(ref_z) || ref_z < test_z)
			return false;

		return true;
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void Pipeline<VO, ATTACHMENT_COUNT, SD>::rasterization_line_brezenham(const Polygon<VO> &polygon,
																		  Framebuffer &fb,
																		  bool depth_test_enable,
																		  SD &shader_data)
	{
		//sort vertices!!!
		constexpr std::pair<int, int> lines[] = {{0, 1}, {1, 2}, {2, 0}};
		for(const auto &line : lines)
		{
			hrs::math::vector<std::int64_t, 2> start(polygon.vertices[line.first].vertex);
			float start_z = polygon.vertices[line.first].vertex[2];
			float start_w = polygon.vertices[line.first].vertex[3];
			VO start_attributes = polygon.vertices[line.first].attributes;

			hrs::math::vector<std::int64_t, 2> end(polygon.vertices[line.second].vertex);
			float end_z = polygon.vertices[line.second].vertex[2];
			float end_w = polygon.vertices[line.second].vertex[3];
			VO end_attributes = polygon.vertices[line.second].attributes;

			std::int64_t dx = end[0] - start[0];
			std::int64_t dy = end[1] - start[1];
			std::int64_t xs = 0, ys = 0;
			std::int64_t hd;
			if(dx < 0) {xs = -1; dx = -dx;}
			else xs = 1;
			if(dy < 0) {ys = -1; dy = - dy;}
			else ys = 1;

			std::int64_t major_axis = (dx > dy ? dx : dy);
			std::int64_t minor_axis = (dx > dy ? dy : dx);
			std::int64_t tmp_minor_axis = minor_axis;
			int major_index = (dx > dy ? 0 : 1);
			int minor_index = (dx > dy ? 1 : 0);
			std::int64_t major_step = (dx > dy ? xs : ys);
			std::int64_t minor_step = (dx > dy ? ys : xs);

			float step_z;
			float step_w;
			VO step_attributes;
			FragmentOutput<ATTACHMENT_COUNT> fragment_output;

			Image *depth_image = fb.GetDepthImage();

			step_z = (end_z - start_z) / major_axis;
			step_w = (end_w - start_w) / major_axis;
			step_attributes = (end_attributes - start_attributes) / major_axis;

			auto mul = []<typename T>(const T &value, const T &step, std::size_t i)
			{
				return value + step * i;
			};

			std::size_t i = 0;
			while(start[major_index] != end[major_index])
			{
				float true_z = 1.0f / mul(start_w, step_w, i);
				if(depth_test_enable && depth_image && is_depth_test_passed(depth_image, start, mul(start_z, step_z, i)))
				{
					fragment_shader(mul(start_attributes, step_attributes, i) * true_z, start, mul(start_z, step_z, i), fragment_output, shader_data);
					set_framebuffer_output(fb, start, fragment_output, start_z);
				}

				start[major_index] += major_step;
				//start_z += step_z;
				//start_w += step_w;
				//start_attributes += step_attributes;
				minor_axis += tmp_minor_axis;
				if(minor_axis >= major_axis)
				{
					minor_axis -= major_axis;
					start[minor_index] += minor_step;
				}

				i++;
			}
		}
	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void Pipeline<VO, ATTACHMENT_COUNT, SD>::rasterization_fill(const Polygon<VO> &polygon,
																Framebuffer &fb,
																bool depth_test_enable,
																SD &shader_data)
	{

	}

	template<LinearInterpolatable VO, std::size_t ATTACHMENT_COUNT, typename SD>
	void
	Pipeline<VO, ATTACHMENT_COUNT, SD>::set_framebuffer_output(Framebuffer &fb,
															   const hrs::math::vector<std::int64_t, 2> &position,
															   const FragmentOutput<ATTACHMENT_COUNT> &output,
															   float depth)
	{
		for(std::size_t i = 0; i < output.attachments.size(); i++)
		{
			auto *color_img = fb.GetColorImage(i);
			if(color_img)
				color_img->SetValueColor(position[0], position[1], output.attachments[i]);

			auto *depth_img = fb.GetDepthImage();
			if(depth_img)
				depth_img->SetValueDepth(position[0], position[1], depth);
		}
	}
};
