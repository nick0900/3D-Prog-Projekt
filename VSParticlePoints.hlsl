struct Particle
{
    float3 position;
    float padding1;

    float4x4 rotation;

    float3 color;
    float padding2;
};

StructuredBuffer<Particle> SimulationState : register (t0);

struct VertexShaderOutput
{
    float3 position : position;
    float3 color : color;
};



VertexShaderOutput main( uint vertexID : SV_VertexID )
{
    VertexShaderOutput output;
    
    output.position = SimulationState[vertexID].position;
    output.color = SimulationState[vertexID].color;
    
	return output;
}