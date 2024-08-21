using System.Collections;
using System.Collections.Generic;
using System.Net.Sockets;
using UnityEngine;

public class NetworkTcpClient
{
    private TcpClient _client;

    public NetworkTcpClient()
    {
        _client = new TcpClient();
    }
}
