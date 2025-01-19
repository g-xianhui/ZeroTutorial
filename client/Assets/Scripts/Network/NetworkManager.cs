using SpaceService;
using System;
using System.Buffers.Binary;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using TMPro;
using Unity.VisualScripting.FullSerializer;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.SceneManagement;
using UnityEngine.UIElements;
using UnityEngine.XR;
using static UnityEngine.EventSystems.EventTrigger;
using Google.Protobuf;

public class BitUtils
{
    public static int Read7BitEncodedInt(byte[] bytes, out int out_pos)
    {
        int value = 0;

        byte c;
        int i = 0;
        int s = 0;
        do
        {
            c = bytes[i++];
            int x = (c & 0x7F);
            x <<= s;
            value += x;
            s += 7;
        } while ((c & 0x80) != 0);

        out_pos = i;
        return value;
    }
}

public class RecvBuffer
{
    enum ParseStage
    {
        TLEN, DATA
    }

    static readonly int TLEN_SIZE = 5;
    byte[] _tlenBytes = new byte[TLEN_SIZE];
    int _tlenBytesPosition = 0;

    ParseStage _parseStage = ParseStage.TLEN;
    int _needBytes = 1;

    public byte[] buffer = null;
    public int position = 0;

    public byte[][] Recv(byte[] bytes, int n)
    {
        int bindex = 0;

        List<byte[]> msgs = new List<byte[]>();

        while (_needBytes > 0 && bindex < n)
        {
            switch (_parseStage)
            {
                case ParseStage.TLEN:
                    int index = TLEN_SIZE - _needBytes;
                    _tlenBytes[_tlenBytesPosition++] = bytes[bindex];
                    if ((bytes[bindex] & 0x80) == 0)
                        _needBytes = 0;

                    bindex += 1;
                    if (_needBytes == 0)
                    {
                        _parseStage = ParseStage.DATA;
                        _needBytes = CalcPackageDataLength();
                        _tlenBytesPosition = 0;

                        buffer = new byte[_needBytes];
                    }
                    break;
                case ParseStage.DATA:
                    int leftBytesNum = n - bindex;
                    if (leftBytesNum < _needBytes)
                    {
                        Buffer.BlockCopy(bytes, bindex, buffer, position, leftBytesNum);
                        _needBytes -= leftBytesNum;
                        bindex += leftBytesNum;
                        position += leftBytesNum;
                    }
                    else
                    {
                        Buffer.BlockCopy(bytes, bindex, buffer, position, _needBytes);
                        bindex += _needBytes;
                        position = 0;

                        // finish one msg
                        byte[] msg = new byte[_needBytes];
                        Buffer.BlockCopy(buffer, 0, msg, 0, _needBytes);
                        msgs.Add(msg);

                        // reset to initial state
                        _parseStage = ParseStage.TLEN;
                        _needBytes = 1;
                    }
                    break;
            }
        }

        return msgs.ToArray();
    }

    int Read7BitEncodedInt(byte[] bytes)
    {
        int value = 0;

        byte c;
        int i = 0;
        int s = 0;
        do
        {
            c = bytes[i++];
            int x = (c & 0x7F);
            x <<= s;
            value += x;
            s += 7;
        } while ((c & 0x80) != 0);

        return value;
    }

    int CalcPackageDataLength()
    {
        return Read7BitEncodedInt(_tlenBytes);
    }
}

class SendBuffer
{
    public byte[] _buffer;
    public uint _capacity;
    // _buffer[_startPos, _endPos] 为未被发送的数据
    public uint _startPos, _endPos;
    public uint _sendBytes = 0;

    private AutoResetEvent _dataEvent = new AutoResetEvent(false);

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
        _dataEvent.Set();
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

    public bool WaitDataEvent()
    {
        _dataEvent.WaitOne();
        return true;
    }

    public void Close()
    {
        _dataEvent.Set();
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
    private RecvBuffer _recvBuffer = null;

    private ConcurrentQueue<byte[]> _msgQueue = new ConcurrentQueue<byte[]>();
    private Dictionary<string, GameObject> _players = new Dictionary<string, GameObject>();
    private GameObject _mainPlayer = null;

    private float _rtt = 0.1f;
    public float RTT
    {
        get => _rtt;
        set
        {
            _rtt = value;
        }
    }

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
        HandleNetworkMsg();
    }

    private void HandleNetworkMsg()
    {
        while (true)
        {
            byte[] bytes = null;
            if (_msgQueue.TryDequeue(out bytes!))
            {
                HandleNetworkMsg(bytes);
            }
            else
            {
                break;
            }
        }
    }

    private void HandleNetworkMsg(byte[] bytes)
    {
        int i = 0;
        int msgNameLenght = BitUtils.Read7BitEncodedInt(bytes, out i);
        string msgName = Encoding.ASCII.GetString(bytes, i, msgNameLenght);

        byte[] msgBytes = bytes.Skip(i + msgNameLenght).ToArray();
        var m = typeof(NetworkManager).GetMethod(msgName);
        if (m != null)
        {
            object[] args = new object[1];
            args[0] = msgBytes;
            m.Invoke(this, args);
        }
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
        _recvBuffer = new RecvBuffer();
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
            await Task.WhenAll(sendTask, readTask);
        }
    }

    public void Close()
    {
        Debug.Log("close socket");
        _isConnected = false;
        _stream.Close();
        _stream = null;
    }

    public void Write7BitEncodedInt(BinaryWriter writer, int value)
    {
        uint uValue = (uint)value;

        // Write out an int 7 bits at a time. The high bit of the byte,
        // when on, tells reader to continue reading more bytes.
        //
        // Using the constants 0x7F and ~0x7F below offers smaller
        // codegen than using the constant 0x80.

        while (uValue > 0x7Fu)
        {
            writer.Write((byte)(uValue | ~0x7Fu));
            uValue >>= 7;
        }

        writer.Write((byte)uValue);
    }

    public void Send(byte[] bytes)
    {
        using (MemoryStream mem = new MemoryStream())
        {
            using (BinaryWriter binaryWriter = new BinaryWriter(mem))
            {
                Write7BitEncodedInt(binaryWriter, bytes.Length);
                binaryWriter.Write(bytes);
            }
            byte[] data = mem.ToArray();
            _sendBuffer.Send(data);
        }
    }

    public void Send(string msg)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(msg);
        Send(bytes);
    }

    public void Send(string msgName, byte[] msgBytes)
    {
        using (MemoryStream mem = new MemoryStream())
        {
            using (BinaryWriter binaryWriter = new BinaryWriter(mem))
            {
                byte[] msgNameBytes = Encoding.ASCII.GetBytes(msgName);
                Write7BitEncodedInt(binaryWriter, msgNameBytes.Length);
                binaryWriter.Write(msgNameBytes);

                if (msgBytes != null)
                    binaryWriter.Write(msgBytes);
            }

            byte[] data = mem.ToArray();
            Send(data);
        }
    }

    private void SendThreadFunc()
    {
        while (_isConnected)
        {
            _sendBuffer.WaitDataEvent();
            try
            {
                _sendBuffer.Flush(_stream);
            }
            catch (Exception e)
            {
                Debug.LogError(e.ToString());
                break;
            }
        }

        if (_isConnected)
            OnLostServer();
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

                byte[][] msgs = _recvBuffer.Recv(buffer, nReads);

                foreach (byte[] bytes in msgs)
                {
                    _msgQueue.Enqueue(bytes);
                }
            }
            catch (Exception e)
            {
                Debug.LogError(e.ToString());
                break;
            }
        }

        if (_isConnected)
            OnLostServer();
    }

    private void OnLostServer()
    {
        Debug.LogError("OnLostServer");
        Close();
    }

    private IEnumerator loadScene() {
        AsyncOperation asyncLoad = SceneManager.LoadSceneAsync("DemoScene");
        while (!asyncLoad.isDone)
        {
            yield return null;
        }

        Send("join", null);
    }

    public void login_reply(byte[] msgBytes)
    {
        SpaceService.LoginReply loginReply = SpaceService.LoginReply.Parser.ParseFrom(msgBytes);
        if (loginReply.Result == 0)
        {
            Debug.Log("login successed");
            StartCoroutine(loadScene());
        }
        else
        {
            Debug.Log("login failed: " + loginReply.Result);
        }
    }

    public void join_reply(byte[] msgBytes)
    {
        SpaceService.JoinReply joinReply = SpaceService.JoinReply.Parser.ParseFrom(msgBytes);
        if (joinReply.Result == 0)
        {
            Vector3 position = new Vector3(joinReply.Position.X, joinReply.Position.Y, joinReply.Position.Z);
            Debug.Log($"join space successed! position: {position}");
            GameObject prefab = Resources.Load<GameObject>("Character/MainCharacter");
            if (prefab != null)
            {
                GameObject mainCharacter = GameObject.Instantiate(prefab, position, Quaternion.identity);
                NetworkComponent networkComponent = mainCharacter.GetComponent<NetworkComponent>();
                networkComponent.NetRole = ENetRole.Autonomous;
                _mainPlayer = mainCharacter;
            }
            else
            {
                Debug.Log("main character not found");
            }
        }
        else
        {
            Debug.Log("join failed: " + joinReply.Result);
        }
    }

    public void players_enter_sight(byte[] msgBytes)
    {
        SpaceService.PlayersEnterSight sight = SpaceService.PlayersEnterSight.Parser.ParseFrom(msgBytes);
        foreach (SpaceService.AoiPlayer aoiPlayer in sight.Players)
        {
            SpaceService.Vector3f aoiPosition = aoiPlayer.Transform.Position;
            SpaceService.Vector3f aoiRotation = aoiPlayer.Transform.Rotation;
            Vector3 position = new Vector3(aoiPosition.X, aoiPosition.Y, aoiPosition.Z);
            Quaternion rotation = Quaternion.Euler(aoiRotation.X, aoiRotation.Y, aoiRotation.Z);
            Debug.Log($"player enter sight, name: {aoiPlayer.Name}, position: {position}, rotation: {rotation.eulerAngles}");

            GameObject prefab = Resources.Load<GameObject>("Character/OtherCharacter");
            if (prefab != null)
            {
                GameObject otherPlayer = GameObject.Instantiate(prefab, position, rotation);
                NetworkComponent networkComponent = otherPlayer.GetComponent<NetworkComponent>();
                networkComponent.NetRole = ENetRole.Simulate;
                networkComponent.InitMovement(aoiPlayer.Transform);
                _players.Add(aoiPlayer.Name, otherPlayer);
            }
            else
            {
                Debug.Log("other character prefab not found");
            }
        }
    }

    public void players_leave_sight(byte[] msgBytes)
    {
        SpaceService.PlayersLeaveSight sight = SpaceService.PlayersLeaveSight.Parser.ParseFrom(msgBytes);
        foreach (string playerName in sight.Players)
        {
            GameObject player = null;
            if (_players.Remove(playerName, out player))
            {
                GameObject.Destroy(player);
            }
        }
    }

    public GameObject find_player(string name)
    {
        return _players.GetValueOrDefault(name);
    }

    public void sync_movement(byte[] msgBytes)
    {
        SpaceService.PlayerMovements playerMovements = SpaceService.PlayerMovements.Parser.ParseFrom(msgBytes);
        foreach (SpaceService.PlayerMovement playerMovement in playerMovements.Datas)
        {
            GameObject otherPlayer = find_player(playerMovement.Name);
            if (otherPlayer != null)
            {
                ServerMovePack serverMovePack = new ServerMovePack
                {
                    Position = new Vector3(playerMovement.Data.Position.X, playerMovement.Data.Position.Y, playerMovement.Data.Position.Z),
                    Rotation = Quaternion.Euler(playerMovement.Data.Rotation.X, playerMovement.Data.Rotation.Y, playerMovement.Data.Rotation.Z),
                    Velocity = new Vector3(playerMovement.Data.Velocity.X, playerMovement.Data.Velocity.Y, playerMovement.Data.Velocity.Z),
                    Acceleration = new Vector3(playerMovement.Data.Acceleration.X, playerMovement.Data.Acceleration.Y, playerMovement.Data.Acceleration.Z),
                    AngularVelocity = new Vector3(playerMovement.Data.AngularVelocity.X, playerMovement.Data.AngularVelocity.Y, playerMovement.Data.AngularVelocity.Z),
                    Mode = playerMovement.Data.Mode,
                    TimeStamp = playerMovement.Data.Timestamp,
                };
                NetworkComponent networkComponent = otherPlayer.GetComponent<NetworkComponent>();
                networkComponent.SyncMovement(serverMovePack);
            }
        }
    }

    public void pong(byte[] msgBytes)
    {
        SpaceService.Pong pong = SpaceService.Pong.Parser.ParseFrom(msgBytes);
        NetworkComponent networkComponent = _mainPlayer.GetComponent<NetworkComponent>();
        networkComponent.Pong(pong.T);
    }
}
