using System.Runtime.CompilerServices;

namespace Omni
{
    internal static class EngineAPI
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static bool Input_KeyPressed(KeyCode key_code);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void Logger_Log(MessageSeverity severity, string msg);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static Vector3 TransformComponent_GetTranslation(ulong entity_id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_SetTranslation(ulong entity_id, Vector3 translation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static Vector3 TransformComponent_GetRotation(ulong entity_id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_SetRotation(ulong entity_id, Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static Vector3 TransformComponent_GetScale(ulong entity_id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void TransformComponent_SetScale(ulong entity_id, Vector3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static string TagComponent_GetTag(ulong entity_id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RigidBody2D_GetLinearVelocity(ulong entity_id, out Vector3 LinearVelocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RigidBody2D_SetLinearVelocity(ulong entity_id, ref Vector3 LinearVelocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RigidBody2D_AddLinearImpulse(ulong entity_id, ref Vector3 impulse);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RigidBody2D_AddForce(ulong entity_id, ref Vector3 force);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RigidBody2D_AddTorque(ulong entity_id, ref Vector3 torque);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void Physics_GetGravity(out Vector3 Gravity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void Physics_SetGravity(ref Vector3 Gravity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void Entity_GetEntity(string name, out UUID object_id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SpriteComponent_GetLayer(ulong entity_id, out int layer);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SpriteComponent_SetLayer(ulong entity_id, ref int layer);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SpriteComponent_GetTint(ulong entity_id, out Vector4 tint);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SpriteComponent_SetTint(ulong entity_id, ref Vector4 tint);

    }
}