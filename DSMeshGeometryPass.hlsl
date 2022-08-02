struct VERTEX
{
    float4 position : SV_Position;
    float3 normal : normal;
    float2 uv : uv;
};

struct OUTPUT_VERTEX
{
    float4 position : SV_Position;
    float3 normal : normal;
    float2 uv : uv;
    float distance : dist;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

cbuffer TesselationConfig : register(b0)
{
    float maxTesselation;
    float maxDistance;
    float minDistance;
    float interpolationFactor;
};

cbuffer CameraTransform : register(b1)
{
    float4x4 cameraTransform;
    float4x4 inverseCameraTransform;
};

cbuffer CameraProjection : register(b2)
{
    float4x4 projectionMatrix;
	
    float widthScalar;
    float heightScalar;

    float projectionConstantA;
    float projectionVonstantB;
};

[domain("tri")]
OUTPUT_VERTEX main(HS_CONSTANT_DATA_OUTPUT input, float3 barycentric : SV_DomainLocation, const OutputPatch<VERTEX, 3> patch)
{
    float3 vertPosition = mul(barycentric, float3x3(patch[0].position.xyz, patch[1].position.xyz, patch[2].position.xyz));
    
    float3 projected0 = vertPosition - mul(patch[0].normal, dot((vertPosition - patch[0].position.xyz), patch[0].normal));
    float3 projected1 = vertPosition - mul(patch[1].normal, dot((vertPosition - patch[1].position.xyz), patch[1].normal));
    float3 projected2 = vertPosition - mul(patch[2].normal, dot((vertPosition - patch[2].position.xyz), patch[2].normal));
    
    float3 interpolatedPosition = mul(barycentric, float3x3(projected0, projected1, projected2));
    
    interpolatedPosition = (1 - interpolationFactor) * vertPosition + interpolationFactor * interpolatedPosition;
    
    OUTPUT_VERTEX output;
    
    output.position = mul(float4(interpolatedPosition, 1.0f), inverseCameraTransform);
    output.position = mul(output.position, projectionMatrix);
    
    output.distance = length(interpolatedPosition);
    
    output.normal = mul(barycentric, float3x3(patch[0].normal, patch[1].normal, patch[2].normal));
    
    output.uv = mul(barycentric, float3x2(patch[0].uv, patch[1].uv, patch[2].uv));

	return output;
}
