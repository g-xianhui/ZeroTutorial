using System;
using System.Buffers.Binary;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Unity.VisualScripting.FullSerializer;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UIElements;
using UnityEngine.XR;

class SendBuffer
{
    public byte[] _buffer;
    public uint _capacity;
    // _buffer[_startPos, _endPos] 为未被发送的数据
    public uint _startPos, _endPos;
    public uint _sendBytes = 0;

    public SendBuffer(uint len)
    {
        _buffer = new byte[len];
        _capacity = len;
        // _startPos由发送线程修改，_endPos由主线程修改，理论上不会有线程问题
        _startPos = _endPos = 0;
    }

    public bool Empty()
    {
        return _startPos == _endPos; 
    }

    public uint Length()
    {
        return _endPos >= _startPos ? _endPos - _startPos : _endPos + _capacity - _startPos;
    }

    public void Send(byte[] data)
    {
        uint dataLength = (uint)data.Length;
        if (dataLength + Length() > _capacity)
        {
            throw new OverflowException("SendBuffer overflow");
        }

        uint lastCapacity = _capacity - _endPos;
        uint copyLength = Math.Min(dataLength, lastCapacity);
        Buffer.BlockCopy(data, 0, _buffer, (int)_endPos, (int)copyLength);

        if (copyLength < dataLength)
        {
            Buffer.BlockCopy(data, (int)copyLength, _buffer, 0, (int)(dataLength - copyLength));
        }

        _endPos = (_endPos + dataLength) % _capacity;
    }

    public void Flush(NetworkStream stream)
    {
        if (_endPos == _startPos)
            return;

        uint saveEndPos = _endPos;
        if (_endPos > _startPos)
        {
            int len = (int)(_endPos - _startPos);
            stream.Write(_buffer, (int)_startPos, len);

            _sendBytes += (uint)len;
        }
        else
        {
            int len = (int)(_capacity - _startPos);
            stream.Write(_buffer, (int)_startPos, len);
            stream.Write(_buffer, 0, (int)_endPos);

            _sendBytes += (uint)len + _endPos;
        }

        _startPos = saveEndPos;
    }
}

public class NetworkManager : MonoBehaviour
{
    public static NetworkManager Instance;

    [Tooltip("connect timeout in millisecond")]
    public int ConnectTimeoutMs = 2000;

    private TcpClient _client;
    private NetworkStream _stream;

    private bool _isConnected = false;

    private uint _sendBufferSize = 64 * 1024;
    private SendBuffer _sendBuffer = null;

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

    public async Task<bool> ConnectAsync(string host, int port, int connectTimeoutMs)
    {
        _client = new TcpClient();
        try
        {
            Task connectTask = _client.ConnectAsync(host, port);
            Task delayTask = Task.Delay(connectTimeoutMs);
            if (await Task.WhenAny(connectTask, delayTask) == delayTask)
            {
                Debug.LogError("connect timeout");
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
        catch (Exception e)
        {
            Debug.LogError(e.ToString());
            return false;
        }

        Debug.Log("connect to server success");
        _stream = _client.GetStream();

        _sendBuffer = new SendBuffer(_sendBufferSize);
        return true;
    }

    public async void Connect(string host, int port, Action<bool> connectCallback)
    {
        Debug.Log("connecting to server");
        _isConnected = await ConnectAsync(host, port, ConnectTimeoutMs);
        connectCallback(_isConnected);

        if (_isConnected)
        {
            Task sendTask = Task.Run(() => { SendThreadFunc(); });
            Task readTask = Task.Run(() => { RecvThreadFunc(); });
            Task.WhenAll(sendTask, readTask);
        }
    }

    public void Close()
    {
        _client.Close();
        _isConnected = false;
    }

    public void Send(string msg)
    {
        Debug.Log("Send: " + msg);
        byte[] bytes = Encoding.UTF8.GetBytes(msg);
        _sendBuffer.Send(bytes);
    }

    private void SendThreadFunc()
    {
        while (_isConnected)
        {
            if (_sendBuffer.Empty())
            {
                Thread.Sleep(100);
            }
            else
            {
                try
                {
                    _sendBuffer.Flush(_stream);
                }
                catch (Exception e)
                {
                    Debug.LogError(e.ToString());
                    Close();
                    break;
                }
            }
        }
    }

    private void RecvThreadFunc()
    {
        byte[] buffer = new byte[2048];
        while (_isConnected)
        {
            try
            {
                int nReads = _stream.Read(buffer, 0, buffer.Length);
                if (nReads == 0)
                    break;
                string msg = Encoding.UTF8.GetString(buffer, 0, nReads);
                Debug.Log("Recv: " + msg);
            }
            catch (Exception e)
            {
                Debug.LogError(e.ToString());
                break;
            }
        }
    }

    private void OnLostServer()
    {
        Debug.LogError("OnLostServer");
        _isConnected = false;
    }
}
