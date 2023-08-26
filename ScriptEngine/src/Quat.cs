﻿namespace Omni
{
    public struct Quat
    {

        public Quat(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public Quat(Vector3 EulerAngles)
        {
            this = EngineAPI.BuildQuatFromEulerAngles(EulerAngles);
        }

        public readonly Quat Rotate(float angle, Vector3 axis)
        {
            return EngineAPI.QuatRotate(this, angle, axis);
        }

        public readonly Quat Normalize()
        {
            return EngineAPI.QuatNormalize(this);
        }

        public float x, y, z, w;
    }
}