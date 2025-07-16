#pragma once

module InlineRayTracing;

import Common;

namespace Omni {

public struct HitInfo {
    public float3 Position;
    public uint InstanceIndex;
    public uint InstanceID;
    public uint GeometryIndex;
    public uint SBTOffset;
    public float Distance;
    public bool Hit;
}

public struct InlineRayTracer {

    /*
    *  Shadow ray tracing
    */

    public HitInfo TraceRay<uint RayFlags>(RayDesc Desc) {
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
            Info.InstanceID = Query.CommittedInstanceID();
            Info.Distance = Query.CommittedRayT();
            Info.SBTOffset = Query.CommittedRayInstanceShaderBindingTableRecordOffset();
            Info.Position = Desc.Origin + Desc.Direction * Info.Distance;
        }

        return Info;
    }


}

}