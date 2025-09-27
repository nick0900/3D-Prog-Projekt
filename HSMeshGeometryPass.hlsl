struct VERTEX_IN
{
    float4 position : SV_Position;
    float3 normal : normal;
    float2 uv : uv;
    float cameraObjectDistance : tesselationDistance;
};

struct VERTEX_OUT
{
    float4 position : SV_Position;
    float3 normal : normal;
    float2 uv : uv;
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

HS_CONSTANT_DATA_OUTPUT ConstantHS(InputPatch<VERTEX_IN, 3> ip, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;
    
    float tesselationFactor;
    if (((maxDistance - minDistance) <= 0.0f)  || (maxTesselation < 1.0f))
    {
        tesselationFactor = maxTesselation;
    }
    else
    {
        tesselationFactor = clamp(ip[0].cameraObjectDistance, minDistance, maxDistance);
        tesselationFactor = 1.0f - (tesselationFactor - minDistance) / (maxDistance - minDistance);
        tesselationFactor = 1.0f + tesselationFactor * (maxTesselation - 1.0f);
	
    }
    
    Output.EdgeTessFactor[0] = tesselationFactor;
    Output.EdgeTessFactor[1] = tesselationFactor;
    Output.EdgeTessFactor[2] = tesselationFactor;
	Output.InsideTessFactor = tesselationFactor;

	return Output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
VERTEX_OUT main( InputPatch<VERTEX_IN, 3> ip, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID )
{
    VERTEX_OUT output;
    
    output.position = ip[i].position;
    output.normal = ip[i].normal;
    output.uv = ip[i].uv;
    
	return output;
}
