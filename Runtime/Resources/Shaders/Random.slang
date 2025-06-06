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

    private uint Seed_;
}

}