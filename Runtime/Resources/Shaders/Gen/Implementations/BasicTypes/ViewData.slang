#pragma once

implementing Gen.BasicTypes;


namespace Omni {

public struct ViewData {
	public float4x4 view;
	public float4x4 proj;
	public float4x4 view_proj;
	public float3 position;
	public Frustum frustum;
	public float3 forward_vector;
	public float fov;
	public float viewport_height;
	public float viewport_width;
	public float near_clip_distance;
	public float far_clip_distance;
	public float gamma;
	public float exposure;
};

}
