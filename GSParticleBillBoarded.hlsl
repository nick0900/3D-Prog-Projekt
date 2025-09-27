struct GeometryShaderInput
{
    float3 position : position;
    float3 color : color;
};

struct GeometryShaderOutput
{
	float4 position : SV_POSITION;
    float2 uv : uv;
    float3 color : color;
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

[maxvertexcount(6)]
void main(point GeometryShaderInput input[1], inout TriangleStream<GeometryShaderOutput> output)
{
    const float SIZE = 0.1f;
    
    float4 quadMiddle = mul(float4(input[0].position, 1.0f), objectWorldTransform);
    quadMiddle = mul(quadMiddle, inverseCameraTransform);
    
    float4 topLeft = float4(quadMiddle.x - SIZE, quadMiddle.y + SIZE, quadMiddle.z, 1.0f);
    float4 topRight = float4(quadMiddle.x + SIZE, quadMiddle.y + SIZE, quadMiddle.z, 1.0f);
    float4 bottomRight = float4(quadMiddle.x + SIZE, quadMiddle.y - SIZE, quadMiddle.z, 1.0f);
    float4 bottomLeft = float4(quadMiddle.x - SIZE, quadMiddle.y - SIZE, quadMiddle.z, 1.0f);

    GeometryShaderOutput outputTopLeft;
    outputTopLeft.color = input[0].color;
    outputTopLeft.position = mul(topLeft, projectionMatrix);
    outputTopLeft.uv = float2(0.0f, 0.0f);
    
    GeometryShaderOutput outputTopRight;
    outputTopRight.color = input[0].color;
    outputTopRight.position = mul(topRight, projectionMatrix);
    outputTopRight.uv = float2(1.0f, 0.0f);
    
    GeometryShaderOutput outputBottomRight;
    outputBottomRight.color = input[0].color;
    outputBottomRight.position = mul(bottomRight, projectionMatrix);
    outputBottomRight.uv = float2(1.0f, 1.0f);
    
    GeometryShaderOutput outputBottomLeft;
    outputBottomLeft.color = input[0].color;
    outputBottomLeft.position = mul(bottomLeft, projectionMatrix);
    outputBottomLeft.uv = float2(0.0f, 1.0f);
    
    output.Append(outputTopLeft);
    output.Append(outputTopRight);
    output.Append(outputBottomRight);
    
    output.RestartStrip();
    
    output.Append(outputTopLeft);
    output.Append(outputBottomRight);
    output.Append(outputBottomLeft);
}