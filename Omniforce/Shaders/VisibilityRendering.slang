#pragma once

module GeometryRendering;

using namespace Omni;

import Common;
import Rendering;
import Packing;
import Compression;
import Gen.VisibilityRenderingInput;

struct TaskPayload {
    uint BaseOffset;
    uint BaseVisibleClustersOffset;
    uint8_t ClusterIDs[32];
    uint DrawID;
};

groupshared TaskPayload Payload;
groupshared uint NumActiveClusters;

static const uint NumTaskThreads = 32;
static const uint NumMeshThreads = 32;

[numthreads(NumTaskThreads, 1, 1)]
[shader("amplification")]
void CullClusters(
    uint LaneID: SV_GroupIndex, 
    uint DrawID: SV_DrawIndex,
    uint3 ThreadID: SV_DispatchThreadID
) {
    InstanceRenderData Instance = Input.Instances[DrawID];
    GeometryMeshData Mesh = Input.Meshes[Instance.geometry_data_id];
    Transform InstanceTransform = Instance.transform;

    if (ThreadID.x >= Mesh.meshlet_count) {
        return;
    }

    if (LaneID == 0) {
        NumActiveClusters = 0;
    }

    MeshClusterBounds ClusterBounds = Mesh.meshlets_cull_bounds[ThreadID.x];

    bool IsOptimalLOD = true;
    bool IsVisible = true;

    uint WaveOffsetAtomic = 0;
    uint WaveVisibleClusterOffsetAtomic = 0;

    uint BitCount = WaveActiveCountBits(IsVisible && IsOptimalLOD);

    if (WaveIsFirstLane()) {
        InterlockedAdd(NumActiveClusters, BitCount, WaveOffsetAtomic);
        InterlockedAdd(Input.OutputVisibleClusters.NumClusters, BitCount, WaveVisibleClusterOffsetAtomic);
        Payload.BaseVisibleClustersOffset = WaveVisibleClusterOffsetAtomic;
    }

    uint WaveOffset = WaveReadLaneFirst(WaveOffsetAtomic);
    uint WaveVisibleClusterOffset = WaveReadLaneFirst(WaveVisibleClusterOffsetAtomic);

    if (IsVisible && IsOptimalLOD) {
        uint Index = WaveOffset + WavePrefixSum(1);
        uint VisibleClusterIndex = WaveVisibleClusterOffset + Index;
        Input.OutputVisibleClusters.Clusters[VisibleClusterIndex] = SceneVisibleCluster(DrawID, ThreadID.x);
        Payload.ClusterIDs[Index] = uint8_t(ThreadID.x & (NumTaskThreads - 1));
    }

    GroupMemoryBarrier();

    if (LaneID == 0 && NumActiveClusters != 0) {
        Payload.BaseOffset = ThreadID.x;
        Payload.DrawID = DrawID;

        DispatchMesh(NumActiveClusters, 1, 1, Payload);
    }
}

struct MSOutput {
    float4 Position : SV_POSITION;
    uint VisibleClusterIndex;
};

[numthreads(NumMeshThreads, 1, 1)]
[outputtopology("triangle")]
[shader("mesh")]
void RenderClusters(
    uint ThreadIndex: SV_GroupIndex,
    uint GroupID: SV_GroupID,
    out vertices MSOutput Vertices[64],
                    out indices uint3 Triangles[124],
    in payload TaskPayload Payload
) {
    InstanceRenderData Instance = Input.Instances[Payload.DrawID];
    GeometryMeshData Mesh = Input.Meshes[Instance.geometry_data_id];
    Transform InstanceTransform = Instance.transform;

    uint ClusterID = Payload.BaseOffset + Payload.ClusterIDs[GroupID.x];
    ClusterGeometryMetadata ClusterMetadata = Mesh.meshlets_data[ClusterID];
    float3 ClusterCenter = Mesh.meshlets_cull_bounds[ClusterID].vis_culling_sphere.center;

    uint ClusterVertexCount = ClusterMetadata.VertexCount;
    uint ClusterTriangleCount = ClusterMetadata.TriangleCount;
    uint ClusterBitrate = ClusterMetadata.Bitrate;

    if (ThreadIndex == 0) {
        SetMeshOutputCounts(ClusterVertexCount, ClusterTriangleCount);
    }

    uint NumVertexIterations = (ClusterVertexCount + NumMeshThreads - 1) / NumMeshThreads;
    int BitExtendMask = 1 << (ClusterBitrate - 1);

    for (int i = 0; i < NumVertexIterations; i++) {
        if ((ThreadIndex * NumVertexIterations + i) >= ClusterVertexCount) {
            break;
        }

        uint VertexID = ThreadIndex * NumVertexIterations + i;

        int3 EncodedVertex = Mesh.FetchVertex(GroupID, VertexID);

        float3 Vertex = VertexDecoder.Decode(ClusterBitrate, ClusterCenter, EncodedVertex);
        Vertex = InstanceTransform.TransformPoint(Vertex);

        MSOutput Output;
        Output.Position = mul(Input.View.view_proj, float4(Vertex, 1.0));
        Output.VisibleClusterIndex = Payload.BaseVisibleClustersOffset + GroupID;

        Vertices[ThreadIndex + NumVertexIterations + i] = Output;
    }

    uint NumIndexIterations = (ClusterTriangleCount + NumMeshThreads - 1) / NumMeshThreads;
    for (int i = 0; i < NumIndexIterations; i++) {
        if ((ThreadIndex * NumIndexIterations + i) >= ClusterTriangleCount) {
            break;
        }

        uint3 Offsets = ClusterMetadata.triangle_offset + ((ThreadIndex * NumIndexIterations + i) * 3) + uint3(0, 1, 2);
        uint3 Indices = uint3(
            Mesh.micro_indices[Offsets.x],
            Mesh.micro_indices[Offsets.y],
            Mesh.micro_indices[Offsets.z],
        );

        Triangles[ThreadIndex * NumIndexIterations + i] = Offsets;
    }
}

[shader("fragment")]
void EmitTriangleVisibility(MSOutput StageInput, float4 PixelCoordinate: SV_POSITION, uint PrimitiveID : SV_PrimitiveID) {
    uint Depth = reinterpret<uint>(PixelCoordinate.z);
    uint Payload = PrimitiveID;
    Payload |= StageInput.VisibleClusterIndex;

    InterlockedMax(SceneVisibilityBuffer[uint2(PixelCoordinate.xy)], Pack64(uint2(Depth, Payload)));
}