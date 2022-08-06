struct Particle
{
    float3 position;
    float padding1;

    float4x4 rotation;

    float3 color;
    float padding2;
};

cbuffer particleCountBuffer : register(b0)
{
    uint particleCount;
    float padding[3];
}

ConsumeStructuredBuffer<Particle> currentSimulationState : register(u0);
AppendStructuredBuffer<Particle> newSimulationState : register(u1);

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleID = DTid.x + DTid.y * 32 + DTid.z * 32 * 32;
    if (particleID < particleCount)
    {
        Particle particle = currentSimulationState.Consume();
        
        particle.position = mul(float4(particle.position, 0.0f), particle.rotation);
        
        newSimulationState.Append(particle);
    }
}