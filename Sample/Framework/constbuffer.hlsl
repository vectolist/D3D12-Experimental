cbuffer CBO  : register(b0)
{
    //hlsl init as column major
    float4x4 view;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput vs)
{
    PSInput ps;
    // p1 = V * P 
    ps.pos = mul(view,float4(vs.pos, 1));
    //ps.pos = float4(vs.pos, 1);
    //ps.color = vs.color;
    ps.color = vs.color;
    return ps;
}

float4 PSMain(PSInput ps) : sv_target
{
    return ps.color;
}

