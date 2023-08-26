
namespace Omni
{
    public struct Vector3
    {
        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static Vector3 operator *(Vector3 vec, float scalar)
        {
            return EngineAPI.Vector3MultiplyByScalar(vec, scalar);
        }

        public static Vector3 operator +(Vector3 x, Vector3 y)
        {
            return new Vector3(x.x + y.x, x.y + y.y, x.z + y.z);
        }

        public Vector3 ToRadians()
        {
            return new Vector3(Math.Radians(x), Math.Radians(y), Math.Radians(z));
        }

        public Vector3 ToDegrees()
        {
            return new Vector3(Math.Degrees(x), Math.Degrees(y), Math.Degrees(y));
        }

        public override string ToString()
        {
            return $"{x} {y} {z}";
        }

        public float x, y, z;
    }
}