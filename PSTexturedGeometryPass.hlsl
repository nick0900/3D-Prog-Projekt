struct PixelShaderInput
{
	float4 position : SV_POSITION;
	float3 normal : normal;
	float2 uv : uv;
};

struct PixelShaderOutput
{
	float4 normal3_shinyness1 : SV_Target0;
	float4 ambient3_color1 : SV_Target1;
	float4 diffuse3_color1 : SV_Target2;
	float4 specular3_color1 : SV_Target3;
};
	
cbuffer MaterialParameters : register(b0)
{
	float shininessConstant;
	float3 padding;
};

Texture2D ambientKoeff : register(t0);

Texture2D diffuseKoeff : register(t1);

Texture2D specularKoeff : register(t2);

SamplerState basicSampler : register(s0);

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	
	output.normal3_shinyness1 = float4(input.normal, shininessConstant);
	
	output.ambient3_color1 = ambientKoeff.Sample(basicSampler, input.uv);
	
	output.diffuse3_color1 = diffuseKoeff.Sample(basicSampler, input.uv);
	
	output.specular3_color1 = specularKoeff.Sample(basicSampler, input.uv);
	
	float3 color = diffuseKoeff.Sample(basicSampler, input.uv).rgb;
	
	output.ambient3_color1.w = color.r;
	output.diffuse3_color1.w = color.g;
	output.specular3_color1.w = color.b;
	
	return output;
}
