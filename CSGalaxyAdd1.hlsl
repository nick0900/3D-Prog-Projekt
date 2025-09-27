struct Particle
{
    float3 position;
    float padding1;

    float4x4 rotation;

    float3 color;
    float padding2;
};

cbuffer newParticle : register(b0)
{
    float3 position;
    float padding1;

    float4x4 rotation;

    float3 color;
    float padding2;
}

AppendStructuredBuffer<Particle> currentSimulationState : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    Particle particle;
    
    particle.position = position;
    particle.rotation = rotation;
    particle.color = color;
    particle.padding1 = 0.0f;
    particle.padding2 = 0.0f;
    
    currentSimulationState.Append(particle);
}