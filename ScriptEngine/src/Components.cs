namespace Omni
{

    public class TransformComponent
    {
        public TransformComponent(GameObject owner)
        {
            Owner = owner;
        }

        public Vector3 Translation
        {
            get { return EngineAPI.TransformComponent_GetTranslation(Owner.GameObjectID); }
            set { EngineAPI.TransformComponent_SetTranslation(Owner.GameObjectID, value); }
        }

        public Vector3 Rotation
        {
            get { return EngineAPI.TransformComponent_GetRotation(Owner.GameObjectID); }
            set { EngineAPI.TransformComponent_SetRotation(Owner.GameObjectID, value); }
        }

        public Vector3 Scale
        {
            get { return EngineAPI.TransformComponent_GetScale(Owner.GameObjectID); }
            set { EngineAPI.TransformComponent_SetScale(Owner.GameObjectID, value); }
        }

        private readonly GameObject Owner;
    }

    public abstract class GameObjectComponent
    {
        public GameObject Owner { get; internal set; }
    }

    public class TagComponent : GameObjectComponent
    {
        public string Tag { 
            get 
            { 
                return EngineAPI.TagComponent_GetTag(Owner.GameObjectID); 
            }
            private set { } 
        }
    }

    public class RigidBody2D : GameObjectComponent
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

    public class SpriteComponent : GameObjectComponent 
    {
        public int Layer
        {
            get
            {
                EngineAPI.SpriteComponent_GetLayer(Owner.GameObjectID, out int layer);
                return layer;
            }
            set
            {
                EngineAPI.SpriteComponent_SetLayer(Owner.GameObjectID, ref value);
            }
        }

        public Vector4 Tint
        {
            get
            {
                EngineAPI.SpriteComponent_GetTint(Owner.GameObjectID, out Vector4 color);
                return color;
            }
            set
            {
                EngineAPI.SpriteComponent_SetTint(Owner.GameObjectID, ref value);
            }
        }
    }

}