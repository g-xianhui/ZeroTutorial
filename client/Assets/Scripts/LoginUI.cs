using ProtoBuf;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

public class LoginUI : MonoBehaviour
{
    public Text ServerAddressText;
    public Text AccountText;

    public void OnLoginResult(bool isSuccess)
    {
        if (isSuccess)
        {
            Debug.Log("on login success");
            SceneManager.LoadScene("DemoScene", LoadSceneMode.Single);
        }
        else
        {
            Debug.Log("on login failed");
        }
    }

    public void OnConnectResult(bool isSuccess)
    {
        if (isSuccess)
        {
            Debug.Log("connect to server successed");
            SpaceService.LoginRequest loginRequest = new SpaceService.LoginRequest();
            loginRequest.Username = AccountText.text;
            NetworkManager.Instance.Send("login", loginRequest);
        }
        else
        {
            Debug.Log("connect to server failed");
        }
    }

    public void OnLoginButtonClick()
    {
        string serverAddress = ServerAddressText.text;
        string account = AccountText.text;

        if (serverAddress == string.Empty || account == string.Empty)
        {
            Debug.Log("offline test");
            SceneManager.LoadScene("DemoScene", LoadSceneMode.Single);
        }
        else
        {
            Debug.Log($"online test, server: {serverAddress}, account: {account}");
            string[] arr = serverAddress.Split(':');
            int port;
            if (arr.Length != 2 || !Int32.TryParse(arr[1], out port))
            {
                Debug.Log($"invalid address {serverAddress}");
                return;
            }
            string host = arr[0];

            NetworkManager.Instance.Connect(host, port, OnConnectResult);
        }
    }
}
