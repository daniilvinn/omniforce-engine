namespace Omni
{
    public enum MessageSeverity : byte
    {
        TRACE = 0,
        INFO,
        WARN,
        ERROR,
        CRITICAL,
    }

    public static class Debug
    {
        public static void Log(MessageSeverity severity, string msg)
        {
            EngineAPI.Logger_Log(severity, msg);
        }
    }
}