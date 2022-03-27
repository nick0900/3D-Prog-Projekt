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
	float3 ambientKoeff;
	float padding1;
	
	float3 diffuseKoeff;
	float padding2;
	
	float3 specularKoeff;
	float shininessConstant;
};

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	
	output.normal3_shinyness1 = float4(input.normal, shininessConstant);
	
	output.ambient3_color1 = float4(ambientKoeff, diffuseKoeff.r);
	
	output.diffuse3_color1 = float4(diffuseKoeff, diffuseKoeff.g);
	
	output.specular3_color1 = float4(specularKoeff, diffuseKoeff.b);
	
	return output;
}