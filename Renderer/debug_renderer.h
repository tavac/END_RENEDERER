#pragma once

#include "math_types.h"

// Interface to the debug renderer
namespace end
{
	namespace debug_renderer
	{
		using namespace DirectX;
		void add_line(float3 point_a, float3 point_b, float4 color_a, float4 color_b);
		void add_line(float4 point_a, float4 point_b, float4 color_a, float4 color_b);
		void add_line(XMVECTOR point_a, XMVECTOR point_b, XMVECTOR color_a, XMVECTOR color_b);

		inline void add_line(float3 p, float3 q, float4 color) { add_line(p, q, color, color); }
		inline void add_line(float4 p, float4 q, float4 color) { add_line(p, q, color, color); }
		inline void add_line(XMVECTOR p, XMVECTOR q, XMVECTOR color) { add_line(p, q, color, color); }

		void clear_lines();

		const colored_vertex* get_line_verts();

		size_t get_line_vert_count();

		size_t get_line_vert_capacity();
	}
}