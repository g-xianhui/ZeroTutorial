using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(Animator))]
public class FootIK : MonoBehaviour
{
    [Header("foot ik")]
    [SerializeField]
    public bool UseFootIK = false;
    [Range(0, 2)]
    [SerializeField]
    public float HeightFromGroundRaycast = 1.2f;
    [Range(0, 2)]
    [SerializeField]
    public float RaycastDownDistance = 1.5f;
    // 用于修正bodyPosition，不知道为什么计算出来的值总是差一点
    [SerializeField]
    public float PelivsOffset = 0;

    [Tooltip("What layers the character uses as ground")]
    public LayerMask GroundLayers;

    private Animator _anim;
    private PlayerController _controller;
    private NetworkComponent _networkComp;

    void Awake()
    {
        _anim = GetComponent<Animator>();
        _controller = GetComponent<PlayerController>();
        _networkComp = GetComponent<NetworkComponent>();

        Application.targetFrameRate = 30;
    }

    private void CalcFootIK(HumanBodyBones bone, Vector3 rayDir, out Vector3 targetIKPos, float bottomHeight, ref Quaternion targetIKRotation)
    {
        Vector3 rayStartPos = _anim.GetBoneTransform(bone).position;
        rayStartPos.y += HeightFromGroundRaycast;

        bool isGround = Physics.Raycast(rayStartPos, rayDir, out RaycastHit hitInfo, HeightFromGroundRaycast + RaycastDownDistance, GroundLayers);
        if (isGround)
        {
            targetIKPos = hitInfo.point;
            targetIKPos.y += bottomHeight;

            Quaternion rotation = Quaternion.FromToRotation(Vector3.up, hitInfo.normal);
            // 注意相乘的顺序不能调转，试一下就知道了
            targetIKRotation = rotation * targetIKRotation;
        }
        else
        {
            targetIKPos = Vector3.zero;
        }
    }

    void MovePelvisHeight(Vector3 leftFootTargetIKPos, Vector3 rightFootTargetIKPos)
    {
        float lOffsetPosition = leftFootTargetIKPos.y - transform.position.y;
        float rOffsetPosition = rightFootTargetIKPos.y - transform.position.y;

        float totalOffset = (lOffsetPosition < rOffsetPosition) ? lOffsetPosition : rOffsetPosition;

        Vector3 newPelvisPosition = _anim.bodyPosition + Vector3.up * (totalOffset + PelivsOffset);
        _anim.bodyPosition = newPelvisPosition;
    }

    private void OnAnimatorIK(int layerIndex)
    {
        if (!UseFootIK)
            return;

        if (_controller !=  null)
        {
            if (_controller.IsFalling || _controller.IsJumping) return;
            if (_controller.GetSpeed() > 0) return;
        }
        else
        {
            if (_networkComp.IsFalling || _networkComp.IsJumping) return;
            if (_networkComp.GetSpeed() > 0) return;
        }

        float ikWeight = 1f;
        _anim.SetIKPositionWeight(AvatarIKGoal.LeftFoot, ikWeight);
        _anim.SetIKRotationWeight(AvatarIKGoal.LeftFoot, ikWeight);

        _anim.SetIKPositionWeight(AvatarIKGoal.RightFoot, ikWeight);
        _anim.SetIKRotationWeight(AvatarIKGoal.RightFoot, ikWeight);

        Vector3 leftFootTargetIKPos, rightFootTargetIKPos;
        Quaternion leftFootTargetIKRotation = _anim.GetIKRotation(AvatarIKGoal.LeftFoot);
        Quaternion rightFootTargetIKRotation = _anim.GetIKRotation(AvatarIKGoal.RightFoot);

        Vector3 rayDir = Vector3.down;
        // for left foot
        CalcFootIK(HumanBodyBones.LeftFoot, rayDir, out leftFootTargetIKPos, _anim.leftFeetBottomHeight, ref leftFootTargetIKRotation);
        //for the right foot
        CalcFootIK(HumanBodyBones.RightFoot, rayDir, out rightFootTargetIKPos, _anim.rightFeetBottomHeight, ref rightFootTargetIKRotation);

        if (leftFootTargetIKPos == Vector3.zero || rightFootTargetIKPos == Vector3.zero)
            return;

        MovePelvisHeight(leftFootTargetIKPos, rightFootTargetIKPos);

        _anim.SetIKPosition(AvatarIKGoal.LeftFoot, leftFootTargetIKPos);
        _anim.SetIKPosition(AvatarIKGoal.RightFoot, rightFootTargetIKPos);

        _anim.SetIKRotation(AvatarIKGoal.LeftFoot, leftFootTargetIKRotation);
        _anim.SetIKRotation(AvatarIKGoal.RightFoot, rightFootTargetIKRotation);
    }
}
