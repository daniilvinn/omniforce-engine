namespace Omni
{
    public enum MessageSeverity : byte
    {
        TRACE       = 1 << 0,
        INFO        = 1 << 1,
        WARN        = 1 << 2,
        ERROR       = 1 << 3,
        CRITICAL    = 1 << 4
    }

    public static class Debug
    {
        public static void Log(MessageSeverity severity, string msg)
        {
            EngineAPI.Logger_Log(severity, msg);
        }
    }
}