module Shader;

import CoreTypes;

internal RWStructuredBuffer<Omni.Sphere> buffer0;

[shader("compute")]
[numthreads(1,1,1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    buffer0[index] = Omni.Sphere();
}