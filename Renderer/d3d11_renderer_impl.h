#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dxgi1_2.h>
#include <d3d11_2.h>
#include <DirectXMath.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DXGI.lib")

#include "renderer.h"
#include "view.h"
#include "blob.h"
#include "../Renderer/shaders/mvp.hlsli"

// NOTE: This header file must *ONLY* be included by renderer.cpp

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

		///////////// PARTICLES /////////////////////
		Emitter emitters[NUM_OF_EMITTERS];
		pool_t<Particle, numOfParticles> particles;
		/////////////////////////////////////////////
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

			/////////////////// PARTICLE CREATION ///////////////////
			create_emitters(W_ORIGIN, WHITE);
			create_particles(emitters, NUM_OF_EMITTERS, W_UP, WHITE);
			/////////////////////////////////////////////////////////
			timer.Restart();
		}

		float fake_timer = 0.0f;
		bool goingDown = false;
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


			// Particles //
			update_particles(&emitters[0], deltaT, { 1.0f,0.0f,0.0f,1.0f });
			update_particles(&emitters[1], deltaT, { 0.0f,1.0f,0.0f,1.0f });
			update_particles(&emitters[2], deltaT, { 0.0f,0.0f,1.0f,1.0f });
			draw_debug_lines(view);
			create_particles(emitters, NUM_OF_EMITTERS, { 0.0f,1.0f,0.0f }, WHITE);
			//////////////

			swapchain->Present(1u, 0u);
			end::debug_renderer::clear_lines();
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

		void create_emitters(end::float3 pos, end::float4 color)
		{
			for (int i = 0; i < NUM_OF_EMITTERS; i++)
			{
				Emitter emitter;
				emitter.origin = pos;
				emitter.init_color = color;
				emitters[i] = emitter;
			}
		}

		void create_particles(Emitter* em, int numOfEm, end::float3 pos, end::float4 color)
		{
			
			for (int i = 0; i < numOfEm; i++)
			{
				for (int lastUsedPtcle = 0; lastUsedPtcle < (numOfParticles / numOfEm); lastUsedPtcle++)
				{
					Particle p;
					p.prev_pos = pos;
					p.pos = pos;
					p.color = color;
					// Check Particle pool for free spot
					int p_index = particles.alloc();
					if (p_index < 0)
						return;
					else
						particles[p_index] = p;

					// Check Emitter for free spot
					int e_index = em[i].parti_indices.alloc();
					if (e_index < 0)
					{
						particles.free(p_index); // Emitter had no empty spot for this particle so put it back.
						return;
					}
					else
						em[i].parti_indices[e_index] = p_index;
				}
			}
		}

		void update_particles(Emitter* em, float dT, end::float4 nColor)
		{
			// Timer for dispersing particles at different rates
			XTime sepTimer;
			int size = em->parti_indices.size();
			for (int i = 0; i < size; i++)
			{
				float s_dT = sepTimer.Delta();

				if (particles[em->parti_indices[i]].life > 2.0f)
				{
					kill_particle(em, i);
					i--;
					return;
				}


				end::float3 dir;
				for (int n = 0; n < 0; n++) // ZEROED to skip over
				{
					int rng = rand();
					if (rng % 2 == 0)
						dir.x = cosf(rand()) * 1.0f;
					else
						dir.x = cosf(rand()) * -1.0f;

					rng = rand();
					if (rng % 2 == 0)
						dir.y = sinf(rand()) * 0.10f;
					else
						dir.y = sinf(rand()) * -0.10f;

					rng = rand();
					if (rng % 2 == 0)
						dir.z = cosf(rand()) * 1.0f;
					else
						dir.z = cosf(rand()) * -1.0f;
				}
				// fountain math
				dir.x = (cosf(i * i) * sinf(-i));// cosf(dT * (3.14156 / 180)) * 0.1f;
				dir.y = tanf(i) * sinf(dir.x * dir.x);
				dir.z = (sinf(i * i) * sinf(-i));// -cosf(dT * (3.14156 / 180)) * 0.1f;
				float scalar =  -.0986f;
				//dir.x = dT;
				//dir.y = dir.x * scalar;
				Particle nP = particles[em->parti_indices[i]];
				nP.prev_pos = nP.pos;
				nP.pos = nP.pos + (dir * scalar);
				nP.color = (nColor);
				nP.color.w = 1.0f;
				nP.life += dT;
				particles[em->parti_indices[i]] = nP;
				end::float4 p = { particles[em->parti_indices[i]].prev_pos.x, particles[em->parti_indices[i]].prev_pos.y, particles[em->parti_indices[i]].prev_pos.z, 1.0f };
				end::float4 q = { particles[em->parti_indices[i]].pos.x, particles[em->parti_indices[i]].pos.y, particles[em->parti_indices[i]].pos.z, 1.0f };
				debug_renderer::add_line(p, q, particles[em->parti_indices[i]].color);
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

		~impl_t()
		{
			// TODO:
			//Clean-up
			clear_particles(&emitters[0]);
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