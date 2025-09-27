cbuffer ShaderConfig : register(b0)
{
    int lightType;
    uint lightCount;
    int shadowMapType;
    float shadowBias;
    bool shadowCaster;
 
    bool padding[15];
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

Texture2D machineDepth : register(t0);

Texture2D normal_shinyness : register(t1);

Texture2D ambient_color : register(t2);

Texture2D diffuse_color : register(t3);

Texture2D specular_color : register(t4);

Texture2D singleShadowmap : register(t5);

TextureCube<float> shadowCubemap : register(t6);

struct LightArrayParameterStruct
{
    int lightType;
    float3 diffuse;

    int castShadow;
    float3 specular;

    float3 position;
    float falloff;

    float3 direction;
    float lightFOV;
};

StructuredBuffer<LightArrayParameterStruct> LightArrayParameters : register(t7);

struct LightArrayShadowmappingStruct
{
    float4x4 shadowView;
    float4x4 shadowProjection;
};

StructuredBuffer<LightArrayShadowmappingStruct> LightArrayShadowmapMatrices : register(t8);

Texture2DArray LightArrayShadowmaps : register(t9);

RWTexture2DArray<unorm float4> backBuffer : register(u0);

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
        backBuffer[uint3(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y, 0)] += float4(diffuseAlbedo * ambientAlbedo * ambient_color.Load(pixelCoords).rgb, 1.0f);
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
    
    if (lightType == 4) //spot/directional light array
    {
        for (int i = 0; i < lightCount; i++)
        {
            LightArrayParameterStruct parameters = LightArrayParameters[i];
            if (parameters.castShadow == 1)
            {
                LightArrayShadowmappingStruct shadowMatrices = LightArrayShadowmapMatrices[i];
                float4 lightSpacePosition = mul(pixelPosition, shadowMatrices.shadowView);
                lightSpacePosition = mul(lightSpacePosition, shadowMatrices.shadowProjection);
                lightSpacePosition /= lightSpacePosition.w;
            
                float3 sampleTex = float3(0.5f * lightSpacePosition.x + 0.5f, -0.5f * lightSpacePosition.y + 0.5f, i);
            
                shadowFactor = ((LightArrayShadowmaps.SampleLevel(borderSampler, sampleTex, 0.0f).r + shadowBias) < lightSpacePosition.z) ? 0.0f : 1.0f;
            }
            
            lightPath = parameters.lightType == 0 ? parameters.position - pixelPosition.xyz : parameters.direction;
            float fallofFactor = max(spotFalloff != 0.0f ? (spotFalloff - length(lightPath)) / spotFalloff : 1.0f, 0.0f);
            lightPath = normalize(lightPath);
            
            float spotFactor = (parameters.lightType == 0) && (dot(lightPath, parameters.direction) < parameters.lightFOV) ? 0.0f : 1.0f;

            diffuseColor = spotFactor * fallofFactor * diffuseAlbedo * parameters.diffuse * max(0.0f, dot(normal, lightPath));
            
            reflected = normalize(reflect(lightPath, normal));
            cameraPath = normalize(cameraPath);
            specularColor = spotFactor * specularAlbedo * parameters.specular * pow(max(0.0f, dot(reflected, cameraPath)), shinyness);
    
            float4 pixelColor = float4((diffuseColor + specularColor) * shadowFactor, 1.0f);
    
            backBuffer[uint3(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y, 0)] += pixelColor;
        }
        return;
    }
    
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
            float pointFallofFactor = max(pointFalloff != 0.0f ? (pointFalloff - length(lightPath)) / pointFalloff : 1.0f, 0.0f);
            
            lightPath = normalize(lightPath);
            
            diffuseColor = pointFallofFactor * diffuseAlbedo * pointDiffuse * max(0.0f, dot(normal, lightPath));
            
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
            float spotFallofFactor = max(spotFalloff != 0.0f ? (spotFalloff - length(lightPath)) / spotFalloff : 1.0f, 0.0f);
            lightPath = normalize(lightPath);
            
            float spotFactor = (dot(lightPath, spotDirection) < spotLightFOV) ? 0.0f : 1.0f;

            diffuseColor = spotFactor * spotFallofFactor * diffuseAlbedo * spotDiffuse * max(0.0f, dot(normal, lightPath));
            
            reflected = normalize(reflect(lightPath, normal));
            cameraPath = normalize(cameraPath);
            specularColor = spotFactor * specularAlbedo * spotSpecular * pow(max(0.0f, dot(reflected, cameraPath)), shinyness);
            break;
    }
    
    float4 pixelColor = float4((diffuseColor + specularColor) * shadowFactor, 1.0f);
    
    backBuffer[uint3(topLeftX + pixelCoords.x, topLeftY + pixelCoords.y, 0)] += pixelColor;
}