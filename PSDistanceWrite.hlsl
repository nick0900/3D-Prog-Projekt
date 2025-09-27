struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float3 normal : normal;
    float2 uv : uv;
    float distance : dist;
};

struct PixelShaderOutput
{
    float distance : SV_Target0;
};

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
	
    output.distance = input.distance;
	
    return output;
}
