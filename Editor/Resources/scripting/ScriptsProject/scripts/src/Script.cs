using Omni; // using engine namespace

// Use whatever namespace you want
namespace Game
{
    // Must inherit from Omni.GameObject to create script
    public class MyScript : GameObject
    {
        // Must implement OnInit()
        // Represents constructor and called before first frame starts
        public override void OnInit()
        {
            
        }

        // Must implement OnUpdate(), where ts is time step
        // Called every frame
        public override void OnUpdate()
        {
            
        }
    }  
}