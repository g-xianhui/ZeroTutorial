using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class NetworkComponent : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKey(KeyCode.E))
        {
            NetworkManager.Instance.Send("hello");
        }
    }
}
