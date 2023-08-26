namespace Omni
{

    public static partial class Math
    {
        public static Vector2 Lerp(Vector2 x, Vector2 y, float factor)
        {
            return x * (1.0f - factor) + (y * factor);
        }

        public static Vector3 Lerp(Vector3 x, Vector3 y, float factor)
        {
            return x * (1.0f - factor) + (y *  factor);
        }

        public static Vector4 Lerp(Vector4 x, Vector4 y, float factor)
        {
            return x * (1.0f - factor) + (y * factor);
        }

        public static float Radians(float value)
        {
            return value * 0.01745329251994329576923690768489f;
        }

        public static double Radians(double value)
        {
            return value * 0.01745329251994329576923690768489;
        }

        public static float Degrees(float value)
        {
            return value * 57.295779513082320876798154814105f;
        }

        public static double Degrees(double value)
        {
            return value * 57.295779513082320876798154814105;
        }

        public static Quat Slerp(Quat x, Quat y, float factor)
        {
            return EngineAPI.QuatSlerp(x, y, factor);
        }

    }

}
