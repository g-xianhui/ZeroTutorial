using Google.Protobuf;
using SpaceService;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using Unity.VisualScripting;
using UnityEngine;

public class Logger
{
    public static void Debug(object obj)
    {
        UnityEngine.Debug.Log(System.DateTime.Now.ToString("MM/dd/yyyy hh:mm:ss.fff") + " : " + obj);
    }
}
public enum ENetRole
{
    Authority,
    Autonomous,
    Simulate
}

public class NetworkComponent : MonoBehaviour
{
    private ENetRole _netRole;
    public ENetRole NetRole
    {
        get => _netRole;
        set => _netRole = value;
    }

    private float _pingInterval = 1f;
    private float _nextPingTime = 0f;

    // Start is called before the first frame update
    void Start()
    {
    }

    // Update is called once per frame
    void Update()
    {
        if (NetRole == ENetRole.Autonomous)
        {
            if (Time.time > _nextPingTime)
            {
                Ping();
                _nextPingTime = Time.time + _pingInterval;
            }
        }
    }

    public void Ping()
    {
        SpaceService.Ping ping = new SpaceService.Ping();
        ping.T = Time.time;
        NetworkManager.Instance.Send("ping", ping.ToByteArray());
    }

    public void Pong(float t)
    {
        float curRtt = Time.time - t;
        float rate = 0.8f;
        NetworkManager.Instance.RTT = NetworkManager.Instance.RTT * rate + (1 - rate) * curRtt;
    }
}
