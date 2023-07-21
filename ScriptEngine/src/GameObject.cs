namespace Omni
{
    public abstract class GameObject
    {

        public abstract void OnInit();
        public abstract void OnUpdate();
        public virtual void OnContactEnter() { }
        public virtual void OnContactPersisted() { }
        public virtual void OnContactRemoved() { }

        protected readonly ulong entity_id;
    }
}