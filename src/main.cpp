#include "hrs/math/matrix.hpp"
#include "hrs/math/vector.hpp"
#include "hrs/math/quaternion.hpp"
#include <SDL2/SDL.h>
#include <cstring>
#include <iostream>
#include "Render/RenderableMesh.h"
#include "Wavefront/ObjParser.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../sdk/stb_image/stb_image.h"

#include "RendererBackend/Pipeline.hpp"

bool is_run = true;
constexpr inline static float NEAR = 0.1f;
constexpr inline static float FAR = 100.0f;
constexpr inline static float FOV = 75.0f;

auto view_rotate = hrs::math::glsl::std430::mat4x4::identity();
auto view_translate = hrs::math::glsl::std430::mat4x4::identity();

struct ShaderData
{
	hrs::math::glsl::std430::mat4x4 projection_matrix;
	hrs::math::glsl::std430::mat4x4 view_matrix;
	hrs::math::glsl::std430::mat4x4 model_matrix = hrs::math::glsl::std430::mat4x4::identity();
} shader_data;

struct RendererObjects
{
	Renderer::Image color_image;
	Renderer::Image depth_image;
	Renderer::Framebuffer framebuffer;
	Renderer::Viewport viewport;
} renderer_objects;

Renderer::State pipeline_state(Renderer::RasterizationTopology::Line,
							   true,
							   renderer_objects.viewport,
							   Renderer::CullSide::Back,
							   Renderer::CullOrder::ClockWise);


Renderer::Format SurfaceFormatToRendererFormat(SDL_PixelFormatEnum surface_format)
{
	switch(surface_format)
	{
		case SDL_PixelFormatEnum::SDL_PIXELFORMAT_XRGB8888:
			return Renderer::Format::ARGB32_PACKED;
			break;
		case SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBX8888:
			return Renderer::Format::RGBA32_PACKED;
			break;
		case SDL_PixelFormatEnum::SDL_PIXELFORMAT_BGRX8888:
			return Renderer::Format::BGRA32_PACKED;
			break;
		case SDL_PixelFormatEnum::SDL_PIXELFORMAT_ARGB8888:
			return Renderer::Format::ARGB32_PACKED;
			break;
		case SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA8888:
			return Renderer::Format::RGBA32_PACKED;
			break;
		case SDL_PixelFormatEnum::SDL_PIXELFORMAT_ABGR8888:
			return Renderer::Format::ABGR32_PACKED;
			break;
		case SDL_PixelFormatEnum::SDL_PIXELFORMAT_BGRA8888:
			return Renderer::Format::BGRA32_PACKED;
			break;
		default:
			assert(false);
			break;
	}
}

float deg_to_rad(float deg)
{
	return deg * std::numbers::pi_v<float> / 180;
}

float GetAspect(SDL_Window *window) noexcept
{
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	return static_cast<float>(w) / h;
}

hrs::math::glsl::std430::mat4x4 Perspective(float fov, float aspect, float near, float far) noexcept
{
	float half_fov_tan = std::tan(fov / 2);
	float top = near * half_fov_tan;
	float right = top * aspect;

	hrs::math::glsl::std430::mat4x4 out_mat;
	out_mat[0][0] = near / right;
	out_mat[1][1] = near / top;
	out_mat[2][2] = -(far) / (near - far);
	out_mat[2][3] = 1;
	out_mat[3][2] = (far * near) / (near - far);

	return out_mat;
}

void SDLEventPoll(SDL_Window *window, SDL_Surface *&surface)
{
	static bool is_camera_active = false;
	static int x_start = 0;
	static int y_start = 0;
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_QUIT:
				is_run = false;
				break;
			case SDL_KEYDOWN:
				switch(ev.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						if(!is_camera_active)
						{
							SDL_GetMouseState(&x_start, &y_start);
							SDL_ShowCursor(SDL_DISABLE);
						}
						else
							SDL_ShowCursor(SDL_ENABLE);

						is_camera_active = !is_camera_active;
						break;
					case SDLK_q:
						is_run = false;
						break;
					case SDLK_f:
						//polygon_mode = (polygon_mode == GL_LINE ? GL_FILL : GL_LINE);
						break;
				}
				break;
			case SDL_MOUSEMOTION:
				{
					if(is_camera_active)
					{
						static float x_angle = 0;
						static float y_angle = 0;
						float dtx = x_start - ev.motion.x;
						float dty = y_start - ev.motion.y;
						if(!(dtx == 0.0f && dty == 0.0f))
						{
							x_angle += static_cast<float>(dtx) * 0.05f;
							y_angle += static_cast<float>(dty) * 0.05f;
							hrs::math::glsl::vec3 x_axis(1, 0, 0);
							hrs::math::quaternion<float> q_x(x_axis, deg_to_rad(y_angle));
							auto rotate_matrix_x = q_x.to_matrix();
							//const auto &y_axis = rotate_matrix_x[1];
							hrs::math::glsl::vec3 y_axis(0, 1, 0);
							hrs::math::quaternion<float> q_y(y_axis, deg_to_rad(x_angle));
							view_rotate = rotate_matrix_x * q_y.to_matrix();

							SDL_WarpMouseInWindow(window, x_start, y_start);
						}
					}
				}
				break;
			case SDL_WINDOWEVENT:
				switch(ev.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
						shader_data.projection_matrix =
							Perspective(deg_to_rad(FOV),
										static_cast<float>(ev.window.data1) / ev.window.data2,
										NEAR,
										FAR);

						surface = SDL_GetWindowSurface(window);
						renderer_objects.color_image.Resize(ev.window.data1,
															ev.window.data2,
															SurfaceFormatToRendererFormat(static_cast<SDL_PixelFormatEnum>(surface->format->format)));

						renderer_objects.depth_image.Resize(ev.window.data1,
															ev.window.data2,
															Renderer::Format::DEPTH32_SFLOAT);

						renderer_objects.viewport = Renderer::Viewport(ev.window.data1,
																	   ev.window.data2,
																	   0,
																	   0,
																	   0,
																	   1);

						pipeline_state.viewport = renderer_objects.viewport;
						break;
				}

				break;

		}
	}
}

void HandleMovement()
{
	auto state = SDL_GetKeyboardState(nullptr);
	if(state[SDL_GetScancodeFromKey(SDLK_w)])
		view_translate[3] -= view_rotate[2] * 0.1f;

	if(state[SDL_GetScancodeFromKey(SDLK_s)])
		view_translate[3] += view_rotate[2] * 0.1f;

	if(state[SDL_GetScancodeFromKey(SDLK_d)])
		view_translate[3] -= view_rotate[0] * 0.1f;

	if(state[SDL_GetScancodeFromKey(SDLK_a)])
		view_translate[3] += view_rotate[0] * 0.1f;

	if(state[SDL_GetScancodeFromKey(SDLK_u)])
		view_translate[3] -= view_rotate[1] * 0.1f;

	if(state[SDL_GetScancodeFromKey(SDLK_b)])
		view_translate[3] += view_rotate[1] * 0.1f;
}

hrs::math::glsl::std430::mat4x4 Translate(float x = 0, float y = 0, float z = 0) noexcept
{
	auto out_mat = hrs::math::glsl::std430::mat4x4::identity();
	out_mat[3][0] = x;
	out_mat[3][1] = y;
	out_mat[3][2] = z;

	return out_mat;
}

/*void AppendMaterialLib(const MaterialLib &lib, std::map<MaterialTreeKey, std::unique_ptr<Material>> &materials)
{
	for(const auto &mtl : lib.GetMaterials())
	{
		MaterialTreeKey key(lib.GetName(), mtl.name);
		auto it = materials.find(key);
		if(it != materials.end())
			continue;

		std::filesystem::path diffuse_path = "../../gamedata/textures/" + mtl.diffuse_map;

		int diffuse_width, diffuse_height, diffuse_channels;
		auto diffuse_format = GL_RGB;
		auto diffuse_data = stbi_load(diffuse_path.c_str(), &diffuse_width, &diffuse_height, &diffuse_channels, 0);
		if(!diffuse_data)
			throw std::runtime_error("Bad texture!");


		std::cout<<"Channels: "<<diffuse_channels<<std::endl;
		//read!!!

		//const void *data = nullptr;
		//int width, height, format;
		std::unique_ptr<Material> u_material(new Material);
		u_material->Create(diffuse_data, diffuse_width, diffuse_height, diffuse_format);
		materials.insert({key, std::move(u_material)});

		stbi_image_free(diffuse_data);
	}
}*/

int main()
{
	stbi_set_flip_vertically_on_load(true);

	auto init_res = SDL_Init(SDL_INIT_EVERYTHING);
	if(init_res)
	{
		std::cout<<SDL_GetError()<<std::endl;
		exit(-1);
	}

	SDL_Window *window = SDL_CreateWindow("title",
										  SDL_WINDOWPOS_UNDEFINED,
										  SDL_WINDOWPOS_UNDEFINED,
										  800,
										  600,
										  SDL_WINDOW_RESIZABLE);

	if(!window)
	{
		std::cout<<SDL_GetError()<<std::endl;
		return 1;
	}

	SDL_Surface *surface = SDL_GetWindowSurface(window);
	if(!surface)
	{
		std::cout<<SDL_GetError()<<std::endl;
		return 1;
	}

	shader_data.projection_matrix = Perspective(deg_to_rad(FOV), GetAspect(window), NEAR, FAR);

	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	renderer_objects.color_image.Resize(w,
										h,
										SurfaceFormatToRendererFormat(static_cast<SDL_PixelFormatEnum>(surface->format->format)));

	renderer_objects.depth_image.Resize(w, h, Renderer::Format::DEPTH32_SFLOAT);
	Renderer::Image * color_images[] = {&renderer_objects.color_image};
	renderer_objects.framebuffer = Renderer::Framebuffer(color_images, &renderer_objects.depth_image);
	renderer_objects.viewport = Renderer::Viewport(w, h, 0, 0, 0, 1);

	struct VertexData
	{
		hrs::math::glsl::vec3 position;
		hrs::math::glsl::vec2 texture;
		hrs::math::glsl::vec3 normal;
	};

	struct VertexShaderOutput
	{
		hrs::math::glsl::vec3 color;

		VertexShaderOutput & operator*=(float value) noexcept
		{
			color *= value;
			return *this;
		}

		VertexShaderOutput operator*(float value) const noexcept
		{
			return VertexShaderOutput(color * value);
		}

		VertexShaderOutput operator/(std::int64_t value) const noexcept
		{
			return VertexShaderOutput(color / value);
		}

		VertexShaderOutput operator-(const VertexShaderOutput &vso) const noexcept
		{
			return VertexShaderOutput(color - vso.color);
		}

		VertexShaderOutput operator+(const VertexShaderOutput &vso) const noexcept
		{
			return VertexShaderOutput(color + vso.color);
		}

		VertexShaderOutput & operator+=(const VertexShaderOutput &vso) noexcept
		{
			color += vso.color;
			return *this;
		}
	};

	auto vertex_shader = [](std::uint32_t vertex_index,
							const std::byte *vertex_input,
							VertexShaderOutput &vertex_output,
							ShaderData &shader_data) -> hrs::math::glsl::vec4
	{
		const VertexData *vertex_data = reinterpret_cast<const VertexData *>(vertex_input);
		vertex_output.color = vertex_data->texture;
		return hrs::math::glsl::vec4(vertex_data->position[0],
									 vertex_data->position[1],
									 vertex_data->position[2],
									 1.0f) *
			   shader_data.model_matrix *
			   shader_data.view_matrix *
			   shader_data.projection_matrix;
	};

	auto fragment_shader = [](const VertexShaderOutput &vertex_output,
							  const hrs::math::vector<std::int64_t, 2> &frag_position,
							  float frag_depth,
							  Renderer::FragmentOutput<1> &fragment_output,
							  ShaderData &shader_data)
	{
		fragment_output.attachments[0][0] = vertex_output.color[0];
		fragment_output.attachments[0][1] = vertex_output.color[1];
		fragment_output.attachments[0][2] = 0;
		fragment_output.attachments[0][3] = 0;
	};

	Renderer::Pipeline<VertexShaderOutput, 1, ShaderData> pipeline(sizeof(VertexData),
																   vertex_shader,
																   fragment_shader);

	ObjParser obj_parser;
	MeshVertexIndexData mesh_data;
	RenderableMesh render_mesh;

	try
	{
		auto mesh = obj_parser.Parse("../../gamedata/objects/stk.obj");
		mesh_data = mesh.CreateData();
		render_mesh.Create(mesh_data);
	}
	catch(const ObjParserError &ex)
	{
		std::cout<<ObjParserResultToString(ex.result)<<" on "<<ex.col<<std::endl;
		return 1;
	}
	catch(const std::exception &ex)
	{
		std::cout<<ex.what()<<std::endl;
		return 1;
	}
	catch(...)
	{
		std::cout<<"Unmanaged exception!"<<std::endl;
		return 1;
	}

	shader_data.model_matrix[3][2] += 4.f;
	pipeline_state.viewport = renderer_objects.viewport;
	while(is_run)
	{
		SDLEventPoll(window, surface);
		HandleMovement();

		shader_data.view_matrix = view_translate * view_rotate.transpose();

		const Renderer::ClearValue clear_value(hrs::math::glsl::vec4(0.33f, 0.33f, 0.33f, 0));
		renderer_objects.framebuffer.ClearImage(clear_value, 0);
		renderer_objects.framebuffer.ClearDepthImage(1.0f);

		for(const auto &part : render_mesh.GetParts())
		{
			pipeline.DrawIndexed(renderer_objects.framebuffer,
								 render_mesh.GetVertexData().data(),
								 render_mesh.GetIndexData().data() + part.offset,
								 part.count,
								 pipeline_state,
								 shader_data);
		}

		int lock_res = SDL_LockSurface(surface);
		if(lock_res)
		{
			std::cout<<SDL_GetError()<<std::endl;
			return 1;
		}

		std::memcpy(surface->pixels,
					renderer_objects.color_image.GetMappedPtr(),
					renderer_objects.color_image.GetWidth() * renderer_objects.color_image.GetHeight() * 4);

		SDL_UnlockSurface(surface);

		SDL_UpdateWindowSurface(window);
#warning RASTERIZATION width - 1 and height - 1!!!
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
