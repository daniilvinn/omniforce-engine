#pragma once

implementing Gen.SpriteRenderingInput;

import Gen.BasicTypes;

namespace Omni {

public struct SpriteRenderingInput {
	public ViewData* View;
	public Sprite* Sprites;
	public uint NumSprites;
};

[[vk::push_constant]]
public ConstantBuffer<SpriteRenderingInput> Input;

}
