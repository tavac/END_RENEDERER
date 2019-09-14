#include "mvp.hlsli"

struct VS_Input
{
    float4 pos : Position;
    float4 color : Color;
};

struct VS_Output
{
    float4 pos : SV_POSITION;
    float4 normal : NORMAL;
    float4 color : COLOR;
};

VS_Output main(VS_Input vsIn)
{
    VS_Output vsOut = (VS_Output) 0;
    vsOut.pos = mul(vsIn.pos, modeling);
    vsOut.pos = mul(vsOut.pos, view);
    vsOut.pos = mul(vsOut.pos, projection);
    vsOut.color = vsIn.color;
    //uint vert_index = cube_i[input.vertexId];

    //float4 vert_pos = cube_v[vert_index];
    //float4 vert_norm = cube_n[input.vertexId / 6];

    //output.pos = mul(vert_pos, modeling);
    //output.pos = mul(output.pos, view);
    //output.pos = mul(output.pos, projection);

    //output.color = float4((vert_norm.xyz + 1.0f) * 0.5f, 1.0f);

    //output.normal = mul(vert_norm, modeling);

    return vsOut;
}