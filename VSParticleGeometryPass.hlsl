struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 normal : normal;
    float2 uv : uv;
};

cbuffer CameraTransform : register(b0)
{
    float4x4 cameraTransform;
    float4x4 inverseCameraTransform;
};

cbuffer CameraProjection : register(b1)
{
    float4x4 projectionMatrix;
	
    float widthScalar;
    float heightScalar;

    float projectionConstantA;
    float projectionVonstantB;
};

cbuffer ObjectTransform : register(b2)
{
    float4x4 objectWorldTransform;
    float4x4 inverseObjectWorldTransform;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(float4(input.position, 1.0f), objectWorldTransform);

    output.position = mul(output.position, inverseCameraTransform);
    output.position = mul(output.position, projectionMatrix);
    output.normal = mul(float4(input.normal, 0.0f), transpose(inverseObjectWorldTransform)).xyz;
    output.uv = input.uv;

    return output;
}