#pragma once
#include "math_types.h"
#include "pools.h"

#define NUM_OF_EMITTERS 3
#define numOfParticles 100
#define W_ORIGIN	{ 0.0f,0.0f,0.0f }
#define W_UP		{0.0f, 1.0f, 0.0f}
#define W_FRONT		{0.0f,0.0f,1.0f}
#define WHITE		{ 1.0f,1.0f,1.0f,1.0f }
#define RED			{ 1.0f,0.0f,0.0f,1.0f }
#define GREEN		{ 0.0f,1.0f,0.0f,1.0f }
#define BLUE		{ 0.0f,0.0f,1.0f,1.0f }

struct Particle
{
	end::float3 pos;
	end::float3 prev_pos;
	end::float4 color;
	float life = 0.0f;
};

struct Emitter
{
	end::float3 origin;
	end::float4 init_color;

	// Constructor // Destructor
	Emitter() {};
	Emitter(end::float3 _o, end::float4 _c) : origin(_o), init_color(_c) {};
	~Emitter() {};

	// Pool of particles to be emitted
	end::sorted_pool_t<int16_t, numOfParticles> parti_indices;
};