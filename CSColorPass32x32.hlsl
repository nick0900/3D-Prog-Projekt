
cbuffer CameraViewport : register(b3)
{
    float width;
    float height;
    
    uint topLeftX;
    uint topLeftY;
};

Texture2D ambient_color : register(t2);

Texture2D diffuse_color : register(t3);

Texture2D specular_color : register(t4);

RWTexture2DArray<unorm float4> backBuffer : register(u0);

[numthreads(32, 32, 1)]

void main(uint3 pixelCoords : SV_DispatchThreadID)
{    
    float4 pixelColor = float4(ambient_color.Load(pixelCoords).w, diffuse_color.Load(pixelCoords).w, specular_color.Load(pixelCoords).w, 1.0f);
    
    backBuffer[uint3(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y, 0)] += pixelColor;
}