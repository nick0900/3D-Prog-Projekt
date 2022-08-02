struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float3 normal : normal;
    float2 uv : uv;
    float cameraObjectDistance : tesselationDistance;
};

cbuffer CameraTransform : register(b0)
{
    float4x4 cameraTransform;
    float4x4 inverseCameraTransform;
};

cbuffer ObjectTransform : register(b2)
{
    float4x4 objectWorldTransform;
    float4x4 inverseObjectWorldTransform;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    float4 objectOrigin = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    output.position = mul(float4(input.position, 1.0f), objectWorldTransform);
    objectOrigin = mul(objectOrigin, objectWorldTransform);

    objectOrigin = mul(objectOrigin, inverseCameraTransform);
    
    output.cameraObjectDistance = length(objectOrigin.xyz);
    
    output.normal = mul(float4(input.normal, 0.0f), transpose(inverseObjectWorldTransform));
    output.normal = normalize(output.normal);
    output.uv = input.uv;

    return output;
}