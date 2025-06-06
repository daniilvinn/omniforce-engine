#pragma once

implementing Gen.ToneMapping;

import Gen.BasicTypes;

namespace Omni {

public struct ToneMappingPassInput {
	public ViewData* View;
};

[[vk::push_constant]]
public ConstantBuffer<ToneMappingPassInput> Input;

}
