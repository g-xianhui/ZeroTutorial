using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class PerformanceDisplay : MonoBehaviour
{
    public Text fpsText;
    public Text pingText;

    private int frames = 0;
    private float deltaTime = 0;

    void Start()
    {
    }

    void Update()
    {
        frames++;
        deltaTime += Time.deltaTime;

        if (deltaTime >= 0.5f)
        {
            var fps = frames / deltaTime;
            frames = 0;
            deltaTime = 0;
            fpsText.text = $"fps: {Mathf.Ceil(fps)}";
        }

        pingText.text = $"ping: {Mathf.Ceil(NetworkManager.Instance.RTT * 1000)} ms";
    }
}
