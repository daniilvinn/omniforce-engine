#pragma once

implementing Gen.RenderingGenerated;


namespace Omni {

public struct RayTracingGeometryData {
	public uint3* indices;
	public GeometryLayoutTable layout;
	public uint8_t* attributes;
};

}
