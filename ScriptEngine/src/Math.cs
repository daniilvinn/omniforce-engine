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

        public override string ToString()
        {
            return $"{x} {y} {z} {w}";
        }
    }

    public class Transform
    {
        public Transform(ref ulong entity_id)
        {
            this.entity_id = entity_id;
        }

        public Vector3 translation
        {
            get { return EngineAPI.Transform_GetTranslation(entity_id); }
            set { EngineAPI.Transform_SetTranslation(entity_id, value); }
        }

        public Vector3 scale
        {
            get { return EngineAPI.Transform_GetScale(entity_id); }
            set { EngineAPI.Transform_SetScale(entity_id, value); }
        }

        private readonly ulong entity_id;
    }
}
