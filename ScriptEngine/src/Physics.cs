namespace Omni
{

    public static class Physics
    {
        public static Vector3 Gravity
        {
            get
            {
                EngineAPI.Physics_GetGravity(out Vector3 Gravity);
                return Gravity;
            }
            set
            {
                EngineAPI.Physics_SetGravity(ref value);
            }
        }
    }

}