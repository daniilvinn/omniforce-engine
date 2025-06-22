#pragma once

module Random;

namespace Omni {

public interface IRNG {
    public uint Generate(inout uint State);
}

public struct PCGGenerator : IRNG {
    public uint Generate(inout uint State)
    {
        State = State * 747796405u + 2891336453u;
        uint Word = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
        return (Word >> 22u) ^ Word;
    };
}

public struct HashGenerator : IRNG {
    public uint Generate(inout uint State)
    {
        State ^= 2747636419u;
        State *= 2654435769u;
        State ^= State >> 16;
        State *= 2654435769u;
        State ^= State >> 16;
        State *= 2654435769u;
        State ^= State >> 16;
        return State == 0u ? 1u : State;
    };
}

public struct RandomNumber<RNG> where RNG : IRNG {
    public __init(uint Seed, ) {
        Seed_ = Seed;
    }

    [mutating]
    public uint Get() {
        RNG Generator;
        return Generator.Generate(Seed_);
    }

    [mutating]
    public float GetNorm(uint Precision = 0xFFFF) {
        return rcp(Precision) * float(Get() & Precision);
    }

    [mutating]
    public T GetXi<T>(uint Precision = 0xFFFF) where T : IRWArray<float> {
        T Result;

        [unroll]
        for (uint i = 0; i < Result.getCount(); i++) {
            Result[i] = GetNorm(Precision);
        }
        return Result;
    }

    private uint Seed_;
}

}