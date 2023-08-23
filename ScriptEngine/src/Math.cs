namespace Omni
{
    public struct Vector2
    {
        public float x, y;

        public Vector2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public static Vector2 operator *(Vector2 vec, float scalar)
        {
            vec.x *= scalar;
            vec.y *= scalar;

            return vec;
        }

        public override string ToString()
        {
            return $"{x} {y}";
        }
    }

    public struct Vector3
    {
        public float x, y, z;

        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        // TODO: introduce SIMD
        public static Vector3 operator *(Vector3 vec, float scalar)
        {
            vec.x *= scalar;
            vec.y *= scalar;
            vec.z *= scalar;

            return vec;
        }

        public override string ToString()
        {
            return $"{x} {y} {z}";
        }
    }

    public struct Vector4
    {
        public float x, y, z, w;
        public Vector3 xyz { 
            get { return new Vector3(x, y, z); }
            private set { }
        }

        public Vector4(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public Vector4(Vector3 v, float w) 
        { 
            x = v.x;
            y = v.y;
            z = v.z;
            this.w = w;
        }

        public static Vector4 operator *(Vector4 vec, float scalar)
        {
            vec.x *= scalar;
            vec.y *= scalar;
            vec.z *= scalar;
            vec.w *= scalar;

            return vec;
        }

        public override string ToString()
        {
            return $"{x} {y} {z} {w}";
        }
    }

    static public class Math
    {
        static public Vector3 Lerp () 
        {
            return new Vector3(0.0f, 0.0f, 0.0f);
        }
    }

}
