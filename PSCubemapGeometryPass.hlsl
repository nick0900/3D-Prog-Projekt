struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float3 normal : normal;
    float3 sampleVector : sampleDir;
};

struct PixelShaderOutput
{
    float4 normal3_shinyness1 : SV_Target0;
    float4 ambient3_color1 : SV_Target1;
    float4 diffuse3_color1 : SV_Target2;
    float4 specular3_color1 : SV_Target3;
};

TextureCube<float4> Cubemap : register(t0);

SamplerState wrapSampler : register(s0);

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
	
    output.normal3_shinyness1 = float4(input.normal, 10.0f);
    
    float3 color = Cubemap.SampleLevel(wrapSampler, normalize(input.sampleVector), 0.0f).rgb;
    
    output.ambient3_color1 = float4(0.0f, 0.0f, 0.0f, color.r);
	
    output.diffuse3_color1 = float4(0.0f, 0.0f, 0.0f, color.g);
	
    output.specular3_color1 = float4(1.0f, 1.0f, 1.0f, color.b);
	
    return output;
}
