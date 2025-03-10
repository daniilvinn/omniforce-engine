#pragma once

module Packing;

namespace Omni {

public uint2 Unpack32(uint64_t Value) {
    return uint2((uint)(Value >> 32), (uint)(Value & 0xFFFFFFFF));
}

public uint64_t Pack64(uint2 Value) {
    return (uint64_t(Value.x) << 32) | Value.y;
}

}