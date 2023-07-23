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
        public extern static Vector3 Transform_GetTranslation(ulong entity_id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void Transform_SetTranslation(ulong entity_id, Vector3 translation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static Vector3 Transform_GetScale(ulong entity_id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void Transform_SetScale(ulong entity_id, Vector3 scale);

    }
}