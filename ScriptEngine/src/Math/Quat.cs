namespace Omni
{
    public struct Quat
    {
        public Quat(float x, float y, float z, float w)
        {
            this.x = x; this.y = y;
            this.z = z; this.w = w;
        }
        public Quat(Vector3 EulerAngles)
        {
            this = EngineAPI.BuildQuatFromEulerAngles(EulerAngles);
        }

        public static Quat Identity
        {
            get { return new Quat(0.0f, 0.0f, 0.0f, 1.0f); }
            private set { }
        }

        public Vector3 EulerAngles
        {
            get { return EngineAPI.QuatToEulerAngles(this); }
            private set { }
        }

        // angle in radians
        public Quat Rotate(float angle, Vector3 axis)
        {
            return EngineAPI.QuatRotate(this, angle, axis);
        }

        public Quat Normalize()
        {
            return EngineAPI.QuatNormalize(this);
        }

        public Quat Inverse()
        {
            return EngineAPI.QuatInverse(this);
        }

        public override string ToString()
        {
            return $"{x} {y} {z} {w}";
        }

        public float x, y, z, w;

    }
}

