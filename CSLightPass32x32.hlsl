cbuffer ShaderConfig : register(b0)
{
    int lightType;
    int shadowMapType;
    float shadowBias;
    bool shadowCaster;
 
    bool padding[3];
}

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
    float projectionConstantB;
};

cbuffer CameraViewport : register(b3)
{
    float width;
    float height;
    
    uint topLeftX;
    uint topLeftY;
};

cbuffer AmbientLight : register(b4)
{
    float3 ambientAlbedo;
    float ambientPadding;
};

cbuffer PointLight : register(b5)
{
    float3 pointDiffuse;
    float pointFalloff;
 
    float3 pointSpecular;
    float pointPadding1;
    
    float3 pointPosition;
    float pointPadding2;
};

cbuffer DirectionalLight : register(b6)
{
    float3 dirDiffuse;
    float dirPadding1;
    
    float3 dirSpecular;
    float dirpadding2;
    
    float3 dirDirection;
    float dirpadding3;
};

cbuffer SpotLight : register(b7)
{
    float3 spotDiffuse;
    float spotFalloff;
    
    float3 spotSpecular;
    float spotLightFOV;
    
    float3 spotPosition;
    float spotPadding1;
    
    float3 spotDirection;
    float spotPadding2;
};

cbuffer shadowmappingMatrices : register(b8)
{
    float4x4 shadowView;
    float4x4 shadowProjection;
};

//Implement viewport start position
//dispatch call need to recieve camera start xy and perform chacks and potential modifications to make sure CS works within backbuffer bounds

Texture2D machineDepth : register(t0);

Texture2D normal_shinyness : register(t1);

Texture2D ambient_color : register(t2);

Texture2D diffuse_color : register(t3);

Texture2D specular_color : register(t4);

Texture2D singleShadowmap : register(t5);

TextureCube<float> shadowCubemap : register(t6);

RWTexture2D<unorm float4> backBuffer : register(u0);

SamplerState wrapSampler : register(s0);
SamplerState borderSampler : register(s1);

[numthreads(32, 32, 1)]

void main( uint3 pixelCoords : SV_DispatchThreadID )
{
    if (lightType == -1)
    {
        return;
    }
    
    float3 diffuseAlbedo = diffuse_color.Load(pixelCoords).rgb;
    
    if (lightType == 0) //AmbientLight
    {
        backBuffer[uint2(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y)] += float4(diffuseAlbedo * ambientAlbedo * ambient_color.Load(pixelCoords).rgb, 1.0f);
        return;
    }
    
    float zDepth = projectionConstantB / (machineDepth.Load(pixelCoords) - projectionConstantA);
    float xPosition = (pixelCoords.x / width - 1.0f / 2.0f) * 2 * widthScalar * zDepth;
    float yPosition = (1.0f / 2.0f - pixelCoords.y / height) * 2 * heightScalar * zDepth;
    
    float4 pixelPosition = float4(xPosition, yPosition, zDepth, 1.0f);
    pixelPosition = mul(pixelPosition, cameraTransform);
    
    float4 cameraPosition = float4(0.0f, 0.0f, 0.0f, 1.0f);
    cameraPosition = mul(cameraPosition, cameraTransform);
    
    float3 cameraPath = pixelPosition - cameraPosition;
    
    float3 diffuseColor = float3(0.0f, 0.0f, 0.0f);
    float3 specularColor = float3(0.0f, 0.0f, 0.0f);
    
    float3 normal = normal_shinyness.Load(pixelCoords).xyz;
    float shinyness = normal_shinyness.Load(pixelCoords).w;
    
    float3 specularAlbedo = specular_color.Load(pixelCoords).rgb;
    
    float3 lightPath;
    float3 reflected;
    
    float shadowFactor = 1.0f;
    
    if (shadowCaster)
    {   
        if (shadowMapType == 0) //single
        {
            float4 lightSpacePosition = mul(pixelPosition, shadowView);
            lightSpacePosition = mul(lightSpacePosition, shadowProjection);
            lightSpacePosition /= lightSpacePosition.w;
            
            float2 sampleTex = float2(0.5f * lightSpacePosition.x + 0.5f, -0.5f * lightSpacePosition.y + 0.5f);
            
            shadowFactor = ((singleShadowmap.SampleLevel(borderSampler, sampleTex, 0.0f).r + shadowBias) < lightSpacePosition.z) ? 0.0f : 1.0f;
        }
        else if (shadowMapType == 1) //cube
        {
            lightPath = pixelPosition.xyz - pointPosition;
            
            shadowFactor = ((shadowCubemap.SampleLevel(wrapSampler, normalize(lightPath), 0.0f) + shadowBias) < length(lightPath)) ? 0.0f : 1.0f;
        }
    }
    switch (lightType)
    {
        case 1://pointlight
            lightPath = pointPosition - pixelPosition.xyz;
            lightPath = normalize(lightPath);
            diffuseColor = (pointFalloff - length(lightPath)) / pointFalloff * diffuseAlbedo * pointDiffuse * max(0.0f, dot(normal, lightPath));
            
            reflected = normalize(reflect(lightPath, normal));
            cameraPath = normalize(cameraPath);
            specularColor = specularAlbedo * pointSpecular * pow(max(0.0f, dot(reflected, cameraPath)), shinyness);
            break;
        
        case 2://directionallight
            diffuseColor = diffuseAlbedo * dirDiffuse * max(0.0f, dot(normal, dirDirection));
            
            reflected = normalize(reflect(dirDirection, normal));
            cameraPath = normalize(cameraPath);
            specularColor = specularAlbedo * dirSpecular * pow(max(0.0f, dot(reflected, cameraPath)), shinyness);
            break;
        
        case 3://spotlight
            lightPath = spotPosition - pixelPosition.xyz;
            lightPath = normalize(lightPath);
            
            float spotFactor = (dot(lightPath, spotDirection) < spotLightFOV) ? 0.0f : 1.0f;

            diffuseColor = spotFactor * (spotFalloff - length(lightPath)) / spotFalloff * diffuseAlbedo * spotDiffuse * max(0.0f, dot(normal, lightPath));
            
            reflected = normalize(reflect(lightPath, normal));
            cameraPath = normalize(cameraPath);
            specularColor = spotFactor * specularAlbedo * spotSpecular * pow(max(0.0f, dot(reflected, cameraPath)), shinyness);
            break;
    }
    
    float4 pixelColor = float4((diffuseColor + specularColor) * shadowFactor, 1.0f);
    
    backBuffer[uint2(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y)] += pixelColor;
}