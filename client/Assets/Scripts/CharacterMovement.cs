using Cinemachine;
using Google.Protobuf;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(Animator))]
public class CharacterMovement : MonoBehaviour
{
    [Tooltip("Move speed of the character in m/s")]
    public float MoveSpeed = 2.0f;

    [Tooltip("Sprint speed of the character in m/s")]
    public float SprintSpeed = 5.335f;

    [Tooltip("rotate speed")]
    public float rotSpeed = 0.8f;

    [Tooltip("Acceleration and deceleration")]
    public float SpeedChangeRate = 10.0f;

    [Tooltip("How fast the character turns to face movement direction")]
    [Range(0.0f, 0.3f)]
    public float RotationSmoothTime = 0.12f;

    [Space(10)]
    [Tooltip("The height the player can jump")]
    public float JumpHeight = 1.2f;

    [Tooltip("The character uses its own gravity value. The engine default is -9.81f")]
    public float Gravity = -15.0f;

    [Space(10)]
    [Tooltip("Time required to pass before being able to jump again. Set to 0f to instantly jump again")]
    public float JumpTimeout = 0.50f;

    [Tooltip("Time required to pass before entering the fall state. Useful for walking down stairs")]
    public float FallTimeout = 0.15f;

    [Tooltip("Useful for rough ground")]
    public float GroundedOffset = -0.14f;

    [Tooltip("The radius of the grounded check. Should match the radius of the CharacterController")]
    public float GroundedRadius = 0.28f;

    [Tooltip("What layers the character uses as ground")]
    public LayerMask GroundLayers;

    [Header("Cinemachine")]
    [Tooltip("The follow target set in the Cinemachine Virtual Camera that the camera will follow")]
    public GameObject CinemachineCameraTarget;

    [Tooltip("How far in degrees can you move the camera up")]
    public float TopClamp = 70.0f;

    [Tooltip("How far in degrees can you move the camera down")]
    public float BottomClamp = -30.0f;

    [Tooltip("For locking the camera position on all axis")]
    public bool LockCameraPosition = false;

    private Animator _anim;
    private CharacterController _controller;
    private NetworkComponent _networkComponent;
    private GameObject _mainCamera;

    private float _speed = 0.0f;
    private float _targetRotation = 0.0f;
    private float _rotationVelocity;
    private float _verticalVelocity;

    // timeout deltatime
    private float _jumpTimeoutDelta = 0.0f;
    // current frame delta height
    public float _deltaHeight = 0.0f;

    // Animation Variable
    public bool Grounded = true;
    public bool IsJumping = false;
    public bool IsFalling = false;
    public bool IsFlying = false;
    public bool IgnoreGravity = false;

    // cinemachine
    private float _cinemachineTargetYaw;
    private float _cinemachineTargetPitch;

    // upload movement
    [Tooltip("interval of movement upload to server in second")]
    public float MovementUploadInterval = 0.03333f;
    private float _nextMovementUploadTime = 0f;
    // 用于计算角速度
    private Quaternion _lastFrameRotation;
    // 用于计算加速度
    private Vector3 _lastFrameVelocity;

    public bool EnableMovement { get; set; } = true;

    void Awake()
    {
        _anim = GetComponent<Animator>();
        _controller = GetComponent<CharacterController>();
        _networkComponent = GetComponent<NetworkComponent>();
        _mainCamera = GameObject.FindGameObjectWithTag("MainCamera");
    }

    // Start is called before the first frame update
    void Start()
    {
        _nextMovementUploadTime = Time.time;
        _lastFrameRotation = transform.rotation;
        _lastFrameVelocity = Vector3.zero;
    }

    // Update is called once per frame
    void Update()
    {
        if (_networkComponent != null && _networkComponent.NetRole == ENetRole.Simulate)
            return;

        GroundedCheck();

        JumpAndGravity();

        Move();

        UpdateAnimation();

        CheckUploadMovement();
    }

    // 我们想应用root motion，但默认的animator move会叠加重力的处理，但显然现在重力是由player controller接管的，需要忽略。
    //private void OnAnimatorMove()
    //{
    //    transform.position += _anim.deltaPosition;
    //    transform.Rotate(_anim.deltaRotation.eulerAngles);
    //}

    private void LateUpdate()
    {
        if (_networkComponent != null && _networkComponent.NetRole == ENetRole.Simulate)
            return;
        CameraRotation();
    }

    public float GetSpeed() { return _speed; }
    public void SetSpeed(float speed) { _speed = speed; }

    private void GroundedCheck()
    {
        // set sphere position, with offset
        Vector3 spherePosition = new Vector3(transform.position.x, transform.position.y - GroundedOffset,
            transform.position.z);
        Grounded = Physics.CheckSphere(spherePosition, GroundedRadius, GroundLayers,
            QueryTriggerInteraction.Ignore);
    }

    void OnDrawGizmosSelected()
    {
        // Draw a yellow sphere at the transform's position
        //Vector3 spherePosition = new Vector3(transform.position.x, transform.position.y - GroundedOffset, transform.position.z);
        //Gizmos.color = Color.yellow;
        //Gizmos.DrawSphere(spherePosition, GroundedRadius);
    }

    private void JumpAndGravity()
    {
        bool isJumpPressed = false;
        bool isFlyPressed = false;

        if (EnableMovement)
        {
            isJumpPressed = Input.GetButtonDown("Jump");
            isFlyPressed = Input.GetButtonDown("Fly");
        }

        _deltaHeight = _verticalVelocity * Time.deltaTime;
        if (!IgnoreGravity)
            _verticalVelocity += Gravity * Time.deltaTime;

        if (Grounded)
        {
            // stop our velocity dropping infinitely when grounded
            if (_verticalVelocity < 0.0f)
            {
                _verticalVelocity = -2f;
            }

            // stop falling
            IsFalling = false;

            if (isJumpPressed && _jumpTimeoutDelta <= 0.0f)
            {
                // the square root of H * -2 * G = how much velocity needed to reach desired height
                _verticalVelocity = Mathf.Sqrt(JumpHeight * -2f * Gravity);
                IsJumping = true;
                _jumpTimeoutDelta = JumpTimeout;
            }
        }

        if (_jumpTimeoutDelta > 0.0f)
        {
            _jumpTimeoutDelta -= Time.deltaTime;
        }

        if (IsFlying)
        {
            // stop fly mode, fall down to ground
            if (isFlyPressed)
            {
                IsFalling = true;
                IsFlying = false;
                IgnoreGravity = false;
            }
        }

        if (IsJumping)
        {
            if (_verticalVelocity <= 0)
            {
                // stop jumping and start falling
                IsJumping = false;
                IsFalling = true;
            }
            else
            {
                // start to fly mode
                if (isFlyPressed)
                {
                    IsJumping = false;
                    IsFlying = true;
                    IgnoreGravity = true;
                    _verticalVelocity = 0.0f;
                }
            }
        }
    }

    void Move()
    {
        // Store the input axes.
        float h = 0f;
        float v = 0f;

        if (EnableMovement)
        {
            h = Input.GetAxis("Horizontal");
            v = Input.GetAxis("Vertical");
        }

        float targetSpeed = 0.0f;
        if (Input.GetKey(KeyCode.LeftShift))
        {
            targetSpeed = SprintSpeed;
        }
        else
        {
            targetSpeed = MoveSpeed;
        }

        Vector2 move = new Vector2(h, v);
        if (move == Vector2.zero)
        {
            targetSpeed = 0.0f;
        }

        // a reference to the players current horizontal velocity
        float currentHorizontalSpeed = new Vector3(_controller.velocity.x, 0.0f, _controller.velocity.z).magnitude;
        float speedOffset = 0.1f;

        if (currentHorizontalSpeed < targetSpeed - speedOffset ||
            currentHorizontalSpeed > targetSpeed + speedOffset)
        {
            _speed = Mathf.Lerp(_speed, targetSpeed, Time.deltaTime * SpeedChangeRate);
        }
        else
        {
            _speed = targetSpeed;
        }

        if (move != Vector2.zero)
        {
            Vector3 inputDirection = new Vector3(h, 0, v).normalized;
            // 相机的朝向就是角色的正面朝向，后者可能落后于前者，例如站着不动，直接调整相机的朝向。所以要先转到相机方向，再转到输入的方向。
            _targetRotation = Mathf.Atan2(inputDirection.x, inputDirection.z) * Mathf.Rad2Deg + _mainCamera.transform.eulerAngles.y;

            // SmoothDampAngle不是插值的实现，而是另一种效果（implements a critically damped harmonic oscillator），所以不要用插值的思维去理解这个函数。
            // 同时，这个函数需要一个ref velocity变量，这个变量的生命周期要支撑到到达target。
            float rotation = Mathf.SmoothDampAngle(transform.eulerAngles.y, _targetRotation, ref _rotationVelocity, RotationSmoothTime);

            // rotate to face input direction relative to camera position
            transform.rotation = Quaternion.Euler(0.0f, rotation, 0.0f);
        }

        // 移动方向是固定的，转身是在移动的过程中完成的
        Vector3 targetDirection = Quaternion.Euler(0.0f, _targetRotation, 0.0f) * Vector3.forward;

        // move the player
        _controller.Move(targetDirection.normalized * (_speed * Time.deltaTime) + new Vector3(0.0f, _deltaHeight, 0.0f));
    }

    private void CameraRotation()
    {
        // h/v 这里返回的是鼠标的偏移里，单位可能是世界坐标，也可能是像素，但无关重要。rotSpeed的单位是： 角度/每单位偏移量，所以h/v乘以rotSpeed之后可以直接当作角度来用。
        float h = Input.GetAxis("Mouse X");
        float v = Input.GetAxis("Mouse Y");

        // 不能直接rotate，想象一下先低头90度，然后向左转90度，低头没问题，但左转时本意是绕Y轴（Space.Self）旋转90度，但低头90度时Y轴同样已经绕X轴旋转了90度，导致效果就完全不对了。
        // 正确的方式是两个轴的旋转量分别累加，这样两个轴之间就不会产生干扰，而且引擎底下是用四元数来表示旋转，不会有死锁的问题。
        // CinemachineCameraTarget.transform.Rotate(v, h, 0);

        if ((h != 0f || v != 0f) && !LockCameraPosition)
        {
            _cinemachineTargetYaw += h * rotSpeed;
            _cinemachineTargetPitch += v * rotSpeed;
        }

        // clamp our rotations so our values are limited 360 degrees
        _cinemachineTargetYaw = ClampAngle(_cinemachineTargetYaw, float.MinValue, float.MaxValue);
        _cinemachineTargetPitch = ClampAngle(_cinemachineTargetPitch, BottomClamp, TopClamp);

        // pitch draw me crazy
        _cinemachineTargetPitch = 0f;

        // Cinemachine will follow this target
        CinemachineCameraTarget.transform.rotation = Quaternion.Euler(_cinemachineTargetPitch, _cinemachineTargetYaw, 0.0f);
    }

    private static float ClampAngle(float lfAngle, float lfMin, float lfMax)
    {
        if (lfAngle < -360f) lfAngle += 360f;
        if (lfAngle > 360f) lfAngle -= 360f;
        return Mathf.Clamp(lfAngle, lfMin, lfMax);
    }

    public int GetMoveMode()
    {
        int result = 0;
        if (Grounded)
        {
            result |= 1;
        }

        if (IsJumping)
        {
            result |= 2;
        }

        if (IsFalling)
        {
            result |= 4;
        }

        if (IsFlying)
        {
            result |= 8;
        }

        if (EnableMovement)
        {
            result |= 16;
        }

        return result;
    }

    public void UpdateMoveMode(int mode)
    {
        Grounded = false;
        IsJumping = false;
        IsFalling = false;
        IsFlying = false;
        EnableMovement = false;

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

        if ((mode & 16) != 0)
        {
            EnableMovement = true;
        }
    }

    public void UpdateAnimation()
    {
        _anim.SetFloat("Speed", _speed);
        _anim.SetBool("Grounded", Grounded);
        _anim.SetBool("Jumping", IsJumping);
        _anim.SetBool("Falling", IsFalling);
        _anim.SetBool("Flying", IsFlying);
    }

    private void CheckUploadMovement()
    {
        if (_networkComponent != null && _networkComponent.NetRole == ENetRole.Autonomous)
        {
            if (_nextMovementUploadTime < Time.time)
            {
                UploadMovement();
                _nextMovementUploadTime += MovementUploadInterval;
            }
            _lastFrameVelocity = _controller.velocity;
            _lastFrameRotation = transform.rotation;
        }
    }

    public void UploadMovement()
    {
        Vector3 velocity = _controller.velocity;
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
            Mode = GetMoveMode(),
            Timestamp = Time.time,
        };
        NetworkManager.Instance.Send("upload_movement", movement.ToByteArray());
    }
}
