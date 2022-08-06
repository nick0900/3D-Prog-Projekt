struct Particle
{
    float3 position;
    float padding1;

    float4x4 rotation;

    float3 color;
    float padding2;
};

ConsumeStructuredBuffer<Particle> currentSimulationState : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    currentSimulationState.Consume();
}