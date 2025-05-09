using SpaceService;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using static UnityEngine.PlayerLoop.PreUpdate;

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

public enum ESyncMovementMode
{
    Direct,
    Interpolate,
    Predict
}

[RequireComponent(typeof(CharacterMovement))]
public class SimulateMovement : IComponent
{
    private CharacterController _characterController;
    private CharacterMovement _characterMovement;

    [Tooltip("movement sync mode")]
    public ESyncMovementMode SyncMovementMode;
    
    [Tooltip("interval of movement sync from server in second")]
    public float MovementSyncInterval = 0.1f;

    // 当前本地速度
    private Vector3 _curVelocity;

    // ====== 插值模式 =======
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

    // ====== 预测模式 =======
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

    // velocity blending
    private Vector3 _localVelocity;
    private Vector3 _serverVelocity;
    private Vector3 _serverAcceleration;

    // position blending
    private Vector3 _localPosition;
    private Vector3 _serverPosition;
    private bool _positionBlending = false;
    private float _positionBlendingTime = 0f;

    // rotation blending
    private Quaternion _serverRotation;
    private Quaternion _localRotation;
    private Vector3 _serverAngularVelocity;
    private bool _rotationBlending = false;
    private float _rotationBlendingTime = 0f;

    // Start is called before the first frame update
    void Start()
    {
        _characterController = GetComponent<CharacterController>();
        _characterMovement = GetComponent<CharacterMovement>();

        ForwardPredictMovement();
    }

    // Update is called once per frame
    void Update()
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

        // root motion控制位移时不能设置speed，否则会触发正常的移动动画，与root motion产生冲突
        if (_characterMovement.EnableMovement)
        {
            Vector3 horizontalV = new Vector3(_curVelocity.x, 0, _curVelocity.z);
            _characterMovement.SetSpeed(horizontalV.magnitude);
        }

        _characterMovement.UpdateAnimation();
    }

    public void InitMovement(SpaceService.Movement movement)
    {
        if (SyncMovementMode == ESyncMovementMode.Predict)
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
        if (SyncMovementMode == ESyncMovementMode.Predict)
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

            _characterMovement.UpdateMoveMode(serverMovePack.Mode);
        }
    }

    private void ApplyMovementFromServer(ServerMovePack serverMovePack)
    {
        _curVelocity = (serverMovePack.Position - transform.position) / MovementSyncInterval;

        transform.position = serverMovePack.Position;
        transform.rotation = serverMovePack.Rotation;

        if (_characterController != null)
        {
            _characterMovement.UpdateMoveMode(serverMovePack.Mode);
        }
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

                float realInterval = MovementSyncInterval;
                if (_lastSimulateTimeStamp > 0)
                {
                    realInterval = serverMovePack.TimeStamp - _lastSimulateTimeStamp;
                }
                _lastSimulateTimeStamp = serverMovePack.TimeStamp;
                _curVelocity = (_endPos - _startPos) / realInterval;
                _characterMovement.UpdateMoveMode(serverMovePack.Mode);

                _isInterpolating = true;
            }
            else
            {
                _isInterpolating = false;
                _lerpTimePass = 0;
                _curVelocity = Vector3.zero;
                _characterMovement.UpdateMoveMode(serverMovePack.Mode);
            }
        }
        else
        {
            _isInterpolating = false;
            _lerpTimePass = 0;
            _curVelocity = Vector3.zero;
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

    public override void NetSerialize(BinaryReader br)
    {
        byte[] data = NetSerializer.ReadBytes(br);
        SpaceService.Movement newMove = SpaceService.Movement.Parser.ParseFrom(data);

        ServerMovePack serverMovePack = new ServerMovePack
        {
            Position = new Vector3(newMove.Position.X, newMove.Position.Y, newMove.Position.Z),
            Rotation = Quaternion.Euler(newMove.Rotation.X, newMove.Rotation.Y, newMove.Rotation.Z),
            Velocity = new Vector3(newMove.Velocity.X, newMove.Velocity.Y, newMove.Velocity.Z),
            Acceleration = new Vector3(newMove.Acceleration.X, newMove.Acceleration.Y, newMove.Acceleration.Z),
            AngularVelocity = new Vector3(newMove.AngularVelocity.X, newMove.AngularVelocity.Y, newMove.AngularVelocity.Z),
            Mode = newMove.Mode,
            TimeStamp = newMove.Timestamp,
        };

        ApplyMovementFromServer(serverMovePack);
    }

    public override void NetDeltaSerialize(BinaryReader br)
    {
        byte[] data = NetSerializer.ReadBytes(br);
        SpaceService.Movement newMove = SpaceService.Movement.Parser.ParseFrom(data);

        ServerMovePack serverMovePack = new ServerMovePack
        {
            Position = new Vector3(newMove.Position.X, newMove.Position.Y, newMove.Position.Z),
            Rotation = Quaternion.Euler(newMove.Rotation.X, newMove.Rotation.Y, newMove.Rotation.Z),
            Velocity = new Vector3(newMove.Velocity.X, newMove.Velocity.Y, newMove.Velocity.Z),
            Acceleration = new Vector3(newMove.Acceleration.X, newMove.Acceleration.Y, newMove.Acceleration.Z),
            AngularVelocity = new Vector3(newMove.AngularVelocity.X, newMove.AngularVelocity.Y, newMove.AngularVelocity.Z),
            Mode = newMove.Mode,
            TimeStamp = newMove.Timestamp,
        };

        SyncMovement(serverMovePack);
    }
}
