using Cinemachine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class MainCamera : MonoBehaviour
{
    private bool followSetupFinished = false;

    // Start is called before the first frame update
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {
        if (!followSetupFinished)
        {
            if (NetworkManager.Instance.MainPlayer != null)
            {
                CinemachineBrain brain = GetComponent<CinemachineBrain>();
                CharacterMovement characterMovement = NetworkManager.Instance.MainPlayer.GetComponent<CharacterMovement>();
                brain.ActiveVirtualCamera.Follow = characterMovement.CinemachineCameraTarget.transform;

                followSetupFinished = true;
            }
        }
    }
}
