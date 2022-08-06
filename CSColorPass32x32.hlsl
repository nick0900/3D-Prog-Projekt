
cbuffer CameraViewport : register(b3)
{
    float width;
    float height;
    
    uint topLeftX;
    uint topLeftY;
};

//Implement viewport start position
//dispatch call need to recieve camera start xy and perform chacks and potential modifications to make sure CS works within backbuffer bounds

Texture2D ambient_color : register(t2);

Texture2D diffuse_color : register(t3);

Texture2D specular_color : register(t4);

RWTexture2D<unorm float4> backBuffer : register(u0);

[numthreads(32, 32, 1)]

void main(uint3 pixelCoords : SV_DispatchThreadID)
{    
    float4 pixelColor = float4(ambient_color.Load(pixelCoords).w, diffuse_color.Load(pixelCoords).w, specular_color.Load(pixelCoords).w, 1.0f);
    
    backBuffer[uint2(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y)] += pixelColor;
}