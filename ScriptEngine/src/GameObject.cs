namespace Omni
{
    public abstract class GameObject
    {
        protected GameObject()
        {
            entity_id = 0;
        }

        internal GameObject(ulong entity_id)
        {
            this.entity_id = entity_id;
            transform = new Transform(ref this.entity_id);
        }

        public abstract void OnInit();
        public abstract void OnUpdate();
        public virtual void OnContactEnter() { }
        public virtual void OnContactPersisted() { }
        public virtual void OnContactRemoved() { }

        protected readonly ulong entity_id;
        public Transform transform;
    }
}