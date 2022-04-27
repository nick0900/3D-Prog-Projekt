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
    float projectionConstantB;
};

cbuffer CameraViewport : register(b2)
{
    float width;
    float height;
    
    uint topLeftX;
    uint topLeftY;
};

//Implement viewport start position
//dispatch call need to recieve camera start xy and perform chacks and potential modifications to make sure CS works within backbuffer bounds

Texture2D machineDepth : register(t0);

Texture2D normal_shinyness : register(t1);

Texture2D ambient_color : register(t2);

Texture2D diffuse_color : register(t3);

Texture2D specular_color : register(t4);

RWTexture2D<float4> backBuffer : register(u0);

[numthreads(32, 18, 1)]

void main( uint3 pixelCoords : SV_DispatchThreadID )
{
    float zDepth = projectionConstantB / (machineDepth.Load(pixelCoords) - projectionConstantA);
    float xcoord = pixelCoords.x;
    float ycoord = pixelCoords.y;
    float xPosition = (xcoord / width - 1.0f / 2.0f) * 2 * widthScalar * zDepth;
    float yPosition = (1.0f / 2.0f - ycoord / height) * 2 * heightScalar * zDepth;
    
    float3 pixelPosition = float3(xPosition, yPosition, zDepth);
    
    backBuffer[uint2(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y)] = float4(pixelPosition, 1.0f);
}