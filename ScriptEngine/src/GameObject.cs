using System;

namespace Omni
{
    public class GameObject
    {
        protected GameObject()
        {
            GameObjectID = 0;
        }

        internal GameObject(ulong ID)
        {
            GameObjectID = ID;
            Transform = new TransformComponent(this);
        }

        public virtual void OnInit() { }
        public virtual void OnUpdate() { }
        public virtual void OnContactAdded(UUID object_id) { }
        public virtual void OnContactPersisted(UUID object_id) { }
        public virtual void OnContactRemoved(UUID object_id) { }

        public T GetComponent<T>() where T : GameObjectComponent, new()
        {
            T component = new T();
            component.Owner = this;
            return component;
        }

        public static GameObject GetGameObject(string name) 
        { 
            EngineAPI.Entity_GetEntity(name, out UUID object_id);
            return object_id.AsGameObject();
        }

        public readonly ulong GameObjectID;
        public TransformComponent Transform;
    }
}