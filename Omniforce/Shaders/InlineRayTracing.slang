#pragma once

module InlineRT;

import Common;

namespace Omni {

public struct HitInfo {
    public uint InstanceIndex;
    public uint GeometryIndex;
    public float Distance;
    public bool Hit;
}

public struct InlineRayTracer {

    /*
    *  Shadow ray tracing
    */
    public HitInfo TraceRay(RayDesc Desc) {
        const uint RayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | 
                              RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

        RayQuery<RayFlags> Query;
        Query.TraceRayInline(
            SceneTLAS,
            RayFlags,
            0xFF,
            Desc
        );

        Query.Proceed();

        HitInfo Info;
        Info.Hit = Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;

        if (Info.Hit) {
            Info.InstanceIndex = Query.CommittedInstanceIndex();
            Info.GeometryIndex = Query.CommittedGeometryIndex();
            Info.Distance = Query.CommittedRayT();
        }

        return Info;
    }
}

}