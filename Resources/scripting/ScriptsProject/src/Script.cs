using Omni; // using engine namespace

// Use whatever namespace you want
namespace test
{
    // Must inherit from Omni.GameObject to create script
    public class MyScript : GameObject
    {
        // Must implement OnInit()
        // Represents constructor and called before first frame starts
        public override void OnInit()
        {
            Debug.Log(MessageSeverity.TRACE, "test");
        }

        // Must implement OnUpdate(float ts), where ts is time step
        // Called every frame
        public override void OnUpdate()
        {
            if(Input.KeyPressed(KeyCode.KEY_W)) {
                Debug.Log(MessageSeverity.INFO, "Test internal call");
            }
        }
    }  
}