struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 uv : uv;
    float3 color : color;
};

struct PixelShaderOutput
{
    float4 normal3_shinyness1 : SV_Target0;
    float4 ambient3_color1 : SV_Target1;
    float4 diffuse3_color1 : SV_Target2;
    float4 specular3_color1 : SV_Target3;
};

Texture2D particleAlpha : register(t0);

SamplerState basicSampler : register(s0);

PixelShaderOutput main(PixelShaderInput input)
{
    float3 color = input.color * particleAlpha.Sample(basicSampler, input.uv).r;
    
    PixelShaderOutput output;
	
    output.normal3_shinyness1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
    output.ambient3_color1 = float4(0.0f, 0.0f, 0.0f, color.r);
	
    output.diffuse3_color1 = float4(0.0f, 0.0f, 0.0f, color.g);
	
    output.specular3_color1 = float4(0.0f, 0.0f, 0.0f, color.b);
	
    return output;
}
