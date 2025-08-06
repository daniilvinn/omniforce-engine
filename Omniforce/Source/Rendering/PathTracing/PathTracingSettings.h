#pragma once

#include <Foundation/Common.h>

namespace Omni {

    struct META(ShaderExpose, Module = "RenderingGenerated") PathTracingSettings {
        uint32 MaxBounces;
        uint32 SamplesPerPixel;
        bool DeterministicSeed;
        uint32 FixedSeed;
        glm::vec3 SkyColor;
        bool EnableSkyLight;
        float32 SkyLightIntensity;
        glm::vec3 SunDirection;
        uint32 StarGenerationSeed;
        uint32 StarDensity;
        uint32 MaxAccumulatedFrameCount;
        bool RayDebugMode;
        float32 RussianRouletteThreshold;
        bool EnableMIS;
        bool EnableNEE;
        float32 RayEpsilon;
        bool VisualizePaths;
        bool VisualizeHitPoints;
        bool VisualizeMaterials;
        bool AdaptiveSampling;
        bool EnableDenoising;
        float32 TemporalFilterStrength;
        bool GatherPerformanceMetrics;

        PathTracingSettings() 
            : MaxBounces(3)
            , SamplesPerPixel(1)
            , DeterministicSeed(false)
            , FixedSeed(0)
            , EnableSkyLight(true)
            , SkyColor(0.7, 0.9, 1.0)
            , SkyLightIntensity(5.0)
            , SunDirection(0.0f, 1.0f, 0.0f)
            , StarGenerationSeed(10)
            , StarDensity(4)
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
            , GatherPerformanceMetrics(false)
        {}
    };
}