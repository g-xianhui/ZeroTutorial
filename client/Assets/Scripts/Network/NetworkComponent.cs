using Google.Protobuf;
using SpaceService;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

public class NetworkComponent : MonoBehaviour
{
    [Tooltip("movement upload interval in second")]
    public float MovementUploadInterval = 0.03333f;

    private float _nextMovementUploadTime = 0f;
    private CharacterController _characterController;

    // Start is called before the first frame update
    void Start()
    {
        _nextMovementUploadTime = Time.time;
        _characterController = GetComponent<CharacterController>();
    }

    // Update is called once per frame
    void Update()
    {
        if (_nextMovementUploadTime < Time.time)
        {
            UploadMovement();
            _nextMovementUploadTime += MovementUploadInterval;
        }
    }

    public void UploadMovement()
    {
        Vector3 velocity = _characterController.velocity;
        SpaceService.Movement movement = new SpaceService.Movement
        {
            Position = new SpaceService.Vector3f { X = transform.position.x, Y = transform.position.y, Z = transform.position.z },
            Rotation = new SpaceService.Vector3f { X = transform.rotation.eulerAngles.x, Y = transform.rotation.eulerAngles.y, Z = transform.rotation.eulerAngles.z },
            Velocity = new SpaceService.Vector3f { X = velocity.x, Y = velocity.y, Z = velocity.z }
        };
        NetworkManager.Instance.Send("upload_movement", movement.ToByteArray());
    }
}
