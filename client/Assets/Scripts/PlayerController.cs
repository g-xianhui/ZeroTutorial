using Google.Protobuf;
using SpaceService;
using UnityEngine;
using UnityEngine.AI;

public class PlayerController : MonoBehaviour
{
    CombatComponent combatComponent;
    Animator _anim;
    NavMeshAgent _agent;

    void Awake()
    {
    }

    // Start is called before the first frame update
    void Start()
    {
        Cursor.lockState = CursorLockMode.Locked;
        combatComponent = GetComponent<CombatComponent>();
        _anim = GetComponent<Animator>();
        _agent = GetComponent<NavMeshAgent>();
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKey(KeyCode.Escape))
            Cursor.lockState = CursorLockMode.None;
        else if (Input.GetKey(KeyCode.RightAlt))
            Cursor.lockState = CursorLockMode.Locked;

        CheckCombatInput();
        CheckNavInput();
        CheckRaycastInput();
    }

    void CheckCombatInput()
    {
        if (NetworkManager.Instance.IsOfflineMode)
            return;

        if (Input.GetButtonDown("Fire"))
        {
            combatComponent.NormalAttack();
        }
        else if (Input.GetKey(KeyCode.Q))
        {
            combatComponent.SkillAttack(0);
        }
    }

    void CheckNavInput()
    {
        if (Input.GetMouseButtonDown(1))
        {
            var ray = Camera.main.ScreenPointToRay(Input.mousePosition);
            RaycastHit hit;
            if (Physics.Raycast(ray, out hit))
            {
                Debug.Log($"hit pos: {hit.point}");
                // _agent.destination = hit.point;

                SpaceService.QueryPath queryPath = new SpaceService.QueryPath
                {
                    StartPos = new SpaceService.Vector3f { X = transform.position.x, Y = transform.position.y, Z = transform.position.z },
                    EndPos = new SpaceService.Vector3f { X = hit.point.x, Y = hit.point.y, Z = hit.point.z }
                };
                NetworkManager.Instance.Send("query_path", queryPath.ToByteArray());
            }
        }
    }

    void CheckRaycastInput()
    {
        if (Input.GetMouseButtonDown(1))
        {
            float distance = 50;
            var ray = Camera.main.ScreenPointToRay(Input.mousePosition);
            RaycastHit hit;
            if (Physics.Raycast(ray, out hit, distance))
            {
                Debug.Log($"client hit pos: {hit.point}");
                SpaceService.QueryRaycast queryRaycast = new SpaceService.QueryRaycast
                {
                    StartPos = new SpaceService.Vector3f { X = ray.origin.x, Y = ray.origin.y, Z = ray.origin.z },
                    Dir = new SpaceService.Vector3f { X = ray.direction.x, Y = ray.direction.y, Z = ray.direction.z },
                    Distance = distance,
                };
                NetworkManager.Instance.Send("query_raycast", queryRaycast.ToByteArray());
            }
        }
    }

    // 我们想应用root motion，但默认的animator move会叠加重力的处理，但显然现在重力是由CharacterMovement接管的，需要忽略。
    private void OnAnimatorMove()
    {
        if (!_anim) return;
        transform.position += _anim.deltaPosition;
        transform.Rotate(_anim.deltaRotation.eulerAngles);
    }
}

