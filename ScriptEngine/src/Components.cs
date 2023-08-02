namespace Omni
{

    public abstract class Component
    {
        public GameObject Owner { get; internal set; }
    }

    public class TagComponent : Component
    {
        public string Tag { 
            get 
            { 
                return EngineAPI.TagComponent_GetTag(Owner.GameObjectID); 
            }
            private set { } 
        }
    }

    public class RigidBody2D : Component
    {
        public void AddLinearImpulse (Vector3 impulse)
        {
            EngineAPI.RigidBody2D_AddLinearImpulse(Owner.GameObjectID, ref impulse);
        }

        public Vector3 LinearVelocity
        {
            get
            {
                EngineAPI.RigidBody2D_GetLinearVelocity(Owner.GameObjectID, out Vector3 velocity);
                return velocity;
            }
            set
            {
                EngineAPI.RigidBody2D_SetLinearVelocity(Owner.GameObjectID, ref value);
            }
        }

        public void AddForce(Vector3 force)
        {
            EngineAPI.RigidBody2D_AddForce(Owner.GameObjectID, ref force);
        }

        public void AddTorque(Vector3 torque)
        {
            EngineAPI.RigidBody2D_AddTorque(Owner.GameObjectID, ref torque);
        }

    }

}