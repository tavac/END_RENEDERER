#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "renderer.h"
#include "view.h"
#include "blob.h"
#include "../Renderer/shaders/mvp.hlsli"

// NOTE: This header file must *ONLY* be included by renderer.cpp

#define FREE_POOL_TEST 0
#define SORTED_POOL_TEST 0
#define RENDER_PARTICLES 0
#define LOOK_AT 1

namespace
{
	template<typename T>
	void safe_release(T* t)
	{
		if (t)
			t->Release();
	}
}

namespace end
{
	using namespace DirectX;

	struct Axis_Vectors
	{
		XMVECTOR x = { 1.0f,0.0f,0.0f, 1.0f };
		XMVECTOR y = { 0.0f,1.0f,0.0f, 1.0f };
		XMVECTOR z = { 0.0f,0.0f,1.0f, 1.0f };
	};

	void mtx_stabilize(XMMATRIX& mtx)
	{
		// Take Z vector from mtx
		XMVECTOR Z = mtx.r[2];
		Z = XMVector3Normalize(Z);

		XMVECTOR nX = XMVector3Cross(W_UP, Z);
		nX = XMVector3Normalize(nX);

		XMVECTOR nY = XMVector3Cross(Z, nX);
		nY = XMVector3Normalize(nY);

		mtx.r[0] = nX;
		mtx.r[1] = nY;
		mtx.r[2] = Z;
	}

	void look_at(XMMATRIX& mtx, XMMATRIX& tgt)
	{
		XMVECTOR nZ = tgt.r[3] - mtx.r[3]; // Vector From Target to "View"
		mtx.r[2] = XMVector3Normalize(nZ);
		mtx_stabilize(mtx);
	}

	void turn_to(XMMATRIX& mtx, XMMATRIX& tgt)
	{
		XMVECTOR nZ = tgt.r[3] - mtx.r[3]; // Vector From Target to "View"

		XMVECTOR HowMuchThisWayIsThatThingGoing = XMVector3Dot(XMVector3Normalize(nZ),mtx.r[0]);

		float degree = HowMuchThisWayIsThatThingGoing.m128_f32[0] * (3.14156f / 180.0f);

		XMMATRIX rotationY = XMMatrixRotationAxis(mtx.r[1], degree);
		mtx = XMMatrixMultiply(rotationY,mtx);

		HowMuchThisWayIsThatThingGoing = XMVector3Dot(XMVector3Normalize(nZ), -mtx.r[1]);

		degree = HowMuchThisWayIsThatThingGoing.m128_f32[0] * (3.14156f / 180.0f);

		XMMATRIX rotationX = XMMatrixRotationAxis(XMVector3Normalize(mtx.r[0]), degree);
		mtx = XMMatrixMultiply(rotationX, mtx);
		mtx_stabilize(mtx);
	}

	void matrix_controller_wasd(XMMATRIX& mtx, float dT, bool stabilize = false)
	{
		dT *= 10.f;
#pragma region MOVEMENT
		if (GetAsyncKeyState('W'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, dT), mtx);
		}
		if (GetAsyncKeyState('S'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, -dT), mtx);
		}
		if (GetAsyncKeyState('D'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(dT, 0.0f, 0.0f), mtx);
		}
		if (GetAsyncKeyState('A'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(-dT, 0.0f, 0.0f), mtx);
		}
#pragma endregion

#pragma region ROTATION
		dT *= 0.1f;
		if (GetAsyncKeyState(VK_LEFT))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationY(-dT), mtx);
		}
		if (GetAsyncKeyState(VK_RIGHT))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationY(dT), mtx);
		}
		if (GetAsyncKeyState(VK_UP))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationX(-dT), mtx);
		}
		if (GetAsyncKeyState(VK_DOWN))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationX(dT), mtx);
		}
#pragma endregion

#pragma region CALIBRATION
		if (stabilize)
			mtx_stabilize(mtx);
#pragma endregion
	}

	void matrix_controller_ijkl(XMMATRIX& mtx, float dT, bool stabilize = false)
	{
		dT *= 10.f;
#pragma region MOVEMENT
		if (GetAsyncKeyState('I'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, dT), mtx);
		}
		if (GetAsyncKeyState('K'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, -dT), mtx);
		}
		if (GetAsyncKeyState('L'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(dT, 0.0f, 0.0f), mtx);
		}
		if (GetAsyncKeyState('J'))
		{
			mtx = XMMatrixMultiply(XMMatrixTranslation(-dT, 0.0f, 0.0f), mtx);
		}
#pragma endregion

#pragma region ROTATION
		dT *= 0.1f;
		if (GetAsyncKeyState(VK_NUMPAD4))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationY(-dT), mtx);
		}
		if (GetAsyncKeyState(VK_NUMPAD6))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationY(dT), mtx);
		}
		if (GetAsyncKeyState(VK_NUMPAD8))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationX(-dT), mtx);
		}
		if (GetAsyncKeyState(VK_NUMPAD2))
		{
			mtx = XMMatrixMultiply(XMMatrixRotationX(dT), mtx);
		}
#pragma endregion

#pragma region CALIBRATION
		if (stabilize)
			mtx_stabilize(mtx);
#pragma endregion
	}

	void draw_axi(XMMATRIX mtx)
	{
		Axis_Vectors av;
		av.x = XMVector4Transform(av.x, mtx);
		av.y = XMVector4Transform(av.y, mtx);
		av.z = XMVector4Transform(av.z, mtx);
		end::debug_renderer::add_line(mtx.r[3], av.x, RED, RED);
		end::debug_renderer::add_line(mtx.r[3], av.y, GREEN, GREEN);
		end::debug_renderer::add_line(mtx.r[3], av.z, BLUE, BLUE);
	}

	struct renderer_t::impl_t
	{
		// platform/api specific members, functions, etc.
		// Device, swapchain, resource views, states, etc. can be members here
		HWND hwnd;

		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		IDXGISwapChain* swapchain = nullptr;

		ID3D11RenderTargetView* render_target[VIEW_RENDER_TARGET::COUNT]{};

		ID3D11DepthStencilView* depthStencilView[VIEW_DEPTH_STENCIL::COUNT]{};

		ID3D11DepthStencilState* depthStencilState[STATE_DEPTH_STENCIL::COUNT]{};

		ID3D11RasterizerState* rasterState[STATE_RASTERIZER::COUNT]{};

		ID3D11Buffer* vertex_buffer[VERTEX_BUFFER::COUNT]{};

		ID3D11Buffer* index_buffer[INDEX_BUFFER::COUNT]{};

		ID3D11InputLayout* input_layout[INPUT_LAYOUT::COUNT]{};

		ID3D11VertexShader* vertex_shader[VERTEX_SHADER::COUNT]{};

		ID3D11PixelShader* pixel_shader[PIXEL_SHADER::COUNT]{};

		ID3D11Buffer* constant_buffer[CONSTANT_BUFFER::COUNT]{};

		D3D11_VIEWPORT				view_port[VIEWPORT::COUNT]{};

		/* Add more as needed...
		ID3D11SamplerState*			sampler_state[STATE_SAMPLER::COUNT]{};

		ID3D11BlendState*			blend_state[STATE_BLEND::COUNT]{};
		*/

		ID3D11Buffer* debugline_constBuff[CONSTANT_BUFFER::COUNT]{};

#if FREE_POOL_TEST
		pool_t<Particle, 100> fp_test;
#endif

#if SORTED_POOL_TEST
		sorted_pool_t<Particle, 100> sp_test;
#endif

#if RENDER_PARTICLES
		///////////// PARTICLES /////////////////////
		Emitter emitters[NUM_OF_EMITTERS];
		pool_t<Particle, numOfParticles * 3> particles;
		/////////////////////////////////////////////
#endif

#if LOOK_AT
		XMMATRIX m_1 = XMMatrixIdentity();
		XMMATRIX m_2 = XMMatrixIdentity();
#endif
		XTime timer;

		// Constructor for renderer implementation
		// 
		impl_t(native_handle_type window_handle, view_t& default_view)
		{
			hwnd = (HWND)window_handle;

			create_device_and_swapchain();

			create_main_render_target();

			setup_depth_stencil();

			setup_rasterizer();

			create_shaders();

			create_constant_buffers();

			float aspect = view_port[VIEWPORT::DEFAULT].Width / view_port[VIEWPORT::DEFAULT].Height;

			XMVECTOR eyepos = XMVectorSet(0.0f, 15.0f, -15.0f, 1.0f);
			XMVECTOR focus = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

			default_view.view_mat = (float4x4_a&)XMMatrixInverse(nullptr, XMMatrixLookAtLH(eyepos, focus, up));
			default_view.proj_mat = (float4x4_a&)XMMatrixPerspectiveFovLH(3.1415926f / 4.0f, aspect, 0.01f, 100.0f);

#if RENDER_PARTICLES
			/////////////////// PARTICLE CREATION ///////////////////
			create_emitters(0, W_ORIGIN, WHITE);
			create_emitters(1, { 10,0,0 }, WHITE);
			create_emitters(2, { -10,0,0 }, WHITE);
			create_particles(/*NUM_OF_EMITTERS, 0,*/ W_UP, WHITE);
			/////////////////////////////////////////////////////////
#endif
			timer.Restart();
		}
		bool LookAt = false;
		void draw_view(view_t& view)
		{
			// TIMER Update //
			timer.Signal();
			float deltaT = timer.Delta();
			/////////////////

			// Fill Color
			const float4 black{ 0.0f, 0.0f, 0.0f, 1.0f };

			context->OMSetDepthStencilState(depthStencilState[STATE_DEPTH_STENCIL::DEFAULT], 1);
			context->OMSetRenderTargets(1, &render_target[VIEW_RENDER_TARGET::DEFAULT], depthStencilView[VIEW_DEPTH_STENCIL::DEFAULT]);

			context->ClearRenderTargetView(render_target[VIEW_RENDER_TARGET::DEFAULT], black.data());
			context->ClearDepthStencilView(depthStencilView[VIEW_DEPTH_STENCIL::DEFAULT], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			// CLEARING DEBUG LINES // 
			end::debug_renderer::clear_lines();
			//////////////////////////

			context->RSSetState(rasterState[STATE_RASTERIZER::DEFAULT]);
			context->RSSetViewports(1, &view_port[VIEWPORT::DEFAULT]);

			context->VSSetShader(vertex_shader[VERTEX_SHADER::BUFFERLESS_CUBE], nullptr, 0);
			context->PSSetShader(pixel_shader[PIXEL_SHADER::BUFFERLESS_CUBE], nullptr, 0);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			context->VSSetConstantBuffers(0, 1, &constant_buffer[CONSTANT_BUFFER::MVP]);

			MVP_t mvp;

			mvp.modeling = XMMatrixTranspose(XMMatrixIdentity());
			mvp.projection = XMMatrixTranspose((XMMATRIX&)view.proj_mat);
			mvp.view = XMMatrixTranspose(XMMatrixInverse(nullptr, (XMMATRIX&)view.view_mat));

			context->UpdateSubresource(constant_buffer[CONSTANT_BUFFER::MVP], 0, NULL, &mvp, 0, 0);

			// The Cube
			//context->Draw(36, 0);

			// Draw Debug Line Stuff //
			draw_debug_grid(view);
			///////////////////////////

#if FREE_POOL_TEST
			end::debug_renderer::clear_lines();
			f_create_test_particles();
			f_update_test_particles(deltaT);
			draw_debug_lines(view);
#endif

#if SORTED_POOL_TEST
			end::debug_renderer::clear_lines();
			create_test_particles();
			update_test_particles(deltaT);
			draw_debug_lines(view);
#endif

#if RENDER_PARTICLES
			//////////////////// Particles ////////////////////
			create_particles(/*NUM_OF_EMITTERS, 0,*/ W_UP, WHITE);
			update_particles(0, deltaT, { 1.0f,0.0f,0.0f,1.0f });

			//create_particles(/*NUM_OF_EMITTERS, 1,*/ W_UP, WHITE);
			update_particles(1, deltaT, { 0.0f,1.0f,0.0f,1.0f });

			//create_particles(/*NUM_OF_EMITTERS, 2,*/ W_UP, WHITE);
			update_particles(2, deltaT, { 0.0f,0.0f,1.0f,1.0f });
			draw_debug_lines(view);
			//////////////////////////////////////////////////
#endif

#if LOOK_AT
			//POINT prev, curr;
			//GetCursorPos(&prev);
			//GetCursorPos(&curr);
			//prev.x;
			matrix_controller_wasd(m_1, deltaT, false);
			matrix_controller_ijkl(m_2, deltaT, true);

			if (GetAsyncKeyState('1'))
				LookAt = true;
			if (GetAsyncKeyState('2'))
				LookAt = false;
			if (LookAt)
				turn_to(m_1, m_2);
				//else
				//look_at(m_1, m_2);

			draw_axi(m_1);
			draw_axi(m_2);
			draw_debug_lines(view);
#endif
			swapchain->Present(1u, 0u);
		}

		void draw_debug_grid(view_t& view)
		{
			// HORIZONTAL LINES
			for (int i = -10; i <= 10; i++)
			{
				float4 pa = { 10.0f	,	0.0f,	(float)i, 1.0f };
				float4 pb = { -10.0f,	0.0f,	(float)i, 1.0f };
				float4 ca = { 0.0f	,	1.0f,	0.0f,	1.0f };
				float4 cb = { 0.0f	,	1.0f,	0.0f,	1.0f };

				end::debug_renderer::add_line(pa, pb, ca, cb);
			}
			// VERTICAL LINES
			for (int i = -10; i <= 10; i++)
			{
				float4 pa = { (float)i,	0.0f,	10.0f, 1.0f };
				float4 pb = { (float)i,	0.0f,	-10.0f, 1.0f };
				float4 ca = { 0.0f	,	1.0f,	0.0f,	1.0f };
				float4 cb = { 0.0f	,	1.0f,	0.0f,	1.0f };

				end::debug_renderer::add_line(pa, pb, ca, cb);
			}


			context->VSSetShader(vertex_shader[VERTEX_SHADER::COLORED_VERTEX], nullptr, 0);
			context->PSSetShader(pixel_shader[PIXEL_SHADER::COLORED_VERTEX], nullptr, 0);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			context->UpdateSubresource(vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX], 0, nullptr, end::debug_renderer::get_line_verts(), 0, 0);
			const UINT strides = sizeof(colored_vertex);
			const UINT offset = 0u;
			context->IASetVertexBuffers(0u, 1u, &vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX], &strides, &offset);

			context->VSSetConstantBuffers(0, 1, &constant_buffer[CONSTANT_BUFFER::MVP]);

			MVP_t mvp;

			mvp.modeling = XMMatrixTranspose(XMMatrixIdentity());
			mvp.projection = XMMatrixTranspose((XMMATRIX&)view.proj_mat);
			mvp.view = XMMatrixTranspose(XMMatrixInverse(nullptr, (XMMATRIX&)view.view_mat));

			context->UpdateSubresource(constant_buffer[CONSTANT_BUFFER::MVP], 0, nullptr, &mvp, 0, 0);

			context->Draw((UINT)end::debug_renderer::get_line_vert_count(), 0u);
		}

		void draw_debug_lines(view_t& view)
		{
			context->VSSetShader(vertex_shader[VERTEX_SHADER::COLORED_VERTEX], nullptr, 0);
			context->PSSetShader(pixel_shader[PIXEL_SHADER::COLORED_VERTEX], nullptr, 0);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			context->UpdateSubresource(vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX], 0, nullptr, end::debug_renderer::get_line_verts(), 0, 0);
			const UINT strides = sizeof(colored_vertex);
			const UINT offset = 0u;
			context->IASetVertexBuffers(0u, 1u, &vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX], &strides, &offset);

			context->VSSetConstantBuffers(0, 1, &constant_buffer[CONSTANT_BUFFER::MVP]);

			MVP_t mvp;

			mvp.modeling = XMMatrixTranspose(XMMatrixIdentity());
			mvp.projection = XMMatrixTranspose((XMMATRIX&)view.proj_mat);
			mvp.view = XMMatrixTranspose(XMMatrixInverse(nullptr, (XMMATRIX&)view.view_mat));

			context->UpdateSubresource(constant_buffer[CONSTANT_BUFFER::MVP], 0, nullptr, &mvp, 0, 0);

			context->Draw((UINT)end::debug_renderer::get_line_vert_count(), 0u);
		}

#if FREE_POOL_TEST
		void f_create_test_particles()
		{
			int rtn = 0;
			do
			{
				rtn = fp_test.alloc();
				if (0 <= rtn)
				{
					Particle p;
					p.color = RED;
					p.life = 0;
					p.pos = W_UP;
					p.prev_pos = W_ORIGIN;
					fp_test[rtn] = p;
				}
			} while (-1 < rtn);
		}

		void f_update_test_particles(float dt)
		{
			for (int i = 0; i < 100; i++)
			{
				fp_test[i].life += dt;
				if (fp_test[i].life >= 2.0f)
				{
					fp_test.free(i);
					continue;
				}
				fp_test[i].prev_pos = fp_test[i].pos;
				fp_test[i].pos.x += sinf(dt) * sinf(i);
				fp_test[i].pos.y += 0.001f * i;
				fp_test[i].pos.z += sinf(i * 0.01f);

				end::debug_renderer::add_line(fp_test[i].prev_pos, fp_test[i].pos, fp_test[i].color, fp_test[i].color);
			}
		}
#endif

#if SORTED_POOL_TEST
		void create_test_particles()
		{
			int rtn = 0;
			for (int i = 0; i < 1; i++)
			{
				rtn = sp_test.alloc();
				if (0 <= rtn)
				{
					Particle p;
					p.color = BLUE;
					p.life = 0;
					p.pos = W_UP;
					p.prev_pos = W_ORIGIN;
					sp_test[rtn] = p;
				}
				else
					return;
			}
		}

		void update_test_particles(float dt)
		{
			for (int i = 0; i < sp_test.size(); i++)
			{
				sp_test[i].life += dt;
				if (sp_test[i].life >= 2.0f)
				{
					sp_test.free(i);
					continue;
				}
				sp_test[i].prev_pos = sp_test[i].pos;
				sp_test[i].pos.x += sinf(dt) * sinf(i);
				sp_test[i].pos.y += 0.001f * i;
				sp_test[i].pos.z += sinf(i * 0.01f);

				end::debug_renderer::add_line(sp_test[i].prev_pos, sp_test[i].pos, sp_test[i].color, sp_test[i].color);
			}
		}
#endif

#if RENDER_PARTICLES
		void create_emitters(int8_t emitter_index, end::float3 pos, end::float4 color)
		{

			Emitter emitter;
			emitter.origin = pos;
			emitter.init_color = color;
			emitters[emitter_index] = emitter;
		}

		void create_particles(/*int numOfEm, int start_em,*/ end::float3 pos, end::float4 color)
		{
			//int currPrtcle = 0;
			for (int i = 0; i < NUM_OF_EMITTERS; i++)
			{
				//for (; currPrtcle < (numOfParticles / NUM_OF_EMITTERS) * (i + 1); currPrtcle++)
				for (int currPrtcle = 0; currPrtcle < 1; currPrtcle++)
				{
					Particle p;
					p.prev_pos = emitters[i].origin;
					p.pos = emitters[i].origin;
					p.color = color;
					// Check Particle pool for free spot
					int p_index = particles.alloc();
					if (p_index < 0)
						return; // No more particles
					else
					{
						// Check Emitter for free spot
						int e_index = emitters[i].parti_indices.alloc();
						if (e_index < 0)
						{
							particles.free(p_index); // Emitter had no empty spot for this particle so put it back.
							break; // try next emitter
						}
						else
						{
							emitters[i].parti_indices[e_index] = p_index;
							particles[p_index] = p;
						}
					}
				}
			}
		}

		void update_particles(int16_t em_index, float dT, end::float4 nColor)
		{
			// Timer for dispersing particles at different rates
			int size = emitters[em_index].parti_indices.size();
			for (int i = 0; i < size; i++)
			{
				if (particles[emitters[em_index].parti_indices[i]].life > 2.0f)
				{
					kill_particle(&emitters[em_index], i);
					i--;
					continue;
				}


				end::float3 dir;
				// fountain math
				dir.x = (cosf(dT * i) * sinf(-i));// cosf(dT * (3.14156 / 180)) * 0.1f;
				dir.y = 0.001f;
				dir.z = (sinf(dT * i) * sinf(-i));// -cosf(dT * (3.14156 / 180)) * 0.1f;
				float scalar = -.0986f;
				//dir.x = dT;
				//dir.y = dir.x * scalar;
				Particle nP = particles[emitters[em_index].parti_indices[i]];
				nP.prev_pos = nP.pos;
				nP.pos = nP.pos + (dir * scalar);
				nP.color = (nColor);
				nP.color.w = 1.0f;
				nP.life += dT;
				particles[emitters[em_index].parti_indices[i]] = nP;
				end::float4 p = { particles[emitters[em_index].parti_indices[i]].prev_pos.x,
								particles[emitters[em_index].parti_indices[i]].prev_pos.y,
								particles[emitters[em_index].parti_indices[i]].prev_pos.z, 1.0f };
				end::float4 q = { particles[emitters[em_index].parti_indices[i]].pos.x,
								particles[emitters[em_index].parti_indices[i]].pos.y,
								particles[emitters[em_index].parti_indices[i]].pos.z, 1.0f };
				debug_renderer::add_line(p, q, particles[emitters[em_index].parti_indices[i]].color);
			}
		}

		void clear_particles(Emitter* em)
		{
			for (int i = 0; i < em->parti_indices.size(); i++)
			{
				// free particle
				particles.free(em->parti_indices[i]);
				// free indice
				em->parti_indices.free(i);
			}
		}

		void kill_particle(Emitter* em, int index)
		{
			// free particle
			particles.free(em->parti_indices[index]);
			// free indice
			em->parti_indices.free(index);
		}
#endif

		~impl_t()
		{
			// TODO:
			//Clean-up
#if RENDER_PARTICLES
			clear_particles(&emitters[0]);
#endif
			//
			// In general, release objects in reverse order of creation
			for (auto& ptr : constant_buffer)
				safe_release(ptr);

			for (auto& ptr : pixel_shader)
				safe_release(ptr);

			for (auto& ptr : vertex_shader)
				safe_release(ptr);

			for (auto& ptr : input_layout)
				safe_release(ptr);

			for (auto& ptr : index_buffer)
				safe_release(ptr);

			for (auto& ptr : vertex_buffer)
				safe_release(ptr);

			for (auto& ptr : rasterState)
				safe_release(ptr);

			for (auto& ptr : depthStencilState)
				safe_release(ptr);

			for (auto& ptr : depthStencilView)
				safe_release(ptr);

			for (auto& ptr : render_target)
				safe_release(ptr);

			safe_release(context);
			safe_release(swapchain);
			safe_release(device);
		}

		void create_device_and_swapchain()
		{
			RECT crect;
			GetClientRect(hwnd, &crect);

			// Setup the viewport
			D3D11_VIEWPORT& vp = view_port[VIEWPORT::DEFAULT];

			vp.Width = (float)crect.right;
			vp.Height = (float)crect.bottom;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;

			// Setup swapchain
			DXGI_SWAP_CHAIN_DESC sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = 2;
			sd.BufferDesc.Width = crect.right;
			sd.BufferDesc.Height = crect.bottom;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = hwnd;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.Windowed = TRUE;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

			D3D_FEATURE_LEVEL  FeatureLevelsSupported;

			const D3D_FEATURE_LEVEL lvl[] =
			{
				D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1
			};

			UINT createDeviceFlags = 0;

#ifdef _DEBUG
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, lvl, _countof(lvl), D3D11_SDK_VERSION, &sd, &swapchain, &device, &FeatureLevelsSupported, &context);

			if (hr == E_INVALIDARG)
			{
				hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, &lvl[1], _countof(lvl) - 1, D3D11_SDK_VERSION, &sd, &swapchain, &device, &FeatureLevelsSupported, &context);
			}

			assert(!FAILED(hr));
		}

		void create_main_render_target()
		{
			ID3D11Texture2D* pBackBuffer;
			// Get a pointer to the back buffer
			HRESULT hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
				(LPVOID*)& pBackBuffer);

			assert(!FAILED(hr));

			// Create a render-target view
			device->CreateRenderTargetView(pBackBuffer, NULL,
				&render_target[VIEW_RENDER_TARGET::DEFAULT]);

			pBackBuffer->Release();
		}

		void setup_depth_stencil()
		{
			/* DEPTH_BUFFER */
			D3D11_TEXTURE2D_DESC depthBufferDesc;
			ID3D11Texture2D* depthStencilBuffer;

			ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

			depthBufferDesc.Width = (UINT)view_port[VIEWPORT::DEFAULT].Width;
			depthBufferDesc.Height = (UINT)view_port[VIEWPORT::DEFAULT].Height;
			depthBufferDesc.MipLevels = 1;
			depthBufferDesc.ArraySize = 1;
			depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthBufferDesc.SampleDesc.Count = 1;
			depthBufferDesc.SampleDesc.Quality = 0;
			depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthBufferDesc.CPUAccessFlags = 0;
			depthBufferDesc.MiscFlags = 0;

			HRESULT hr = device->CreateTexture2D(&depthBufferDesc, NULL, &depthStencilBuffer);

			assert(!FAILED(hr));

			/* DEPTH_STENCIL */
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;

			ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;

			hr = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView[VIEW_DEPTH_STENCIL::DEFAULT]);

			assert(!FAILED(hr));

			depthStencilBuffer->Release();

			/* DEPTH_STENCIL_DESC */
			D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

			ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

			depthStencilDesc.DepthEnable = true;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			hr = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState[STATE_DEPTH_STENCIL::DEFAULT]);

			assert(!FAILED(hr));
		}

		void setup_rasterizer()
		{
			D3D11_RASTERIZER_DESC rasterDesc;

			ZeroMemory(&rasterDesc, sizeof(rasterDesc));

			rasterDesc.AntialiasedLineEnable = true;
			rasterDesc.CullMode = D3D11_CULL_BACK;
			rasterDesc.DepthBias = 0;
			rasterDesc.DepthBiasClamp = 0.0f;
			rasterDesc.DepthClipEnable = false;
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.FrontCounterClockwise = false;
			rasterDesc.MultisampleEnable = false;
			rasterDesc.ScissorEnable = false;
			rasterDesc.SlopeScaledDepthBias = 0.0f;

			HRESULT hr = device->CreateRasterizerState(&rasterDesc, &rasterState[STATE_RASTERIZER::DEFAULT]);

			assert(!FAILED(hr));
		}

		void create_shaders()
		{
			HRESULT hr;

			//////// CUBE SHADERS ////////
			binary_blob_t vs_blob = load_binary_blob("vs_cube.cso");
			binary_blob_t ps_blob = load_binary_blob("ps_cube.cso");
			hr = device->CreateVertexShader(vs_blob.data(), vs_blob.size(), NULL, &vertex_shader[VERTEX_SHADER::BUFFERLESS_CUBE]);

			assert(!FAILED(hr));

			hr = device->CreatePixelShader(ps_blob.data(), ps_blob.size(), NULL, &pixel_shader[PIXEL_SHADER::BUFFERLESS_CUBE]);

			assert(!FAILED(hr));

			const D3D11_INPUT_ELEMENT_DESC inputDesc[] =
			{
				{"SV_VertexID",0,DXGI_FORMAT_R32_UINT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
			};
			hr = device->CreateInputLayout(inputDesc, 1, vs_blob.data(), vs_blob.capacity(), &input_layout[INPUT_LAYOUT::BUFFERLESS_CUBE]);

			assert(!FAILED(hr));

			context->IASetInputLayout(input_layout[INPUT_LAYOUT::BUFFERLESS_CUBE]);
			///

			/////// DEBUG LINES SHADERS ///////
			binary_blob_t vs_debug_blob = load_binary_blob("debug_line_vs.cso");
			binary_blob_t ps_debug_blob = load_binary_blob("debug_line_ps.cso");
			hr = device->CreateVertexShader(vs_debug_blob.data(), vs_debug_blob.size(), NULL, &vertex_shader[VERTEX_SHADER::COLORED_VERTEX]);

			assert(!FAILED(hr));

			hr = device->CreatePixelShader(ps_debug_blob.data(), ps_debug_blob.size(), NULL, &pixel_shader[PIXEL_SHADER::COLORED_VERTEX]);

			assert(!FAILED(hr));

			const D3D11_INPUT_ELEMENT_DESC debug_inputDesc[] =
			{
				{"Position",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
				{"Color",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0}
			};
			hr = device->CreateInputLayout(debug_inputDesc, 2, vs_debug_blob.data(), vs_debug_blob.capacity(), &input_layout[INPUT_LAYOUT::COLORED_VERTEX]);

			assert(!FAILED(hr));

			context->IASetInputLayout(input_layout[INPUT_LAYOUT::COLORED_VERTEX]);
			///


		}

		void create_constant_buffers()
		{
			D3D11_BUFFER_DESC mvp_bd;
			ZeroMemory(&mvp_bd, sizeof(mvp_bd));

			mvp_bd.Usage = D3D11_USAGE_DEFAULT;
			mvp_bd.ByteWidth = sizeof(MVP_t);
			mvp_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			mvp_bd.CPUAccessFlags = 0;

			HRESULT hr = device->CreateBuffer(&mvp_bd, NULL, &constant_buffer[CONSTANT_BUFFER::MVP]);

			assert(!FAILED(hr));

			// Create Vertex Buffer for Debug Lines
			D3D11_BUFFER_DESC vbDes;
			vbDes.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vbDes.Usage = D3D11_USAGE_DEFAULT;
			vbDes.CPUAccessFlags = 0u;
			vbDes.MiscFlags = 0u;
			vbDes.ByteWidth = sizeof(colored_vertex) * (UINT)end::debug_renderer::get_line_vert_capacity();
			vbDes.StructureByteStride = sizeof(colored_vertex);
			D3D11_SUBRESOURCE_DATA subData = {};
			subData.pSysMem = end::debug_renderer::get_line_verts();

			hr = device->CreateBuffer(&vbDes, &subData, &vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX]);

			assert(!FAILED(hr));


		}
	};

};