#pragma once

module InstanceCulling;

import Gen.BasicTypes;
import Common;
import Gen.InstanceCullingInput;

using namespace Omni;

[shader("compute")]
void FrustumCull(uint3 ThreadID: SV_DispatchThreadID) {
    if (ThreadID.x >= Input.SourceInstanceCount) {
        return;
    }

    InstanceRenderData Instance = Input.SourceInstances[ThreadID.x];
    Transform InstanceTransform = Instance.transform;
    GeometryMeshData GeometryData = Input.Meshes[Instance.geometry_data_id];

    Sphere BoundingSphere = GeometryData.bounding_sphere;

    BoundingSphere.center = InstanceTransform.TransformPoint(BoundingSphere.center);
    BoundingSphere.radius *= max3(InstanceTransform.scale.x, InstanceTransform.scale.y, InstanceTransform.scale.z);

    if (Input.View.frustum.IntersectSphere(BoundingSphere)) {
        uint Index;
        InterlockedAdd(Input.IndirectParams.InstanceCount, 1, Index);

        Input.PostCullInstances[Index] = Instance;

        const uint PreferredWorkGroupSize = 32; // TODO: fix this later
        uint ClusterCount = (GeometryData.meshlet_count + PreferredWorkGroupSize - 1) / PreferredWorkGroupSize;

        Input.IndirectParams.DrawParams[Index] = uint3(ClusterCount, 1, 1);

    }

}