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

public enum ESyncMovementMode
{
    Direct,
    Interpolate,
    Predict
}

public class ServerMovePack
{
    public Vector3 Position;
    public Quaternion Rotation;
    public Vector3 Velocity;
    public Vector3 Acceleration;
    public Vector3 AngularVelocity;
    public int Mode;
    public float TimeStamp;
}

public class NetworkComponent : MonoBehaviour
{

    private CharacterController _characterController;
    private PlayerController _playerController;
    private Animator _anim;

    private ENetRole _netRole;
    public ENetRole NetRole
    {
        get => _netRole;
        set => _netRole = value;
    }

    [Tooltip("movement sync mode")]
    public ESyncMovementMode SyncMovementMode;

    [Tooltip("interval of movement upload to server in second")]
    public float MovementUploadInterval = 0.03333f;
    private float _nextMovementUploadTime = 0f;

    [Tooltip("interval of movement sync from server in second")]
    public float MovementSyncInterval = 0.1f;

    // 从网络同步过来的位移数据包队列
    private Queue<ServerMovePack> _interpolateMovements = new Queue<ServerMovePack>();
    // 位移插值的起始点
    private Vector3 _startPos;
    private Quaternion _startRot;
    // 位移插值的终点
    private Vector3 _endPos;
    private Quaternion _endRot;
    // 位移插值当前经过的时长
    private float _lerpTimePass = 0f;
    private bool _isInterpolating = false;
    private float _lastSimulateTimeStamp = 0f;

    // 位移相差超过该值开始修正
    [Tooltip("correction distance")]
    public float PredictionCorrectionDistance = 1f;
    // 位移相差超过该值强制立即更新
    [Tooltip("hard snap distance")]
    public float PredictionHardSnapDistance = 3f;
    // 朝向相差超过该值开始修正
    [Tooltip("correction rotation")]
    public float PredictionCorrectionRotation = 30f;
    // 朝向相差超过该值强制立即更新
    [Tooltip("hard snap rotation")]
    public float PredictionHardSnapRotation = 90f;
    // 人形角色的旋转速度太快了，预测十有八九是错的，或者就把同步的频率调高
    [Tooltip("enable rotation predicte")]
    public bool EnableRotationPredicte = false;

    private Vector3 _curVelocity;
    private Vector3 _localVelocity;
    private Vector3 _serverVelocity;
    private Vector3 _serverAcceleration;

    // 用于计算角速度
    private Quaternion _lastFrameRotation;
    // 用于计算加速度
    private Vector3 _lastFrameVelocity;

    private Vector3 _localPosition;
    private Vector3 _serverPosition;
    private bool _positionBlending = false;
    private float _positionBlendingTime = 0f;

    private Quaternion _serverRotation;
    private Quaternion _localRotation;
    private Vector3 _serverAngularVelocity;
    private bool _rotationBlending = false;
    private float _rotationBlendingTime = 0f;

    private float _pingInterval = 1f;
    private float _nextPingTime = 0f;

    public bool Grounded = false;
    public bool IsJumping = false;
    public bool IsFalling = false;
    public bool IsFlying = false;

    // Start is called before the first frame update
    void Start()
    {
        _nextMovementUploadTime = Time.time;
        _characterController = GetComponent<CharacterController>();
        _playerController = GetComponent<PlayerController>();
        _anim = GetComponent<Animator>();

        _lastFrameRotation = transform.rotation;
        _lastFrameVelocity = Vector3.zero;

        ForwardPredictMovement();

        Logger.Debug("Netcompnent start");
    }

    // Update is called once per frame
    void Update()
    {
        if (NetRole == ENetRole.Autonomous)
        {
            if (_nextMovementUploadTime < Time.time)
            {
                UploadMovement();
                _nextMovementUploadTime += MovementUploadInterval;
            }
            _lastFrameVelocity = _characterController.velocity;
            _lastFrameRotation = transform.rotation;

            if (Time.time > _nextPingTime)
            {
                Ping();
                _nextPingTime = Time.time + _pingInterval;
            }
        }
        else if (NetRole == ENetRole.Simulate)
        {
            if (SyncMovementMode == ESyncMovementMode.Interpolate)
            {
                InterpolateMovement();
            }
            else if (SyncMovementMode == ESyncMovementMode.Predict)
            {
                PredicteMovement();
                PredictRotate();
            }

            Vector3 horizontalV = new Vector3(_curVelocity.x, 0, _curVelocity.z);
            _anim.SetFloat("Speed", horizontalV.magnitude);
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

    public void InitMovement(SpaceService.Movement movement)
    {
        if (NetRole == ENetRole.Simulate && SyncMovementMode == ESyncMovementMode.Predict)
        {
            Vector3 Velocity = new Vector3(movement.Velocity.X, movement.Velocity.Y, movement.Velocity.Z);
            Vector3 Acceleration = new Vector3(movement.Acceleration.X, movement.Acceleration.Y, movement.Acceleration.Z);
            Vector3 AngularVelocity = new Vector3(movement.AngularVelocity.X, movement.AngularVelocity.Y, movement.AngularVelocity.Z);

            _serverPosition = new Vector3(movement.Position.X, movement.Position.Y, movement.Position.Z);
            _serverRotation = Quaternion.Euler(movement.Rotation.X, movement.Rotation.Y, movement.Rotation.Z);
            _serverVelocity = Velocity;
            _serverAcceleration = Acceleration;
            _serverAngularVelocity = AngularVelocity;
        }
    }

    private void ForwardPredictMovement()
    {
        if (NetRole == ENetRole.Simulate && SyncMovementMode == ESyncMovementMode.Predict)
        {
            // 立马向前预测RTT时间，对齐服务端时间线
            _serverPosition = ForwardPredictePosition(_serverPosition, _serverVelocity, _serverAcceleration, NetworkManager.Instance.RTT);
            _serverRotation = ForwardPredicteRotation(_serverRotation, _serverAngularVelocity, NetworkManager.Instance.RTT);
            _serverVelocity = _serverVelocity + _serverAcceleration * NetworkManager.Instance.RTT;

            transform.position = _serverPosition;
            transform.rotation = _serverRotation;
            _localPosition = transform.position;
            _localRotation = transform.rotation;
            _localVelocity = _serverVelocity;
            _curVelocity = _localVelocity;
        }
    }

    public void UploadMovement()
    {
        Vector3 velocity = _characterController.velocity;
        Vector3 acceleration = (velocity - _lastFrameVelocity) / Time.deltaTime;

        Quaternion deltaRotation = transform.rotation * Quaternion.Inverse(_lastFrameRotation);
        // deltaRotation可能是个负数，但此时通过deltaRotation.eulerAngles得到的却是一个正数，估计范围是[180, 360)，但其实没有转那么大的角度，只是方向反了而已。例如359，其实只是转了1度。
        // 因此使用Mathf.DeltaAngle计算真正的最小旋转角度，例如359，就变成-1了。
        Vector3 eulerRotation = new Vector3(
            Mathf.DeltaAngle(0, deltaRotation.eulerAngles.x),
            Mathf.DeltaAngle(0, deltaRotation.eulerAngles.y),
            Mathf.DeltaAngle(0, deltaRotation.eulerAngles.z));
        Vector3 angularVelocity = eulerRotation / Time.deltaTime;

        SpaceService.Movement movement = new SpaceService.Movement
        {
            Position = new SpaceService.Vector3f { X = transform.position.x, Y = transform.position.y, Z = transform.position.z },
            Rotation = new SpaceService.Vector3f { X = transform.rotation.eulerAngles.x, Y = transform.rotation.eulerAngles.y, Z = transform.rotation.eulerAngles.z },
            Velocity = new SpaceService.Vector3f { X = velocity.x, Y = velocity.y, Z = velocity.z },
            Acceleration = new SpaceService.Vector3f { X = acceleration.x, Y = acceleration.y, Z = acceleration.z },
            AngularVelocity = new SpaceService.Vector3f { X = angularVelocity.x, Y = angularVelocity.y, Z = angularVelocity.z },
            Mode = _playerController.GetMoveMode(),
            Timestamp = Time.time,
        };
        NetworkManager.Instance.Send("upload_movement", movement.ToByteArray());
    }

    public void SyncMovement(ServerMovePack serverMovePack)
    {
        if (SyncMovementMode == ESyncMovementMode.Direct)
        {
            ApplyMovementFromServer(serverMovePack);
        }
        else if (SyncMovementMode == ESyncMovementMode.Interpolate)
        {
            _interpolateMovements.Enqueue(serverMovePack);
        }
        else
        {
            serverMovePack.Position = ForwardPredictePosition(serverMovePack.Position, serverMovePack.Velocity, serverMovePack.Acceleration, NetworkManager.Instance.RTT);
            serverMovePack.Velocity = serverMovePack.Velocity + serverMovePack.Acceleration * NetworkManager.Instance.RTT;

            _serverPosition = serverMovePack.Position;
            _serverAcceleration = serverMovePack.Acceleration;
            _serverVelocity = serverMovePack.Velocity;

            float dist = Vector3.Distance(serverMovePack.Position, transform.position);
            if (dist > PredictionHardSnapDistance)
            {
                HardSnapToServerState(serverMovePack);
                _positionBlending = false;
            }
            else
            {
                _localVelocity = _curVelocity;
                _localPosition = transform.position;
                if (dist > PredictionCorrectionDistance)
                {
                    _positionBlending = true;
                }
                else
                {
                    _positionBlending = false;
                }
            }
            _positionBlendingTime = 0f;

            _serverAngularVelocity = serverMovePack.AngularVelocity;
            if (EnableRotationPredicte)
            {
                _serverRotation = ForwardPredicteRotation(serverMovePack.Rotation, serverMovePack.AngularVelocity, NetworkManager.Instance.RTT);
            }
            else
            {
                _serverRotation = serverMovePack.Rotation;
            }

            Quaternion deltaRotation = transform.rotation * Quaternion.Inverse(_serverRotation);
            float rot = Mathf.Abs(Mathf.DeltaAngle(0, deltaRotation.eulerAngles.y));
            if (rot > PredictionHardSnapRotation)
            {
                transform.rotation = _serverRotation;
                _rotationBlending = false;
            }
            else
            {
                _localRotation = transform.rotation;
                if (rot > PredictionCorrectionRotation)
                {
                    _rotationBlending = true;
                }
                else
                {
                    _rotationBlending = false;
                }
            }
            _rotationBlendingTime = 0f;

            UpdateMoveMode(serverMovePack.Mode);
        }
    }

    private void ApplyMovementFromServer(ServerMovePack serverMovePack)
    {
        _curVelocity = (serverMovePack.Position - transform.position) / MovementSyncInterval;

        transform.position = serverMovePack.Position;
        transform.rotation = serverMovePack.Rotation;

        UpdateMoveMode(serverMovePack.Mode);
    }

    private void InterpolateMovement()
    {
        if (_isInterpolating)
        {
            // 插值进行中
            _lerpTimePass += Time.deltaTime;
            float t = Mathf.Min(1f, _lerpTimePass / MovementSyncInterval);
            transform.position = Vector3.Lerp(_startPos, _endPos, t);
            transform.rotation = Quaternion.Slerp(_startRot, _endRot, t);

            if (_lerpTimePass >= MovementSyncInterval)
            {
                _lerpTimePass = _lerpTimePass - MovementSyncInterval;
                CheckInterpolateMovement();
            }
        }
        else
        {
            CheckInterpolateMovement();
        }
    }

    private void CheckInterpolateMovement()
    {
        if (_interpolateMovements.Count > 0)
        {
            ServerMovePack serverMovePack = _interpolateMovements.Dequeue();
            float dist = Vector3.Distance(transform.position, serverMovePack.Position);
            if (dist > 0.01)
            {
                _startPos = transform.position;
                _startRot = transform.rotation;

                _endPos = serverMovePack.Position;
                _endRot = serverMovePack.Rotation;

                float t = Mathf.Min(1f, _lerpTimePass / MovementSyncInterval);
                transform.position = Vector3.Lerp(_startPos, _endPos, t);
                transform.rotation = Quaternion.Slerp(_startRot, _endRot, t);

                // FIXME: 目前服务器并没有在实时计算更新玩家的位置，服务器的位置采用的是由玩家主动上传的值。所以:
                // velocity = (_endPos - _startPos) / MovementSyncInterval
                // 计算出来的的velocity并不准确，服务器在MovementSyncInterval内不一定就能稳定收到来自客户端总计移动了MovementSyncInterval的数据包。
                // 因此，当采用上面的公式计算时，会发现角色的移动动画偶尔会突变，例如本来autonomous稳定的跑动，速度是5，但到了simulator上，速度有时是5，
                // 但偶尔会先跳到3，然后再跳到7,就是数据同步没那么稳定导致的。
                // 在当前服务器还没自己实时计算更新的情况下，我们只能做一些妥协，让simulator看起来流畅一些，因此这里就把_curVelocity设为了autonomous
                // 原本的移动速度，但这个速度只会应用到动画上，simulator真实的移动速度还是上面公式计算的值。虽然偶尔动画与实际的速度不一致，细看可能会发
                // 现滑步之类的情况，但已经比动画突变要好很多了。
                float realInterval = MovementSyncInterval;
                if (_lastSimulateTimeStamp > 0)
                {
                    realInterval = serverMovePack.TimeStamp - _lastSimulateTimeStamp;
                }
                _lastSimulateTimeStamp = serverMovePack.TimeStamp;
                _curVelocity = (_endPos - _startPos) / realInterval;

                UpdateMoveMode(serverMovePack.Mode);

                _isInterpolating = true;
            }
            else
            {
                _isInterpolating = false;
                _lerpTimePass = 0;
                _curVelocity = Vector3.zero;
            }
        }
        else
        {
            _isInterpolating = false;
            _lerpTimePass = 0;
        }
    }

    private void PredicteMovement()
    {
        _positionBlendingTime += Time.deltaTime;
        float rate = Mathf.Min(1.0f, _positionBlendingTime / MovementSyncInterval);
        _curVelocity = _localVelocity + (_serverVelocity - _localVelocity) * rate;

        if (!_positionBlending)
        {
            _characterController.Move(_curVelocity * Time.deltaTime);
        }
        else
        {
            Vector3 P0 = ForwardPredictePosition(_localPosition, _curVelocity, _serverAcceleration, _positionBlendingTime);
            Vector3 P1 = ForwardPredictePosition(_serverPosition, _serverVelocity, _serverAcceleration, _positionBlendingTime);
            Vector3 newPosition = P0 + (P1 - P0) * rate;
            Vector3 movement = newPosition - transform.position;
            _characterController.Move(movement);
        }
    }

    private void PredictRotate()
    {
        _rotationBlendingTime += Time.deltaTime;
        float rate = Mathf.Min(1.0f, _rotationBlendingTime / MovementSyncInterval);

        if (!_rotationBlending || !EnableRotationPredicte)
        {
            transform.Rotate(_serverAngularVelocity * Time.deltaTime);
        }
        else
        {
            Quaternion Q0 = ForwardPredicteRotation(_localRotation, _serverAngularVelocity, _rotationBlendingTime);
            Quaternion Q1 = ForwardPredicteRotation(_serverRotation, _serverAngularVelocity, _rotationBlendingTime);
            Quaternion newRotation = Quaternion.Slerp(Q0, Q1, rate);
            transform.rotation = newRotation;
        }
    }

    private void HardSnapToServerState(ServerMovePack serverMovePack)
    {
        _characterController.enabled = false;
        transform.position = serverMovePack.Position;
        _characterController.enabled = true;

        _localVelocity = serverMovePack.Velocity;
        _localPosition = serverMovePack.Position;
    }

    private Vector3 ForwardPredictePosition(Vector3 position, Vector3 velocity, Vector3 acceleration, float t)
    {
        Vector3 originPosition = transform.position;
        _characterController.enabled = false;
        transform.position = position;
        _characterController.enabled = true;
        Vector3 movement = velocity * t + 0.5f * acceleration * t * t;
        _characterController.Move(movement);

        Vector3 newPosition = transform.position;
        _characterController.enabled = false;
        transform.position = originPosition;
        _characterController.enabled = true;
        return newPosition;
    }

    private Quaternion ForwardPredicteRotation(Quaternion rotation, Vector3 velocity, float t)
    {
        return rotation * Quaternion.Euler(velocity * t);
    }

    private void UpdateMoveMode(int mode)
    {
        IsJumping = false;
        IsFalling = false;
        Grounded = false;
        IsFlying = false;

        if ((mode & 1) != 0)
        {
            Grounded = true;
        }

        if ((mode & 2) != 0)
        {
            IsJumping = true;
        }

        if ((mode & 4) != 0)
        {
            IsFalling = true;
        }

        if ((mode & 8) != 0)
        {
            IsFlying = true;
        }

        _anim.SetBool("Grounded", Grounded);
        _anim.SetBool("Jumping", IsJumping);
        _anim.SetBool("Falling", IsFalling);
        _anim.SetBool("Flying", IsFlying);
    }

    public float GetSpeed()
    {
        Vector3 horizontalV = new Vector3(_curVelocity.x, 0, _curVelocity.z);
        return horizontalV.magnitude;
    }
}
