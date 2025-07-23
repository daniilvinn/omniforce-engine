#pragma once

#ifndef STRIP_GLOBAL_PARAMETERS_MODULE_DECL
module GlobalParameters;
#endif

namespace Omni {

    [[vk::binding(0, 0)]]
    public Sampler2D TextureHeap[65536];

    [[vk::binding(1, 0)]]
    public RWTexture2D<uint64_t> SceneVisibilityBuffer;

    [[vk::binding(2, 0)]]
    public RaytracingAccelerationStructure SceneTLAS;

    [[vk::binding(3, 0)]]
    public RWTexture2D<float4> OutputImage;

}