#include "debug_renderer.h"
#include <array>

// Anonymous namespace
namespace
{
	// Declarations in an anonymous namespace are global BUT only have internal linkage.
	// In other words, these variables are global but are only visible in this source file.

	// Maximum number of debug lines at one time (i.e: Capacity)
	constexpr size_t MAX_LINE_VERTS = 4096; 

	// CPU-side buffer of debug-line verts
	// Copied to the GPU and reset every frame.
	size_t line_vert_count = 0;
	std::array< end::colored_vertex, MAX_LINE_VERTS> line_verts;
}

namespace end
{
	namespace debug_renderer
	{
		void add_line(float4 point_a, float4 point_b, float4 color_a, float4 color_b)
		{
			// Add points to debug_verts, increments debug_vert_count
			if (line_vert_count < MAX_LINE_VERTS)
			{
				//line_verts[line_vert_count] = { (point_a, point_b), (color_a,color_b) };
				line_verts[line_vert_count].pos = point_a;
				line_verts[line_vert_count].color = color_a;
				line_vert_count++;

				line_verts[line_vert_count].pos = point_b;
				line_verts[line_vert_count].color = color_b;
				line_vert_count++;
			}
		}

		void clear_lines()
		{
			line_vert_count = 0;
		}

		const colored_vertex* get_line_verts()
		{ 
			return line_verts.data();
		}

		size_t get_line_vert_count() 
		{ 
			return line_vert_count;
		}

		size_t get_line_vert_capacity()
		{
			return MAX_LINE_VERTS;
		}
	}
}