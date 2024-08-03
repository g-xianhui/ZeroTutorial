using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;

public class NetworkManager : MonoBehaviour
{
    public static NetworkManager Instance;

    private TcpClient _client;
    private NetworkStream _stream;

    public bool DoPrint = true;
    public bool IsConnected = false;

    private int _bytesWrite = 0;

    void Awake()
    {
        Instance = this;
        DontDestroyOnLoad(gameObject);
    }

    // Start is called before the first frame update
    void Start()
    {
    }

    // Update is called once per frame
    void Update()
    {
    }

    public async Task<bool> ConnectAsync(string host, int port)
    {
        _client = new TcpClient();
        try
        {
            Task connectTask = _client.ConnectAsync(host, port);
            Task delayTask = Task.Delay(2000);
            if (await Task.WhenAny(connectTask, delayTask) == delayTask)
            {
                Debug.Log("connect timeout");
                return false;
            }

            // it may failed immediately when connect to loopback interface
            if (connectTask.Status == TaskStatus.Faulted) {
                foreach (var ex in connectTask.Exception?.InnerExceptions ?? new(Array.Empty<Exception>()))
                {
                    Debug.Log(ex.ToString());
                }
                return false;
            }
        }
        catch (SocketException  e)
        {
            Debug.Log(e.ToString());
            return false;
        }
        catch (ObjectDisposedException e)
        {
            Debug.Log(e.ToString());
            return false;
        }
        catch (Exception e)
        {
            Debug.Log(e.ToString());
            return false;
        }

        Debug.Log("connect to server success");
        return true;
    }

    public async void Connect(string host, int port, Action<bool> connectCallback)
    {
        Debug.Log("connecting to server");
        IsConnected = await ConnectAsync(host, port);
        connectCallback(IsConnected);
    }
}
