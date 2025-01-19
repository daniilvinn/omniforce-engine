namespace Omni
{
    public static partial class Math
    {
        static public Vector2 Lerp(Vector2 x, Vector2 y, float factor)
        {
            return EngineAPI.Vector2Lerp(x, y, factor);
        }

        static public Vector3 Lerp(Vector3 x, Vector3 y, float factor)
        {
            return EngineAPI.Vector3Lerp(x, y, factor);
        }

        static public Vector4 Lerp(Vector4 x, Vector4 y, float factor)
        {
            return EngineAPI.Vector4Lerp(x, y, factor);
        }

        static public Quat Slerp(Quat x, Quat y, float factor)
        {
            return EngineAPI.QuatSlerp(x, y, factor);
        }

        static public float Degrees(float radians)
        {
            return radians * 57.295779513082320876798154814105f;
        }

        static public float Radians(float degrees)
        {
            return degrees * 0.01745329251994329576923690768489f;
        }
    }
}