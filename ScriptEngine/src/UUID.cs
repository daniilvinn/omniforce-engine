namespace Omni
{

    public struct UUID
    {
        public UUID (ulong id)
        {
            ID = id;
        }

        public static bool operator== (UUID a, UUID b)
        {
            return a.ID == b.ID;
        }

        public static bool operator !=(UUID a, UUID b)
        {
            return a.ID != b.ID;
        }

        public GameObject AsGameObject() {
            return new GameObject(ID);
        }

        public ulong ID { get; private set; }

    }

}