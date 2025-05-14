#pragma once

module VisibleMaterialResolve;

using namespace Omni;

import Common;
import Rendering;
import Packing;
import FullScreenBase;
import Gen.ResolveVisibleMaterialsMaskInput;

public float ComputeMaterialResolveFragmentDepth(uint seed) {
    uint divisor = ((seed ^ ((seed & 0xFF000000) >> 8)) & 0x00FFFFFF);
    return 1.0f / divisor;
};

[shader("fragment")]
float ResolveMaterialMask(VSOutput StageInput, float4 PixelPosition : SV_POSITION) : SV_DEPTH {

    uint2 Payload = Unpack32(SceneVisibilityBuffer.Load(int2(PixelPosition.xy)));

    if (Payload.g == 0u) {
        return NaN;
    }

    SceneVisibleCluster VisibleCluster = Input.VisibleClusters[Payload.r >> 7];
    InstanceRenderData InstanceData = Input.Instances[VisibleCluster.InstanceIndex];

    uint* MaterialShaderID = (uint*)InstanceData.material_address;

    return ComputeMaterialResolveFragmentDepth(*MaterialShaderID);
}