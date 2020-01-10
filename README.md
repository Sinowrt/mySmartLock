# mySmartLock
## 初步部署
#### 2018.1.27
* 舵机安装，用热熔胶将SG92R 9g小型舵机粘到门锁旁，舵机牵扯尼龙绳实现开锁动作
* 利用原有的papersignals固件对舵机开门的可行性进行测试，发现舵机在门锁有阻力的情况下，在舵机通电一刻仍能高效开锁
* 对该项目的架构进行构思

### **1.借鉴papersignals项目：**   
* esp8266充当客户端，定时请求服务端获取json数据  
* 手机端发送请求到达服务端，服务端解析请求并修改本地json数据  
* 从而达到伪装实时开锁的目的



## 工作与过年原因项目搁置许久 ##

### **2.构思内网穿透方案**  
* 利用pandorabox的可扩展性，安装内网穿透客户端ngrok  
* 在搬瓦工vps上安装ngrokd服务端  
* 抛弃papersignals的类似方式，将esp作为服务端，手机作为客户端
* 鉴于小程序的跨平台特性，决定采用小程序的方式进行手机端开发

### 在搬瓦工vps搭建ngrok服务端的过程 ###
* 参考[这篇博客](https://www.leocode.net/article/index/19.html)在vps上进行ngrok的编译  
* 该博客中遇到的问题，我全都遇到了，按照相应的步骤操作即可解决
* 另外在go1.4安装操作步骤混乱的时候，造成goroot路径的错乱，从而无法安装上go1.4和go1.6，只需要unset GOROOT即可
* 一切安装就绪，卡在ngrok的go编译上，在编译过程中报错：go build project/test: signal: killed 查阅[资料](https://segmentfault.com/q/1010000000486445)，发现是vps内存太小导致的无法编译 
* 改用centos进行编译，但是只有visualbox的centos镜像，而visualbox复制文件上面完全比不上vmware方便，所以改用ubuntu进行ngrok编译。
* 在ubuntu中部署ngrok编译环境的时候，使用yum命令，发现yum是centos专用的安装命令，在ubuntu上使用会导致很多问题的发生，例如yum的源不知如何配置，所以用回apt命令
* 由于无法使用yum，造成很多依赖包无法安装上，所以无法安装上官网下载的go1.10包
* 查找资料发现可以使用sudo apt-get install go-lang命令直接安装go环境，若找不到直接apt update更新源即可
* 开启sslocal，把ngrok项目从git中拉下来
* 对ngrok项目进行编译
* 其中最重要的是$NGROK_DOMAIN的设置，在该教程中直接关系到openssl证书的生成
* 编译出ngrokd服务端
* 丢进vps中跑一下
* 问题：80端口和443端口的占用
* 解决：启动ngrok命令中修改-httpAddr以及httpsAddr 以更改监听端口，4443为透传接入端口，更改之后直接启动即可，在浏览器中输入www.sinowrt.cn:81 出现trunnel...的即表示启动成功
* 编译出windows的客户端，windows的客户端必须要在cmd中才能运行，新建ngrok.cfg,将内容

```
server_addr: "www.sinowrt.cn:4443"
trust_host_root_certs: false
```
写入该文件中，在cmd进入该目录，敲入命令
```
ngrok.exe -config="ngrok.cfg" -subdomain="abc" 80
```
连接上ngrok服务端即可将本地的80端口暴露出外网
* 问题：一开始死活连接不上，出现fail
* 原因：abc.www.sinowrt.cn没有配置解析，dns无法解析该url所以造成无法连接上服务器，在腾讯云中添加*.www的泛解析，问题解决
* 问题：出现reconnecting
* 原因：server_addr后面的内容必须跟证书生成过程中的地址完全匹配，重新填写该参数或重新生成证书而重新编译即可解决问题
* 至此，服务器端部署告一段落
直接敲入命令
* 以上部署在应用到小程序中会有坑，请看小程序中的分析

* 最终版的命令
```
./ngrokd -domain=ngrok.sinowrt.cn -httpAddr=:80 -httpsAddr=:443 -tlsCrt /etc/ngrok_ssl/server.crt -tlsKey /etc/ngrok_ssl/server.key&
```

即可启动ngrok服务端

### 小程序“我只是个开门的玩意儿”开发的过程 ###
* 小程序账号的申请
* 基本信息的填写
* 开发工具的下载
* 根据原有的框架，对bindViewTap函数进行改写，点击即调用wx.request方法发起post请求
* 由于wx.request会对url进行安全校验，根据网上资料指示，在开发工具中关闭校验
* 问题：发现wx.request不能加端口号
* 解决: 修改vps的apache监听端口为81，ngrokd的监听端口为80
* 添加toast对开门过程进行文字弹幕展示  

**以上部署在应用到小程序中会有坑，请看后续分析**
* 2018.3.2进行了体验版本的发布，回来想试用一下app进行开门，发现竟然无响应
* 无意中开启调试模式，竟然又可以了（此事用的是http协议）
* 查阅网上资料，发现要进行合法域名配置，于是配置合法域名为abc.ngrok.sinowrt.cn（发现一个问题，域名的前缀为https，且无法更改）
* 发现仍是在开发工具中成功运行，而app中始终是调试模式才能成功
* 继续查阅资料，发现只有调试模式才没有进行域名校验
* 查看vps中的443端口占用，ssserver占用了，对sserver进行kill -9 pid操作，再用其他端口重新开启，再重新开启ngrok服务，https连接没问题
* 再修改客户端中的连接协议为https
* 开启开发工具和域名合法检测，发现始终无法请求成功（重启ngrokd的时候发现有配置服务器证书的参数）
* 在腾讯云上为ngrok.sinowrt.cn申请免费的ssl证书,审核成功后，下载，放入vps，指定ngrokd的证书为/etc/ngrok_ssl/server.csr与/etc/ngrok_ssl/server.key
* 发现请求abc.ngrok.sinowrt.cn仍然为不安全，而ngrok.sinowrt.cn却是安全的了
* 于是再为abc.ngrok.sinowrt.cn申请ssl证书，
* 为了减少错误的发生，将ngrok.sinowrt.cn的证书放进ubuntu的ngrok编译环境中，重新编译出ngrokd扔进去vps中
* 按上述的方法指定该证书，重启ngrokd，成功
* 但是小程序似乎仍然毫无反应，而改回http之后才有反应，判断应该是请求的时候被拦截了
* 查看服务器端的tls支持情况，发现服务器端是支持tls1.2以下版本的
* 再查阅[资料1](https://www.jianshu.com/p/082ba2965a88)以及[资料2](http://blog.csdn.net/hgg923/article/details/73897011),按照相应步骤配置中间证书后，小程序请求问题终于解决了，一切正常使用
* 在使用过程中，点下按钮由于服务端会有延时操作，所以过了几秒才有开锁成功，所以添加“正在开锁”的toast

### esp8266服务端的搭建 ###
* 参考网上[资料](http://www.yfrobot.com/thread-11874-1-1.html)
* 将如下源码进行改写

```
/*
    This sketch demonstrates how to set up a simple HTTP-like server.
    The server will set a GPIO pin depending on the request
      http://server_ip/gpio/0 will set the GPIO2 low,
      http://server_ip/gpio/1 will set the GPIO2 high
    server_ip is the IP address of the ESP8266 module, will be
    printed to Serial when the module is connected.
*/
 
#include <ESP8266WiFi.h>
 
#define LED 2
 
const char* ssid = "YFROBOT";
const char* password = "yfrobot2016";
 
// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
 
void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  delay(10);
 
  // prepare GPIO2
  pinMode(LED, OUTPUT);
 
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.println(WiFi.localIP());
 
  digitalWrite(LED, 1); // LOW 
}
 
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  delay(10);
 
  // Match the request
  int val = 1;
  if (req.indexOf("/gpio/0") != -1)
    val = 1;
  else if (req.indexOf("/gpio/1") != -1)
    val = 0;
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }
 
  // Set GPIO2 according to the request
  digitalWrite(LED, val);
 
  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
  s += (val) ? "low" : "high";
  s += "</html>\n";
  // Send the response to the client
  client.print(s);
  delay(1);
   
  Serial.println("Client disonnected");
  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}
```
* 添加servo控制，以及相应的请求解析即可完成舵机的控制
* 将改写好的ino文件烧录进esp8266即可
* 烧写接线与正常运行接线区别只在于gpio0  
gpio15 -> -  
ch_en  -> +  
vcc -> +  
gnd ->-  
烧录：gpio -> -
运行：gpio -> + 或 置空+


### pandorabox的ngrok客户端部署 ###
* 查阅资料发现openwrt上的ngrok客户端为c编写的ngrokc
* ngrokc的github地址：https://github.com/dosgo/ngrok-c
* 无意中又发现python版本的ngrok：https://github.com/hauntek/python-ngrok
* 正好我的路由配置了python环境，为了省事，直接用python版本的
* 将ngrok客户端的py文件扔进/usr/bin中，运行发现缺少ssl.py模块，安装完整版的python以及openssl也无法解决，最后配置源，安装了python-openssl问题才解决
* python版本的ngrok客户端由于我在windows中已经配置好它的配置文件，放到路由中直接运行即可，下面贴出配置文件：
```
{
    "server": {
        "host": "ngrok.sinowrt.cn",
        "port": 4443,
        "bufsize": 1024
    },
    "client": [
        {
            "protocol": "http",
            "hostname": "",
            "subdomain": "abc",
            "rport": 0,
            "lhost": "127.0.0.1",
            "lport": 8080
        }
    ]
}
```
直接运行命令：
```
python ./ngrok /etc/config/ngrok.config&
```
即可启动客户端
* 以上部署在应用小程序的时候会有坑，请看小程序中的分析
* 最终版的ngrok.config:
```
{
    "server": {
        "host": "ngrok.sinowrt.cn",
        "port": 4443,
        "bufsize": 1024
    },
    "client": [
        {
            "protocol": "https",
            "hostname": "",
            "subdomain": "abc",
            "rport": 0,
            "lhost": "192.168.1.121",
            "lport": 80
        }
    ]
}
```
## [StartSSL](https://segmentfault.com/a/1190000007547179)

# 2018/3/7
* 晚上发现舵机扫齿了，果然，塑料齿轮还是脆了

# 2018/3/8
* 早上淘了一个金属齿轮的国产舵机MG945，看了店家描述,总结下
* MG996和MG995优点在于响应快，扭力相对小
* MG945优点在于扭力大，相应相对较慢

# 2018/3/10
* 下午拿了舵机装上直接点亮
* 修复发现很久的bug
* 由于舵机初始化时currentposition角度为unlockposition 90，开机时由于`MoveServoToPosition(RESETPOSITION, 10);`语句，舵机会先转到90度再转到180度
* 更改为初始化currentposition角度为RESTPOSTION
* 更改unlockposition为0
* 更改resetposition为90

## 2018/3/13 ##
* 星期二晚上到货
* 取开发资料
* 浏览用户手册

## 2018/3/14 ##
* 根据用户手册进行模块接线
* 接入串口，利用开发包中的上位机对指纹模块进行操作
* 用串口调试软件，输入相应的指令，进行指纹采集等操作，但进行相应的操作总是无法实现
* 原因：对原有指令进行了改造，而校验码并没有改变
* 对上位机软件进行改造，使其输出相应操作流程的指令 
#### 在OEMHostDlg.cpp中查找对应的按钮执行的内容
* 1.找到void COEMHostDlg::OnBtnVerify()
0
```
void COEMHostDlg::OnBtnVerify() 
{
	int		w_nRet, w_nTmplNo, w_nFpStatus, w_nLearned;
	DWORD	w_dwTime;
	CString	w_strTmp;

	UpdateData(TRUE);

	if (!CheckUserID())
		return;

	EnableControl(FALSE);
	GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_STOP)->EnableWindow(TRUE);
	
	m_bCancel = FALSE;

	w_nTmplNo = m_nUserID;

	//. Check if fp is exist
	w_nRet = m_clsCommu.Run_GetStatus(w_nTmplNo, &w_nFpStatus);
	
	if( w_nRet != ERR_SUCCESS )
	{
		m_strCmdResult = GetErrorMsg(w_nRet);
		goto l_exit;
	}
	
	if( w_nRet == ERR_SUCCESS && w_nFpStatus == GD_TEMPLATE_EMPTY )
	{
		m_strCmdResult = _T("Template is empty");
		goto l_exit;
	}
	
	m_clsCommu.Run_SLEDControl(1);

	m_strCmdResult = _T("Input your finger.");
	UpdateData(FALSE);
	
	w_dwTime = GetTickCount();
	
	//. Get Image
	while (1)
	{
		DoEvents();

		if(m_bCancel)
		{
			m_strCmdResult = _T("Operation Canceled");
			goto l_exit;
		}

		//. Get Image
		w_nRet = m_clsCommu.Run_GetImage();
		
		if(w_nRet == ERR_SUCCESS)
			break;
		else if(w_nRet == ERR_CONNECTION)
		{
			m_strCmdResult = GetErrorMsg(w_nRet);
			goto l_exit;
		}					
	}
	
	m_strCmdResult = "Release your finger";
	UpdateData(FALSE);
	DoEvents();

	//. Up Image
	if (m_chkShowImage.GetCheck())
	{
		m_strCmdResult = _T("Uploading image...");
		UpdateData(FALSE);
		DoEvents();

		w_nRet = m_clsCommu.Run_UpImage(0, g_FpImageBuf, &g_nImageWidth, &g_nImageHeight);

		if(w_nRet != ERR_SUCCESS)
		{
			m_strCmdResult = GetErrorMsg(w_nRet);
			goto l_exit;
		}	

		m_wndFinger.SetImage(g_FpImageBuf, g_nImageWidth, g_nImageHeight);

		m_strCmdResult = _T("Uploading image is successed.");
		UpdateData(FALSE);
		DoEvents();
	}

	w_dwTime = GetTickCount();

	//. Create Template
	w_nRet = m_clsCommu.Run_Generate(0);
	
	if (w_nRet != ERR_SUCCESS)
	{
		m_strCmdResult = GetErrorMsg(w_nRet);
		goto l_exit;
	}

	//. Verify 
	w_nRet = m_clsCommu.Run_Verify(w_nTmplNo, 0, &w_nLearned);
	
	w_dwTime = GetTickCount() - w_dwTime;

	if (w_nRet == ERR_SUCCESS)
	{
		m_strCmdResult.Format(_T("Result : Success\r\nTemplate No : %d, Learn Result : %d\r\nMatch Time : %dms"), w_nTmplNo, w_nLearned, w_dwTime );
	}
	else
	{
		m_strCmdResult = GetErrorMsg(w_nRet);
	}
	
l_exit:
	m_clsCommu.Run_SLEDControl(0);
	UpdateData(FALSE);	
	EnableControl(TRUE);
	GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(TRUE);
}
```
**2.查看m_clsCommu.Run_SLEDControl(0)，跳转到Communication.cpp**

```
int	CCommunication::Run_SLEDControl(int p_nState)
{
	BOOL	w_bRet;
	BYTE	w_abyData[2];
	
	w_abyData[0] = LOBYTE(p_nState);
	w_abyData[1] = HIBYTE(p_nState);
	
	InitCmdPacket(CMD_SLED_CTRL, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_abyData, 2);
	
	SEND_COMMAND(CMD_SLED_CTRL, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
```
3.查看SEND_COMMAND定义，跳转到Command.cpp

```
BOOL SendCommand(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nSendCnt = 0;
	LONG	w_nResult = 0;

	g_Serial.Purge();	

	::SendMessage(g_hMainWnd, WM_CMD_PACKET_HOOK, 0, 0);

	w_nResult = g_Serial.Write(g_Packet, g_dwPacketSize , &w_nSendCnt, NULL, COMM_TIMEOUT);

	if(ERROR_SUCCESS != w_nResult)
	{
		return FALSE;
	}

	return ReceiveAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID);
}
```
4.可以看到串口输出函数g_Serial.Write,g_packet应该为输出的内容，现在要做的就是将g_packet打印出来
5.顺势根据`return ReceiveAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID)`语句找到ReceiveAck函数，分析可知发送了消息通过ReceiveAck函数接收响应内容，则在此也添加Print_gPacket(false)以打印接收的内容

```
BOOL ReceiveAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	Print_gPacket(true);
	DWORD	w_nAckCnt = 0;
	LONG	w_nResult = 0;
	DWORD	w_dwTimeOut = COMM_TIMEOUT;

	if (p_wCMDCode == CMD_TEST_CONNECTION)
		w_dwTimeOut = 2000;
	else if (p_wCMDCode == CMD_ADJUST_SENSOR)
		w_dwTimeOut = 30000;
	
l_read_packet:

	//w_nResult = g_Serial.Read(g_Packet, sizeof(ST_RCM_PACKET), &w_nAckCnt, NULL, w_dwTimeOut);	
	if (!ReadDataN(g_Packet, sizeof(ST_RCM_PACKET), w_dwTimeOut))
	{
		return FALSE;
	}

	g_dwPacketSize = sizeof(ST_RCM_PACKET);	

	::SendMessage(g_hMainWnd, WM_RCM_PACKET_HOOK, 0, 0);

	if (!CheckReceive(g_Packet, sizeof(ST_RCM_PACKET), RCM_PREFIX_CODE, p_wCMDCode))
		return FALSE;
	
	if (g_pCmdPacket->m_byDstDeviceID != p_bySrcDeviceID)
	{
		goto l_read_packet;
	}

	Print_gPacket(false);
	return TRUE;
}
```


6.由于该软件为mfc程序，所以在command添加如下代码，在输出的debug内容中显示相应的内容

```
#include "windows.h"
#ifdef _DEBUG    

#define DP0(fmt) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt));OutputDebugString(sOut);}    
#define DP1(fmt,var) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt),var);OutputDebugString(sOut);}    
#define DP2(fmt,var1,var2) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt),var1,var2);OutputDebugString(sOut);}    
#define DP3(fmt,var1,var2,var3) {TCHAR sOut[256];_stprintf_s(sOut,_T(fmt),var1,var2,var3);OutputDebugString(sOut);}    

#endif
```
7.添加一个函数，用于打印g_packet,

```
/******************************
 create by Jacky  2018/3/14
 To print command
 type true  for send
      false for recive
*******************************/
void Print_gPacket(bool type) {
	if (type) {
		DP0("发送的数据：");
	}
	else {
		DP0("接收的数据：");
	}
	for (int i = 0; i < 26; i++) {
		DP1("%02x", g_Packet[i]);
	}
	DP0("\n");
}
```


* 总结出采集验证指纹的过程总共有以下几条指令

```
*****************************
*    指纹模块一次指纹比对 
*    host命令
*****************************
// 采集指纹
55aa000020000000000000000000000000000000000000001f01

// 模板生成到rambuffer0
55aa000060000200000000000000000000000000000000006101

// 1:N对比
55aa00006300060000000100f401000000000000000000005e02

// 关灯
55aa000024000200000000000000000000000000000000002501
```


## 2018/3/15 ##
* 由于esp8266与指纹模块通讯的过程中串口的数据交换并不可见，所以想到透传
* 上午上完课回来，进行透传程序Transmission的编写
```
#include <ESP8266WiFi.h>

const char *ssid = "sinowrt";
const char *password = "asdfghjkl";

const int tcpPort = 80;//要建立的TCP服务的端口号

WiFiServer server(tcpPort);


void setup()
{
	Serial.begin(115200);
	delay(10);
	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
	server.begin();
}


void loop()
{
	WiFiClient client = server.available();
	if (client) {
		while (client.connected()) {
			/*while (client.available()) {
			char c = client.read();
			Serial.write(c);
			}*/

			if(client.available()){
			size_t counti=client.available();
			uint8_t *buf=new uint8_t[counti];
			Serial.write((const uint8_t*)buf,counti);
			delete(buf);
			}

			delay(1000);   //由于指纹模块是首先发送响应头再发响应数据，若不加这句，响应数据会分两次读取

			if (Serial.available())
			{
				size_t counti = Serial.available();
				uint8_t *sbuf = new uint8_t[counti];
				Serial.readBytes(sbuf, counti);
				client.write((const uint8_t*)sbuf, counti);

				delete(sbuf);
			}
		}
		client.stop();
	}

}
```
* 注：
* 1.有坑，请看注释
* 2.遇到下列报错：

```
Compiling debug version of 'Transmission' for 'NodeMCU 1.0 (ESP-12E Module)'
 
Error compiling project sources
Debug build failed for project 'Transmission'
ESP8266WiFi.h:39: In file included from
Transmission.ino: from
WiFiClient.h: In instantiation of size_t WiFiClient::write(T&, size_t) [with T = unsigned char*; size_t = unsigned int]
Transmission.ino:60: required from here
 
WiFiClient.h: 123:36: error: request for member 'available' in 'source', which is of non-class type 'unsigned char*
   size_t left = source.available()
 
WiFiClient.h: 127:5: error: request for member 'read' in 'source', which is of non-class type 'unsigned char*
   source.read(buffer.get(), will_send)
```
原因是client的write函数必须传入一个buffer，但是通过网上查找解决方案，发现在第一个参数前加上(const uint8_t*)进行强制转换也可以编译通过

# 2018/3/16
#### 透传实现了，那么就开始改造原有的mySmartLock.ino，首先第一版的程序构思为：
循环
* 1.读取指纹模块的手指感应信号（有信号就逐条发送指纹验证指令）
* 2.读取网络端是否传来请求（有就判断请求url是否包含指定的字符串）
* 3.读取透传客户端是否已连接（若已连接且传入debug()就更改标志，（考虑到指令录指纹时会造成录指纹和验证指纹指令的冲突）停止1的指纹验证，并允许串口穿透输入与输出，若为exit()就修改标志，重新启用1的指纹验证，只输出串口通讯的信息）

### 第一版的编程实现，采用的串行处理模式
* 坑1：在判断串口传入数据是否为debug()或exit()的过程中，用到了strcmp()函数，但是貌似读取出来的unsigned char*类型的字节流是不带结束符的而"debug()"是带结束符的长度为8的char数组，所以一直判断不等，经过查阅资料，发现还有一个函数就是strncmp()可以进行一定长度字符串的比较，所以改用 `strncmp((const char*)command, "debug()", counti)`语句进行判断，经测试完全可行！！

#### 但是问题来了，在arduino的loop()函数中，是循环执行的，当透传客户端连接上之后，除非用`while (client.connected())`语句进行连接的保持，否则当前的transclient实例会在一次循环后被销毁，但是若保持连接又无法输出当前指纹模块与esp8266的通讯交互信息

### 发现还是程序的状态图没分清楚，构思总结出改进的三个状态
* 1.透传连接，并通过debug()进入debug模式
* 2.透传连接，保持默认状态，或在debug模式下通过exit()退出调试模式的，就只输出esp8266与指纹通讯的信息
* 3.串口没有连接，esp8266与指纹模块正常通讯

### 编程实现
* 1.将指纹验证和读取网络请求两部分封装成loopBody(bool modeSwitch)
* 2.用modeSwitch标记是否进入了debug模式，通过判断modeSwitch来判断是否进行透传输出，modeSwitch通过是否传入debug()和exit()命令进行切换
* 3.在transclient可用时进入while(client.connected())的循环，可以进行回话的保持，返回当前串口通讯的数据
* 4.总的源码如下所示

```
/*
This sketch demonstrates how to set up a simple HTTP-like server.
The server will set a GPIO pin depending on the request

http://server_ip/gpio/0 will set the GPIO2 low,
http://server_ip/gpio/1 will set the GPIO2 high
http://server_ip/mySmartLock/unlock will unlock the Smartlock

server_ip is the IP address of the ESP8266 module, will be
printed to Serial while the module is connected.
*/

#include <Servo.h>
#include <ESP8266WiFi.h>


#define LED 2                    //led output pin
#define MYSERVOPIN 14            //servo output pin
#define FINGERPRINTSIGNAL 13     //fingerprint module Touch Signal input pin
#define UNLOCKPOSITION 0         //the position of unlock
#define RESETPOSITION 90         //the position of reset
#define TCPPORT 80
#define TRANSMISSIONPORT 81

WiFiClient client;
WiFiClient transClient;
Servo servo;
int currentServoPosition = RESETPOSITION;
bool modeSwitch;

const char* ssid = "sinowrt";        //wifi name
const char* password = "asdfghjkl";  //wifi password
const unsigned char CMD_GET_IMAGE[26] = { 0x55,0xAA,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x01 };
const unsigned char CMD_GENERATE[26]  = { 0x55,0xaa,0x00,0x00,0x60,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x61,0x01 };
const unsigned char CMD_SEARCH[26]    = { 0x55,0xaa,0x00,0x00,0x63,0x00,0x06,0x00,0x00,0x00,0x01,0x00,0xf4,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5e,0x02 };
const unsigned char CMD_SLED_CTRL[26] = { 0x55,0xaa,0x00,0x00,0x24,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x25,0x01 };
// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(TCPPORT);
WiFiServer transServer(TRANSMISSIONPORT);

void setup() {
	WiFi.mode(WIFI_STA);
	Serial.begin(115200);
	delay(10);

	// prepare GPIO2 & GPIO13
	pinMode(LED, OUTPUT);
	pinMode(FINGERPRINTSIGNAL, INPUT);
	//prepare GPIO14
	servo.attach(MYSERVOPIN);

	// Connect to WiFi network
	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");

	// Start the server
	server.begin();
		Serial.println("TCP Server started at 80 port!");
	transServer.begin();
		Serial.println("Transmission Server started at 81 port!");

	// Print the IP address
	Serial.println(WiFi.localIP());

	digitalWrite(LED, false); // LOW 
	MoveServoToPosition(RESETPOSITION, 10);
	modeSwitch = false;
}

void loop() {
	Serial.flush();
	//Check if a transmission client has connected
	transClient = transServer.available();
	if (transClient) {
		while(transClient.connected()) {
			if (transClient.available()) {
				size_t counti = transClient.available();
				unsigned char* command = new unsigned char[counti];
				transClient.readBytes(command, counti);
				
				if (!strncmp((const char*)command, "debug()", counti)) {
					modeSwitch = true;
				}else if (!strncmp((const char*)command, "exit()", counti)) {
					modeSwitch = false;
				}else if(!modeSwitch) {
					Serial.write(command, counti);
				}	
			}

			delay(1000);

			if (Serial.available())
			{
			size_t counti = Serial.available();
			uint8_t *sbuf=new uint8_t[counti];
			Serial.readBytes(sbuf, counti);
			transClient.write((const uint8_t*)sbuf, counti);
			delete(sbuf);
			}

			loopBody(modeSwitch);
		}
		transClient.stop();
	}else {
		loopBody(modeSwitch);
	}
	
}

void loopBody(bool modeSwitch) {
	//Check if a user has touched the fingerprint module
	int status = digitalRead(FINGERPRINTSIGNAL);
	if (status&&!modeSwitch) {
		unsigned char* g_packet = new unsigned char[26];
		size_t size;
		Serial.write(CMD_GET_IMAGE, 26);
		delay(500);
		if (Serial.available()) {
			size = Serial.available();
			Serial.readBytes(g_packet, size);
				transClient.write((const char*)g_packet, size);

			if (g_packet[0] = 0xAA && g_packet[1] == 0x55 && g_packet[4] == 0x20 && g_packet[8] == 0x00 && g_packet[9] == 0x00) {
				memset(g_packet, 0, 26);
				Serial.write(CMD_GENERATE, 26);
				delay(500);
				if (Serial.available()) {
					size = Serial.available();
					Serial.readBytes(g_packet, size);
						transClient.write((const char*)g_packet, size);
					
					if (g_packet[0] = 0xAA && g_packet[1] == 0x55 && g_packet[4] == 0x60 && g_packet[8] == 0x00 && g_packet[9] == 0x00) {
						memset(g_packet, 0, 26);
						Serial.write(CMD_SEARCH, 26);
						delay(500);
						if (Serial.available()) {
							size = Serial.available();
							Serial.readBytes(g_packet, size);
								transClient.write((const char*)g_packet, size);

							if (g_packet[0] = 0xAA && g_packet[1] == 0x55 && g_packet[4] == 0x63 && g_packet[6] == 0x05 && g_packet[8] == 0x00 && g_packet[9] == 0x00) {
								unlock();
							}
						}
					}
				}
			}
			Serial.write(CMD_SLED_CTRL, 26);
			delay(500);
			if (size = Serial.available()) {
				Serial.readBytes(g_packet, size);
				transClient.write((const char*)g_packet, size);
			}
			
		}


	}

	// Check if a Network client has connected
	client = server.available();
	if (client) {
		// Read the first line of the request
		String req = client.readStringUntil('\r');

		client.flush();
		delay(10);

		// Match the request
		int val = 1;
		if (req.indexOf("/gpio/0") != -1)
			ledControl(LED, 1);
		else if (req.indexOf("/gpio/1") != -1)
			ledControl(LED, 0);
		else if (req.indexOf("/mySmartLock/unlock") != -1) {
			unlock();
		}
		else {
			//Serial.println("invalid request");
			client.stop();
			return;
		}

		Serial.println("Client disonnected");
		// The client will actually be disconnected
		// when the function returns and 'client' object is detroyed
	}
}

void ledControl(int ledPin,int val) {
	// Set GPIO2 according to the request
	digitalWrite(LED, val);

	// Prepare the response
	String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
	s += (val) ? "low" : "high";
	s += "</html>\n";
	// Send the response to the client
	client.print(s);
	delay(1);
}

void unlock() {
	MoveServoToPosition(UNLOCKPOSITION, 1);
	delay(3000);
	MoveServoToPosition(RESETPOSITION, 1);

	// Prepare the response
	String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
	s += "Unlock succeeded!";
	s += "</html>\n";

	// Send the response to the client
	client.print(s);
	delay(1);
}

void MoveServoToPosition(int position, int speed) {
	if (position < currentServoPosition)
	{
		for (int i = currentServoPosition; i > position; i--)
		{
			servo.write(i);
			delay(speed);
		}
	}
	else if (position > currentServoPosition)
	{
		for (int i = currentServoPosition; i < position; i++)
		{
			servo.write(i);
			delay(speed);
		}
	}
	currentServoPosition = position;
}

```

# 2018/3/17


