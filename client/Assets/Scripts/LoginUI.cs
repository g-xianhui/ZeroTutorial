using Google.Protobuf;
using SpaceService;
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
    public bool OfflineMode = false;

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
            string username = AccountText.text;
            if (username == string.Empty)
            {
                username = GetRandomString(5);
            }
            NetworkManager.Instance.login(username);
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
            if (OfflineMode)
            {
                NetworkManager.Instance.LoadOfflineScene();
            }
            else
            {
                string host = "127.0.0.1";
                int port = 1988;
                NetworkManager.Instance.Connect(host, port, OnConnectResult);
            }
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

    public string GetRandomString(int length)
    {
        byte[] b = new byte[4];
        new System.Security.Cryptography.RNGCryptoServiceProvider().GetBytes(b);
        System.Random r = new System.Random(BitConverter.ToInt32(b, 0));
        string s = null, str = "";
        str += "0123456789";
        str += "abcdefghijklmnopqrstuvwxyz";
        str += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        for (int i = 0; i < length; i++)
        {
            s += str.Substring(r.Next(0, str.Length - 1), 1);
        }
        return s;
    }
}
