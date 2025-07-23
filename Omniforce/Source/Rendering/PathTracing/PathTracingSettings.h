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

        constexpr PathTracingSettings() 
            : MaxBounces(3)
            , SamplesPerPixel(1)
            , DeterministicSeed(false)
            , FixedSeed(0)
            , MaxAccumulatedFrameCount(4096)
            , RayDebugMode(false)
            , RussianRouletteThreshold(0.8f)
            , EnableMIS(true)
            , EnableNEE(true)
            , RayEpsilon(1e-4f)
            , VisualizePaths(false)
            , VisualizeHitPoints(false)
            , VisualizeMaterials(false)
            , AdaptiveSampling(true)
            , EnableDenoising(true)
            , TemporalFilterStrength(0.1f)
            , EnableEnvironmentLighting(true)
            , GatherPerformanceMetrics(false)
        {}
    };
}