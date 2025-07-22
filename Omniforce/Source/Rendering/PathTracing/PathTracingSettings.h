#pragma once

#include <Foundation/Common.h>

namespace Omni {

    struct META(ShaderExpose, Module = "RenderingGenerated") PathTracingSettings {
        uint32 MaxBounces;
        uint32 SamplesPerPixel;
        bool DeterministicSeed;
        uint32 FixedSeed;
        uint32 MaxAccumulatedFrameCount;
        bool RayDebugMode;
        float RussianRouletteThreshold;
        bool EnableMIS;
        bool EnableNEE;
        float RayEpsilon;
        bool VisualizePaths;
        bool VisualizeHitPoints;
        bool VisualizeMaterials;
        bool AdaptiveSampling;
        bool EnableDenoising;
        float TemporalFilterStrength;
        bool EnableEnvironmentLighting;
        bool GatherPerformanceMetrics;
    };
}