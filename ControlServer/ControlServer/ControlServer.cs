using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using System.IO;
using System.Runtime.InteropServices;

namespace ControlServer
{
    public partial class ControlServer : Form
    {
        public ControlServer()
        {
            InitializeComponent();
            //关闭对文本框的非法线程操作检查
            TextBox.CheckForIllegalCrossThreadCalls = false;
        }
        //分别创建一个监听客户端的线程和套接字
        Thread threadWatch = null;
        Socket socketWatch = null;

        private void btnStartService_Click(object sender, EventArgs e)
        {
            //定义一个套接字用于监听客户端发来的信息  包含3个参数(IP4寻址协议,流式连接,TCP协议)
            socketWatch = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            //发送信息 需要1个IP地址和端口号
            //获取服务端IPv4地址
            IPAddress ipAddress = GetLocalIPv4Address();
            lblIP.Text = ipAddress.ToString();
            //给服务端赋予一个端口号
            int port = 8888;
            lblPort.Text = port.ToString();

            //将IP地址和端口号绑定到网络节点endpoint上 
            IPEndPoint endpoint = new IPEndPoint(ipAddress, port);
            //将负责监听的套接字绑定网络端点
            socketWatch.Bind(endpoint);
            //将套接字的监听队列长度设置为20
            socketWatch.Listen(20);
            //创建一个负责监听客户端的线程 
            threadWatch = new Thread(WatchConnecting);
            //将窗体线程设置为与后台同步
            threadWatch.IsBackground = true;
            //启动线程
            threadWatch.Start();
            txtMsg.AppendText("控制端已经启动,开始监听子端传来的信息!" + "\r\n");
            btnStartService.Enabled = false;
        }

        /// <summary>
        /// 获取本地IPv4地址
        /// </summary>
        /// <returns>本地IPv4地址</returns>
        public IPAddress GetLocalIPv4Address()
        {
            IPAddress localIPv4 = null;
            //获取本机所有的IP地址列表
            IPAddress[] ipAddressList = Dns.GetHostAddresses(Dns.GetHostName());
            foreach (IPAddress ipAddress in ipAddressList)
            {
                //判断是否是IPv4地址
                if (ipAddress.AddressFamily == AddressFamily.InterNetwork) //AddressFamily.InterNetwork表示IPv4 
                {
                    localIPv4 = ipAddress;
                }
                else
                    continue;
            }
            return localIPv4;
        }

        //用于保存所有通信客户端的Socket
        Dictionary<string, Socket> dicSocket = new Dictionary<string, Socket>();

        //创建与客户端建立连接的套接字
        Socket socConnection = null;
        string clientName = null; //创建访问客户端的名字
        IPAddress clientIP; //访问客户端的IP
        int clientPort; //访问客户端的端口号
        /// <summary>
        /// 持续不断监听客户端发来的请求, 用于不断获取客户端发送过来的连续数据信息
        /// </summary>
        private void WatchConnecting()
        {
            while (true)
            {
                try
                {
                    socConnection = socketWatch.Accept();
                }
                catch (Exception ex)
                {
                    txtMsg.AppendText(ex.Message); //提示套接字监听异常
                    break;
                }
                //获取访问客户端的IP
                clientIP = (socConnection.RemoteEndPoint as IPEndPoint).Address;
                //获取访问客户端的Port
                clientPort = (socConnection.RemoteEndPoint as IPEndPoint).Port;
                //创建访问客户端的唯一标识 由IP和端口号组成 
                clientName = "IP: " + clientIP +" Port: "+ clientPort;
                lstClients.Items.Add(clientName); //在客户端列表添加该访问客户端的唯一标识
                dicSocket.Add(clientName, socConnection); //将客户端名字和套接字添加到添加到数据字典中

                //创建通信线程 
                ParameterizedThreadStart pts = new ParameterizedThreadStart(ServerRecMsg);
                Thread thread = new Thread(pts);
                thread.IsBackground = true;
                //启动线程
                thread.Start(socConnection);
                txtMsg.AppendText("IP: " + clientIP + " Port: " + clientPort + " 的子端与您连接成功,现在你们可以开始通信了.\r\n");
            }
        }

        /// <summary>
        /// 发送普通信息到子端的方法
        /// </summary>
        /// <param name="sendMsg">发送的字符串信息</param>
        private void ServerSendMsg(string sendMsg)
        {
            sendMsg = txtSendMsg.Text.Trim();
            //将输入的字符串转换成 机器可以识别的字节数组
            byte[] arrSendMsg = Encoding.UTF8.GetBytes(sendMsg);
            //向客户端列表选中的客户端发送信息
            if (!string.IsNullOrEmpty(lstClients.Text.Trim())) 
            {
                //获得相应的套接字 并将字节数组信息发送出去
                dicSocket[lstClients.Text.Trim()].Send(arrSendMsg);
                //通过Socket的send方法将字节数组发送出去
                txtMsg.AppendText("您在 " + GetCurrentTime() + " 向 IP: " + clientIP + " Port: " + clientPort + " 的客户端发送了:\r\n" + sendMsg + "\r\n");
            }
            else //如果未选择任何客户端 则默认为群发信息
            {
                //遍历所有的客户端
                for (int i = 0; i < lstClients.Items.Count; i++)
                {
                    dicSocket[lstClients.Items[i].ToString()].Send(arrSendMsg);
                }
                txtMsg.AppendText("您在 " + GetCurrentTime() + " 群发了信息:\r\n" + sendMsg + " \r\n");
            }
        }

        /// <summary>
        /// 发送控制信息到子端的方法
        /// </summary>
        /// <param name="sendMsg">发送的字符串信息</param>
        private void ServerSendControl(string sendMsg)
        {
            string ScreenName = null; //创建访问演示端的名字（唯一标识）
            //获取演示端信息文本框输入的控制端IP和Port
            IPAddress ScreenIPAddress = IPAddress.Parse(txtIP.Text.Trim());
            int ScreenPort = int.Parse(txtPort.Text.Trim());
            //创建访问客户端的唯一标识 由IP和端口号组成 
            ScreenName = "IP: " + ScreenIPAddress + " Port: " + ScreenPort;

            //将输入的字符串转换成 机器可以识别的字节数组
            byte[] arrServerMsg = Encoding.UTF8.GetBytes(sendMsg);
            //实际发送的字节数组比实际输入的长度多1 用于存取标识符
            byte[] arrServerSendMsg = new byte[arrServerMsg.Length + 1];
            arrServerSendMsg[0] = 2;  //在索引为0的位置上添加一个标识符
            Buffer.BlockCopy(arrServerMsg, 0, arrServerSendMsg, 1, arrServerMsg.Length);
            //获得相应的套接字 并将字节数组信息发送出去
            dicSocket[ScreenName.ToString()].Send(arrServerSendMsg);     
        }

        string strSRecMsg = null;
        /// <summary>
        /// 接收客户端发来的信息
        /// </summary>
        /// <param name="socketClientPara">客户端套接字的委托对象</param>
        private void ServerRecMsg(object socketClientPara)
        {
            Socket socketClient = socketClientPara as Socket;
            while (true)
            {
                int length = 0;
                //创建一个接收用的内存缓冲区 大小为10M字节数组
                byte[] arrServerRecMsg = new byte[10 * 1024 * 1024];
                try
                {
                    //获取接收的数据,并存入内存缓冲区  返回一个字节数组的长度
                    length = socketClient.Receive(arrServerRecMsg);
                }
                catch (SocketException ex)
                {
                    txtMsg.AppendText("套接字异常消息:" + ex.Message + "\r\n");
                    //如果套接字出现异常 很可能是对方客户端已经断开连接
                    txtMsg.AppendText("IP: " + clientIP + " Port: " + clientPort+ " 的客户端已经与您断开连接\r\n");
                    dicSocket.Remove(clientName);//移除相应远程客户端的套接字 
                    lstClients.Items.Remove(clientName);//从客户端列表中移除该客户端 
                    break;
                }
                catch (Exception ex)
                {
                    txtMsg.AppendText("系统异常消息: " + ex.Message + "\r\n");
                    break;
                }
                //判断发送过来的数据是普通文字信息
                if (arrServerRecMsg[0] == 0) //0为文字信息
                {
                    //将字节数组 转换为人可以读懂的字符串
                    strSRecMsg = Encoding.UTF8.GetString(arrServerRecMsg, 1, length - 1);//真实有用的文本信息要比接收到的少1(标识符)
                    //将接收到的信息附加到文本框txtMsg上  
                    txtMsg.AppendText("IP: " + clientIP + " Port: " + clientPort + " 的子端在 " + GetCurrentTime() + " 向您发送了:\r\n" + strSRecMsg + "\r\n");
                }
                //如果发送过来的数据是文件
                if (arrServerRecMsg[0] == 1)
                {
                    //调用接受文件的方法
                    SaveFile(arrServerRecMsg, length - 1);//同样实际文件长度需要-1(减去标识符)
                }
                //判断发送过来的数据是控制信息
                if (arrServerRecMsg[0] == 2) //2为控制信息
                {
                    //将字节数组 转换为人可以读懂的字符串
                    strSRecMsg = Encoding.UTF8.GetString(arrServerRecMsg, 1, length - 1);//真实有用的文本信息要比接收到的少1(标识符)
                    handleRecControl(strSRecMsg);                
                }
            }
        }

        //将信息发送到到客户端
        private void btnSendMsg_Click(object sender, EventArgs e)
        {
            //调用发送信息的方法 并检查发送是否成功
            ServerSendMsg(txtSendMsg.Text);
            txtSendMsg.Clear();
        }

        //快捷键 Enter 发送信息
        private void txtSendMsg_KeyDown(object sender, KeyEventArgs e)
        {
            ////当光标位于输入文本框上的情况下 发送信息的热键为回车键Enter
            if (e.KeyCode == Keys.Enter)
            {
                //则调用 服务器向客户端发送信息的方法 
                ServerSendMsg(txtSendMsg.Text);
                txtSendMsg.Clear();
            }
        }

        /// <summary>
        /// 保存接收文件的方法 包含一个字节数组参数 和 文件的长度
        /// </summary>
        /// <param name="arrFile">字节数组参数</param>
        /// <param name="fileLength">文件的长度</param>
        private void SaveFile(byte[] arrFile, int fileLength)
        {
            SaveFileDialog sfDialog = new SaveFileDialog();
            //创建一个用于保存文件的对话框
            sfDialog = new SaveFileDialog();
            //获取文件名的后缀 比如文本文件后缀 .txt
            string fileNameSuffix = strSRecMsg.Substring(strSRecMsg.LastIndexOf("."));
            sfDialog.Filter = "(*" + fileNameSuffix + ")|*" + fileNameSuffix + ""; //文件类型
            sfDialog.FileName = strSRecMsg;  //文件名
            //如果点击了对话框中的保存文件按钮 
            if (sfDialog.ShowDialog(this) == DialogResult.OK)
            {
                string savePath = sfDialog.FileName; //获取文件的全路径
                //保存文件
                FileStream fs = new FileStream(savePath, FileMode.Create);
                //Write()方法需要传入3个参数,文件的字节数组,开始写入字节的索引位置以及字节数组的长度
                fs.Write(arrFile, 1, fileLength);
                string fName = savePath.Substring(savePath.LastIndexOf("\\") + 1); //文件名 不带路径
                string fPath = savePath.Substring(0, savePath.LastIndexOf("\\")); //文件路径 不带文件名
                txtMsg.AppendText("您在 " + GetCurrentTime() + "\r\n成功接收了IP: " + clientIP + " Port: " + clientPort + " 的客户端发来的文件" + fName + "\r\n保存路径为:" + fPath + "\r\n");
            }
        }

        /// <summary>
        /// 获取当前系统时间
        /// </summary>
        /// <returns>当前系统时间</returns>
        public DateTime GetCurrentTime()
        {
            DateTime currentTime = new DateTime();
            currentTime = DateTime.Now;
            return currentTime;
        }

        //关闭服务端
        private void btnExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        //取消客户端列表选中状态
        private void btnClearSelectedState_Click(object sender, EventArgs e)
        {
            lstClients.SelectedItem = null;
        }


        Boolean is_rec_control_start = false;
        Boolean is_send_control_start = false;
        private void btnRecControlStart_Click(object sender, EventArgs e)
        {
            is_rec_control_start = true;
            btnRecControlStart.Enabled = false;
            btnRecControlStop.Enabled = true;
            labelCondition.Text = "正在从子端接收控制信息";
        }
        private void btnRecControlStop_Click(object sender, EventArgs e)
        {
            is_rec_control_start = false;
            btnRecControlStart.Enabled = true;
            btnRecControlStop.Enabled = false;
            labelCondition.Text = "尚未从子端接收控制信息";
        }
        private void btnSendControlStart_Click(object sender, EventArgs e)
        {
            is_send_control_start = true;
            btnSendControlStart.Enabled = false;
            btnSendControlStop.Enabled = true;
            labelCondition2.Text = "正在从子端接收控制信息";
        }
        private void btnSendControlStop_Click(object sender, EventArgs e)
        {
            is_send_control_start = false;
            btnSendControlStart.Enabled = true;
            btnSendControlStop.Enabled = false;
            labelCondition2.Text = "尚未从子端接收控制信息";
        }

        Boolean[] is_double_order = new Boolean[6] { false, false, false, false, false, false };
        
        /// <summary>
        /// 处理接收到的控制信息
        /// </summary>
        /// <param name="Msg">控制信息格式的字符串</param>
        private void handleRecControl(string Msg)
        {
            if (is_rec_control_start == false)
            {
                return;
            }
            int[] pXY = new int[2];
            float x = 0;
            float y = 0;
            int[] keyControl = new int[2];

            if (Msg[0] == 'M' && Msg[1] == 'A'){
                pXY = getNumberfromString(Msg);
                txtMouse1.Clear();
                txtSendMouse1.Clear();
                txtMouse1.AppendText("X1 - " + pXY[0].ToString() + "  Y1 - " + pXY[1].ToString());
                if (is_send_control_start == false) return;
                x = pXY[0] * 1366 / 640;
                y = pXY[1] * 768 / 480;
                ServerSendControl("MA" + 'X' + x.ToString() + 'Y' + y.ToString() + "END");
                txtSendMouse1.AppendText("X1 - " + x.ToString() + "  Y1 - " + y.ToString());
                return;
            }
            if (Msg[0] == 'M' && Msg[1] == 'B'){
                pXY = getNumberfromString(Msg);
                txtMouse2.Clear();
                txtSendMouse2.Clear();
                txtMouse2.AppendText("X2 - " + pXY[0].ToString() + "  Y2 - " + pXY[1].ToString());
                if (is_send_control_start == false) return;
                x = pXY[0] *1366 / 640;
                y = pXY[1] *768 / 480;
                ServerSendControl("MB" + 'X' + x.ToString() + 'Y' + y.ToString() + "END");
                txtSendMouse2.AppendText("X2 - " + x.ToString() + "  Y2 - " + y.ToString());
                return;
            }
            if (Msg[0] == 'M' && Msg[1] == 'C')
            {
                pXY = getNumberfromString(Msg);
                txtMouse3.Clear();
                txtSendMouse3.Clear();
                txtMouse3.AppendText("X3 - " + pXY[0].ToString() + "  Y3 - " + pXY[1].ToString());
                if (is_send_control_start == false) return;
                x = pXY[0] *1366 / 640;
                y = pXY[1] *768 / 480;
                ServerSendControl("MC" + 'X' + x.ToString() + 'Y' + y.ToString() + "END");
                txtSendMouse3.AppendText("X3 - " + x.ToString() + "  Y3 - " + y.ToString());
                return;
            }
            if (Msg[0] == 'M' && Msg[1] == 'D')
            {
                pXY = getNumberfromString(Msg);
                txtMouse4.Clear();
                txtSendMouse4.Clear();
                txtMouse4.AppendText("X4 - " + pXY[0].ToString() + "  Y4 - " + pXY[1].ToString());
                if (is_send_control_start == false) return;
                x = pXY[0] *1366 / 640;
                y = pXY[1] *768 / 480;
                ServerSendControl("MD" + 'X' + x.ToString() + 'Y' + y.ToString() + "END");
                txtSendMouse4.AppendText("X4 - " + x.ToString() + "  Y4 - " + y.ToString());
                return;
            }
            if (Msg[0] == 'K' && Msg[1] == 'A'){
                keyControl = getNumberfromString2(Msg);
                txtKeyboard1.Clear();
                txtSendKeyboard1.Clear();
                if (keyControl[0] == 12){
                    txtKeyboard1.AppendText(" 单手 - 左旋 ");
                    if (is_send_control_start == false) return;
                    if (is_double_order[2] == true) 
                    {
                        is_double_order[0] = false;
                        is_double_order[1] = false;
                        is_double_order[3] = false;
                        is_double_order[4] = false;
                        is_double_order[5] = false;
                        return; 
                    }
                    ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                    is_double_order[2] = true;
                    txtSendKeyboard1.AppendText(" 单手 - 左旋 ");
                    return;
                }
                if (keyControl[0] == 15){
                    txtKeyboard1.AppendText(" 单手 - 缩放  半径 = " + keyControl[1].ToString());
                    if (is_send_control_start == false) return;
                    //if (is_double_order[5] == true)
                    //{
                        is_double_order[0] = false;
                        is_double_order[1] = false;
                        is_double_order[2] = false;
                        is_double_order[3] = false;
                        is_double_order[4] = false;
                        //return;
                    //}
                    ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                    //is_double_order[5] = true;
                    txtSendKeyboard1.AppendText(" 单手 - 缩放  半径 = " + keyControl[1].ToString());
                    return;
                }
                if (keyControl[0] == 14)
                {
                   /* 
                    if (is_send_control_start == false) return;
                    if (is_double_order[4] == true)
                    {
                        is_double_order[0] = false;
                        is_double_order[1] = false;
                        is_double_order[2] = false;
                        is_double_order[3] = false;
                        is_double_order[5] = false;
                        return;
                    }
                    if (keyControl[1] == 0)
                    {
                        txtKeyboard1.AppendText(" 单手 - 向左旋转");
                        if (is_send_control_start == false) return;
                        ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                        is_double_order[4] = true;
                        txtSendKeyboard1.AppendText(" 单手 - 向左旋转");
                        return;
                    }
                    if (keyControl[1] == 1)
                    {
                        txtKeyboard1.AppendText(" 单手 - 向右旋转");
                        if (is_send_control_start == false) return;
                        ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                        is_double_order[4] = true;
                        txtSendKeyboard1.AppendText(" 单手 - 向右旋转");
                        return;
                    }
                    if (keyControl[1] == 2)
                    {
                        txtKeyboard1.AppendText(" 单手 - 向上翻转");
                        if (is_send_control_start == false) return;
                        ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                        is_double_order[4] = true;
                        txtSendKeyboard1.AppendText(" 单手 - 向上翻转");
                        return;
                    }
                    if (keyControl[1] == 3)
                    {
                        txtKeyboard1.AppendText(" 单手 - 向下翻转");
                        if (is_send_control_start == false) return;
                        ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                        is_double_order[4] = true;
                        txtSendKeyboard1.AppendText(" 单手 - 向下翻转");
                        return;
                    }*/

                    txtKeyboard1.AppendText(" 单手 - 右旋");
                    if (is_send_control_start == false) return;
                    if (is_double_order[4] == true)
                    {
                        is_double_order[0] = false;
                        is_double_order[1] = false;
                        is_double_order[2] = false;
                        is_double_order[3] = false;
                        is_double_order[5] = false;
                        return;
                    }
                    ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                    is_double_order[4] = true;
                    txtSendKeyboard1.AppendText(" 单手 - 右旋");
                    return;
                }
                if (keyControl[0] == 13)
                {
                    txtKeyboard1.AppendText(" 单手 - 取消所有操作及使模型静止");
                    if (is_send_control_start == false) return;
                    if (is_double_order[3] == true)
                    {
                        is_double_order[0] = false;
                        is_double_order[1] = false;
                        is_double_order[2] = false;
                        is_double_order[4] = false;
                        is_double_order[5] = false;
                        return;
                    }
                    ServerSendControl("KA" + 'X' + keyControl[0].ToString() + 'Y' + keyControl[1].ToString() + "END");
                    is_double_order[3] = true;
                    txtSendKeyboard1.AppendText(" 单手 - 取消所有操作及使模型静止");
                    return;
                }
            }
        }

        /// <summary>
        /// 提取坐标控制信息
        /// </summary>
        /// <param name="str"></param>
        /// <returns></returns>
        private int[] getNumberfromString(String str)
        {
            int[] returnXY = new int[2];
            returnXY[0] = 0;
            returnXY[1] = 0;

            string XStr = string.Empty;
            string YStr = string.Empty;

            int i = 0;
            for (; str[i] != 'X'; i++)
            {
            }
            for (; str[i] != 'Y'; i++)
            {
                if (Char.IsNumber(str, i) == true)
                {
                    XStr += str.Substring(i, 1);
                }
            }
            for (; str[i] != 'E'; i++)
            {
                if (Char.IsNumber(str, i) == true)
                {
                    YStr += str.Substring(i, 1);
                }
            }
            //MessageBox.Show(XStr);
            //MessageBox.Show(YStr);
            for (int j = 0; j < XStr.Length; j++)
            {
                returnXY[0] = returnXY[0] * 10 + XStr[j] - '0';
            }
            for (int j = 0; j < YStr.Length; j++)
            {
                returnXY[1] = returnXY[1] * 10 + YStr[j] - '0';
            }
            //MessageBox.Show(returnXY[0].ToString());
            //MessageBox.Show(returnXY[1].ToString());
            return returnXY;
        }

        /// <summary>
        /// 提取键盘控制信息
        /// </summary>
        /// <param name="str"></param>
        /// <returns></returns>
        private int[] getNumberfromString2(String str)
        {
            int[] returnXY = new int[2];
            returnXY[0] = 0;
            returnXY[1] = 0;

            string XStr = string.Empty;
            string YStr = string.Empty;

            int i = 0;
            for (; str[i] != 'X'; i++)
            {
            }
            for (; str[i] != 'Y'; i++)
            {
                if (Char.IsNumber(str, i) == true)
                {
                    XStr += str.Substring(i, 1);
                }
            }
            for (; str[i] != 'E'; i++)
            {
                if (Char.IsNumber(str, i) == true)
                {
                    YStr += str.Substring(i, 1);
                }
            }
            //MessageBox.Show(XStr);
            //MessageBox.Show(YStr);
            for (int j = 0; j < XStr.Length; j++)
            {
                returnXY[0] = returnXY[0] * 10 + XStr[j] - '0';
            }
            for (int j = 0; j < YStr.Length; j++)
            {
                returnXY[1] = returnXY[1] * 10 + YStr[j] - '0';
            }
            //MessageBox.Show(returnXY[0].ToString());
            //MessageBox.Show(returnXY[1].ToString());
            return returnXY;
        }









/*        //创建鼠标监视线程
        Thread threadMouse = null;

        private void btnStartMouseWatch_Click(object sender, EventArgs e)
        {
            
            threadMouse = new Thread(WatchMouse);
            threadMouse.IsBackground = true;
            threadMouse.Start();
            txtMsg.AppendText("鼠标监视thread已经启动!" + "\r\n");
            btnStartMouseWatch.Enabled = false;
            btnCloseMouseWatch.Enabled = true;
        }

        private void WatchMouse()
        {
            while (true)
            {
                System.Threading.Thread.Sleep(400);
                Point mouse = MousePosition;
                txtMouse1.AppendText("您在 " + GetCurrentTime() + " 发送了鼠标控制信息:\r\n" + "X - " + mouse.X.ToString() + "  Y - " + mouse.Y.ToString() + "\r\n");
                txtSendMsg.Text = "M-" + "X" + mouse.X +"Y"+ mouse.Y;
                //调用发送信息的方法 并检查发送是否成功
                ServerSendMsg(txtSendMsg.Text);
                txtSendMsg.Text = "";
            }
        }

        private void btnCloseMouseWatch_Click(object sender, EventArgs e)
        {
            threadMouse.Abort();
            txtMsg.AppendText("鼠标监视thread已被终止!" + "\r\n");
            btnStartMouseWatch.Enabled = true;
            btnCloseMouseWatch.Enabled = false;
        }

        class Message
        {
            public const int USER = 0x0400;
            public const int WM_mouse = USER + 101;
            public const int WM_keyboard = USER + 102;
        }

        protected override void DefWndProc(ref System.Windows.Forms.Message m)
        {
            string message;
            switch (m.Msg)
            {
                case Message.WM_mouse://处理消息
                   // message = string.Format("收到鼠标消息！参数为：{0}, {1}", m.WParam, m.LParam);
                   // MessageBox.Show(message);
                    //txtSendMsg.Text = "M-" + "X" + m.WParam + "Y" + m.LParam;
                    txtMouse1.AppendText("您在 " + GetCurrentTime() + " 发送了鼠标控制信息:\r\n" + "X - " + m.WParam.ToString() + "  Y - " + m.LParam.ToString() + "\r\n");
                   break;
                case Message.WM_keyboard:
                    message = string.Format("收到键盘消息！参数为：{0}, {1}", m.WParam, m.LParam);
                    MessageBox.Show(message);
                    break;
                default:
                    base.DefWndProc(ref m);
                    break;
            }
        }

        [DllImport("user32.dll", EntryPoint = "FindWindow")]
        public extern static IntPtr FindWindow(string lpClassName, string lpWindowName);


        [DllImport("User32.dll", EntryPoint = "SendMessage")]
        private static extern int SendMessage(IntPtr hWnd, int msg, int wParam, int lParam);
             
        private void button1_Click(object sender, EventArgs e)
        { 
            IntPtr hwnd = FindWindow(null, "客户端");
            if (hwnd == IntPtr.Zero)
            {
                MessageBox.Show("没有找到控制端窗口");
            }
            SendMessage(hwnd, Message.WM_mouse, 0, 0);
            SendMessage(hwnd, Message.WM_keyboard, 300, 300);       
        }
*/      
  
    }
}
