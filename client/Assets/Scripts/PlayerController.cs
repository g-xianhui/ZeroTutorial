using UnityEngine;

public class PlayerController : MonoBehaviour
{
    CombatComponent combatComponent;
    Animator _anim;

    void Awake()
    {
    }

    // Start is called before the first frame update
    void Start()
    {
        Cursor.lockState = CursorLockMode.Locked;
        combatComponent = GetComponent<CombatComponent>();
        _anim = GetComponent<Animator>();
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKey(KeyCode.Escape))
            Cursor.lockState = CursorLockMode.None;
        else if (Input.GetKey(KeyCode.RightAlt))
            Cursor.lockState = CursorLockMode.Locked;

        CheckCombatInput();
    }

    void CheckCombatInput()
    {
        if (Input.GetButtonDown("Fire"))
        {
            combatComponent.NormalAttack();
        }
        else if (Input.GetKey(KeyCode.Q))
        {
            combatComponent.SkillAttack(0);
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

