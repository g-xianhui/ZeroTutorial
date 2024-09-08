using System.Collections;
using System.Collections.Generic;
using System.Text;
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
            string s = GetRandomCharacters(200);
            NetworkManager.Instance.Send(s);
        }
    }

    public static string GetRandomCharacters(int n = 10, bool Number = true, bool Lowercase = false, bool Capital = false)
    {
        StringBuilder tmp = new StringBuilder();
        string characters = (Capital ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ" : null) + (Number ? "0123456789" : null) + (Lowercase ? "abcdefghijklmnopqrstuvwxyz" : null);
        if (characters.Length < 1)
        {
            return (null);
        }
        for (int i = 0; i < n; i++)
        {
            tmp.Append(characters[Random.Range(0, characters.Length)].ToString());
        }
        return (tmp.ToString());
    }
}
