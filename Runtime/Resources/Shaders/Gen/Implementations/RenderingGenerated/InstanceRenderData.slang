#pragma once

implementing Gen.RenderingGenerated;

import Gen.BasicTypes;

namespace Omni {

public struct InstanceRenderData {
	public Transform transform;
	public uint geometry_data_id;
	public uint8_t* material_address;
};

}
