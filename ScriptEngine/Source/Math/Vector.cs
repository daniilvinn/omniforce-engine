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

        public static Vector2 operator +(Vector2 x, Vector2 y)
        {
            return new Vector2(x.x + y.x, x.y + y.y);
        }

        public Vector2 ToRadians()
        {
            return new Vector2(Math.Radians(x), Math.Radians(y));
        }

        public Vector2 ToDegrees()
        {
            return new Vector2(Math.Degrees(x), Math.Degrees(y));
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
    }

    public struct Vector4
    {
        public float x, y, z, w;
        public Vector3 xyz
        {
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

        public static Vector4 operator +(Vector4 x, Vector4 y)
        {
            return new Vector4(x.x + y.x, x.y + y.y, x.z + y.z, x.w + y.w);
        }

        public Vector4 ToRadians()
        {
            return new Vector4(Math.Radians(x), Math.Radians(y), Math.Radians(z), Math.Radians(w));
        }

        public Vector4 ToDegrees()
        {
            return new Vector4(Math.Degrees(x), Math.Degrees(y), Math.Degrees(y), Math.Degrees(w));
        }

        public override string ToString()
        {
            return $"{x} {y} {z} {w}";
        }
    }
}
