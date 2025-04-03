using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class HeadUI : MonoBehaviour
{
    Slider heathSlider;
    Slider manaSlider;

    private void Awake()
    {
        Transform heathSliderTrans = transform.Find("HealthSlider");
        heathSlider = heathSliderTrans.GetComponent<Slider>();
        Transform manaSliderTrans = transform.Find("ManaSlider");
        manaSlider = manaSliderTrans.GetComponent<Slider>();
    }

    public void ShowAt(Vector3 worldPos)
    {
        Vector3 screenPos = Camera.main.WorldToScreenPoint(worldPos);
        if (screenPos.z > 0)
        {
            transform.position = screenPos;
            gameObject.SetActive(true);
        }
        else
        {
            gameObject.SetActive(false);
        }
    }

    public void UpdateHeathSlider(float value)
    {
        heathSlider.value = value;
    }

    public void UpdateManaSlider(float value)
    {
        manaSlider.value = value;
    }
}
