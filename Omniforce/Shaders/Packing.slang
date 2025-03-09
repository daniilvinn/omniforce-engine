#pragma once

module Packing;

namespace Omni {

public uint2 Unpack32(uint64_t Value) {
    return uint2((uint)(Value >> 32), (uint)(Value & 0xFFFFFFFF));
}

}