using Omni; // using engine namespace

// Use whatever namespace you want
namespace test
{
    // Must inherin from Omni.GameObject to create script
    public class MyScript : GameObject
    {
        // Must implement OnInit()
        public override void OnInit()
        {
            Debug.Log(ToString());
        }

        // Must implement OnUpdate(float ts), where ts is time step
        public override void OnUpdate(float ts)
        {
            Debug.Log(ts.ToString());
        }
    }

    
}