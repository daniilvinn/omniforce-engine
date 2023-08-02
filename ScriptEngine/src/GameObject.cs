using System;

namespace Omni
{
    public abstract class GameObject
    {
        protected GameObject()
        {
            GameObjectID = 0;
        }

        internal GameObject(ulong ID)
        {
            GameObjectID = ID;
            transform = new Transform(this);
        }

        public abstract void OnInit();
        public abstract void OnUpdate();
        public virtual void OnContactEnter() { }
        public virtual void OnContactPersisted() { }
        public virtual void OnContactRemoved() { }

        public T GetComponent<T>() where T : Component, new()
        {
            T component = new T();
            component.Owner = this;
            return component;
        }

        public readonly ulong GameObjectID;
        public Transform transform;
    }
}