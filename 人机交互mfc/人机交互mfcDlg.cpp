
// 人机交互mfcDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "人机交互mfc.h"
#include "人机交互mfcDlg.h"
#include "afxdialogex.h"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <errno.h>
#include "qivw.h"
#include "qisr.h"
#include "qtts.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "speech_recognizer.h"
#include <locale.h>


#ifdef _WIN64
#pragma comment(lib,"../libs/msc_x64.lib") //x64
#else
#pragma comment(lib,"../libs/msc.lib") //x86
#endif

#define	BUFFER_SIZE	4096
#define FRAME_LEN	640 
#define HINTS_SIZE  100

enum{
	EVT_START = 0,
	EVT_STOP,
	EVT_TOTAL
};

/* wav音频头部格式 */
typedef struct _wave_pcm_hdr
{
	char            riff[4];                // = "RIFF"
	int				size_8;                 // = FileSize - 8
	char            wave[4];                // = "WAVE"
	char            fmt[4];                 // = "fmt "
	int				fmt_size;				// = 下一个结构体的大小 : 16

	short int       format_tag;             // = PCM : 1
	short int       channels;               // = 通道数 : 1
	int				samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	int				avg_bytes_per_sec;      // = 每秒字节数 : samples_per_sec * bits_per_sample / 8
	short int       block_align;            // = 每采样点字节数 : wBitsPerSample / 8
	short int       bits_per_sample;        // = 量化比特数: 8 | 16

	char            data[4];                // = "data";
	int				data_size;              // = 纯数据长度 : FileSize - 44 
} wave_pcm_hdr;

/* 默认wav音频头部数据 */
wave_pcm_hdr default_wav_hdr = 
{
	{ 'R', 'I', 'F', 'F' },
	0,
	{'W', 'A', 'V', 'E'},
	{'f', 'm', 't', ' '},
	16,
	1,
	1,
	16000,
	32000,
	2,
	16,
	{'d', 'a', 't', 'a'},
	0  
};

HWAVEIN hWaveIn;  //输入设备  
WAVEFORMATEX waveform; //采集音频的格式，结构体  
BYTE *pBuffer1;//采集音频时的数据缓存  
WAVEHDR wHdr1; //采集音频时包含数据缓存的结构体  
FILE *pf;  
#define IVW_AUDIO_FILE_NAME "audio/awaketry.pcm"
#define FRAME_LEN	640 //16k采样率的16bit音频，一帧的大小为640B, 时长20ms
bool isawake=false;
bool flagE=false;
CPictureEx m_PicWav;

static HANDLE events[EVT_TOTAL] = {NULL,NULL};

static COORD begin_pos = {0, 0};
static COORD last_pos = {0, 0};

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
using namespace std;



// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// C人机交互mfcDlg 对话框



C人机交互mfcDlg::C人机交互mfcDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(C人机交互mfcDlg::IDD, pParent)
	, m_STTstr(_T(""))
	, m_Speed(50)
	, m_Volume(50)
	, m_TTSstr(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hAccel=::LoadAccelerators(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_ACCELERATOR1));
}

void C人机交互mfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_LANGUAGE, m_LanguageCom);
	DDX_Text(pDX, IDC_EDIT1, m_STTstr);
	DDX_Control(pDX, IDC_RADIO1, m_ButtonSTT);
	DDX_Text(pDX, IDC_EDIT7, m_Speed);
	DDX_Text(pDX, IDC_EDIT8, m_Volume);
	DDX_Control(pDX, IDC_SLIDER1, m_SliderSpeed);
	DDX_Control(pDX, IDC_SLIDER2, m_SliderVolume);
	DDX_Control(pDX, IDC_COMBO_SEX, m_SexaCom);
	DDX_Text(pDX, IDC_EDIT2, m_TTSstr);
	DDX_Control(pDX, IDC_PICWAV, m_PicWav);
}

BEGIN_MESSAGE_MAP(C人机交互mfcDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &C人机交互mfcDlg::Onwavtra)
	ON_BN_CLICKED(IDC_BUTTON3, &C人机交互mfcDlg::OnSTTStart)
	ON_MESSAGE(1000, &C人机交互mfcDlg::OnSTTShow)
	ON_BN_CLICKED(IDC_BUTTON4, &C人机交互mfcDlg::OnSTTEnd)
	ON_BN_CLICKED(IDC_RADIO1, &C人机交互mfcDlg::OnBnClickedRadioMic)
	ON_BN_CLICKED(IDC_RADIO2, &C人机交互mfcDlg::OnBnClickedRadioWav)
	ON_BN_CLICKED(IDC_BUTTON_SAVESTT, &C人机交互mfcDlg::OnBnClickedButtonSaveSTT)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BUTTON_TTS_START, &C人机交互mfcDlg::OnBnClickedButtonTtsStart)
	ON_BN_CLICKED(IDC_BUTTON_TTS_STOP, &C人机交互mfcDlg::OnBnClickedButtonTtsStop)
	ON_BN_CLICKED(IDC_BUTTON_TTS_CONTINUE, &C人机交互mfcDlg::OnBnClickedButtonTtsContinue)
	ON_BN_CLICKED(IDC_BUTTON_TTS_QUIT, &C人机交互mfcDlg::OnBnClickedButtonTtsQuit)
	ON_BN_CLICKED(IDC_BUTTON_OPENTXT, &C人机交互mfcDlg::OnBnClickedButtonOpentxt)
	ON_BN_CLICKED(IDC_BUTTON_SAVETTS, &C人机交互mfcDlg::OnBnClickedButtonSavetts)
	ON_COMMAND(ID_ACCELERATOR_E, &C人机交互mfcDlg::OnAcceleratorE)
END_MESSAGE_MAP()


// C人机交互mfcDlg 消息处理程序
void sleep_ms(int ms)
{
	Sleep(ms);
}

int cb_ivw_msg_proc( const char *sessionID, int msg, int param1, int param2, const void *info, void *userData )
{
	if (MSP_IVW_MSG_ERROR == msg) //唤醒出错消息
	{
		AfxMessageBox(_T("MSP_IVW_MSG_ERROR"));
	}
	else if (MSP_IVW_MSG_WAKEUP == msg) //唤醒成功消息
	{
		isawake=true;
	}
	return 0;
}

void run_ivw(const char *grammar_list, const char* audio_filename ,  const char* session_begin_params)
{
	int         ret       = MSP_SUCCESS;
	const char *lgi_param = "appid = 5dac08ae, work_dir = .";
	//重置登陆
	ret = MSPLogin(NULL, NULL, lgi_param); 


	const char *session_id = NULL;
	int err_code = MSP_SUCCESS;
	FILE *f_aud = NULL;
	long audio_size = 0;
	long real_read = 0;
	long audio_count = 0;
	int count = 0;
	int audio_stat = MSP_AUDIO_SAMPLE_CONTINUE;
	char *audio_buffer=NULL;
	char sse_hints[128];
	if (NULL == audio_filename)
	{
		AfxMessageBox(_T("params error"));
		return;
	}

	f_aud=fopen(audio_filename, "rb");
	if (NULL == f_aud)
	{
		AfxMessageBox(_T("audio file open failed"));
		return;
	}
	fseek(f_aud, 0, SEEK_END);
	audio_size = ftell(f_aud);
	fseek(f_aud, 0, SEEK_SET);
	audio_buffer = (char *)malloc(audio_size);
	if (NULL == audio_buffer)
	{
		AfxMessageBox(_T("malloc failed"));
		goto exit;
	}
	real_read = fread((void *)audio_buffer, 1, audio_size, f_aud);
	if (real_read != audio_size)
	{
		AfxMessageBox(_T("read audio file failed"));
		goto exit;
	}

	session_id=QIVWSessionBegin(grammar_list, session_begin_params, &err_code);
	if (err_code != MSP_SUCCESS)
	{
		goto exit;
	}

	err_code = QIVWRegisterNotify(session_id, cb_ivw_msg_proc,NULL);
	if (err_code != MSP_SUCCESS)
	{
		_snprintf(sse_hints, sizeof(sse_hints), "QIVWRegisterNotify errorCode=%d", err_code);
		goto exit;
	}
	while(1)
	{
		long len = 10*FRAME_LEN; //16k音频，10帧 （时长200ms）
		audio_stat = MSP_AUDIO_SAMPLE_CONTINUE;
		if(audio_size <= len)
		{
			len = audio_size;
			audio_stat = MSP_AUDIO_SAMPLE_LAST; //最后一块
		}
		if (0 == audio_count)
		{
			audio_stat = MSP_AUDIO_SAMPLE_FIRST;
		}

		err_code = QIVWAudioWrite(session_id, (const void *)&audio_buffer[audio_count], len, audio_stat);
		if (MSP_SUCCESS != err_code)
		{
			AfxMessageBox(_T("QIVWAudioWrite failed"));
			_snprintf(sse_hints, sizeof(sse_hints), "QIVWAudioWrite errorCode=%d", err_code);
			goto exit;
		}
		if (MSP_AUDIO_SAMPLE_LAST == audio_stat)
		{
			break;
		}
		audio_count += len;
		audio_size -= len;

		sleep_ms(200); //模拟人说话时间间隙，10帧的音频时长为200ms
	}
	_snprintf(sse_hints, sizeof(sse_hints), "success");
	MSPLogout(); 
exit:
	if (NULL != session_id)
	{
		QIVWSessionEnd(session_id, sse_hints);
	}
	if (NULL != f_aud)
	{
		fclose(f_aud);
	}
	if (NULL != audio_buffer)
	{
		free(audio_buffer);
	}
}

DWORD WINAPI xiancheng2(LPVOID lpParameter){
	HANDLE          wait;  
    waveform.wFormatTag = WAVE_FORMAT_PCM;//声音格式为PCM  
    waveform.nSamplesPerSec = 16000;//采样率，16000次/秒  
    waveform.wBitsPerSample = 16;//采样比特，16bits/次  
    waveform.nChannels = 1;//采样声道数，2声道  
    waveform.nAvgBytesPerSec = 16000;//每秒的数据率，就是每秒能采集多少字节的数据  
    waveform.nBlockAlign = 2;//一个块的大小，采样bit的字节数乘以声道数  
    waveform.cbSize = 0;//一般为0  
  
    wait = CreateEvent(NULL, 0, 0, NULL);  
    //使用waveInOpen函数开启音频采集  
    //建立两个数组（这里可以建立多个数组）用来缓冲音频数据  
    DWORD bufsize = 1024*100;//每次开辟10k的缓存存储录音数据  
    int i;  
 
	while(isawake==false){
		i=2;
		fopen_s(&pf, IVW_AUDIO_FILE_NAME, "wb");
		waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform,(DWORD_PTR)wait, 0L, CALLBACK_EVENT);
		while(i--)//录制2秒左右声音，结合音频解码和网络传输可以修改为实时录音播放的机制以实现对讲功能  
		{  
			pBuffer1 = new BYTE[bufsize];  
			wHdr1.lpData = (LPSTR)pBuffer1;  
			wHdr1.dwBufferLength = bufsize;  
			wHdr1.dwBytesRecorded = 0;  
			wHdr1.dwUser = 0;  
			wHdr1.dwFlags = 0;  
			wHdr1.dwLoops = 1;  
			waveInPrepareHeader(hWaveIn, &wHdr1, sizeof(WAVEHDR));//准备一个波形数据块头用于录音  
			waveInAddBuffer(hWaveIn, &wHdr1, sizeof (WAVEHDR));//指定波形数据块为录音输入缓存  
			waveInStart(hWaveIn);//开始录音  
			Sleep(1000);//等待声音录制1s  
			waveInReset(hWaveIn);//停止录音  
			fwrite(pBuffer1, 1, wHdr1.dwBytesRecorded, pf);  
			delete pBuffer1;
		}  
		fclose(pf);  
  
		waveInClose(hWaveIn);
		const char *ssb_param = "ivw_threshold=0:1450,sst=wakeup,ivw_res_path =fo|res/ivw/wakeupresource.jet";
		run_ivw(NULL, IVW_AUDIO_FILE_NAME, ssb_param); 
	exit:
		MSPLogout(); //退出登录
	}
	if(!flagE)
	{
		m_PicWav.Stop();//因为一开始不让图片动，所以立刻停
		C人机交互mfcDlg *pThis = (C人机交互mfcDlg*)lpParameter;
		pThis->GetDlgItem(IDC_PIC)->ShowWindow(SW_HIDE);
		pThis->GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_STATIC)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_STATIC2)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_STATIC3)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_STATIC4)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_STATIC5)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_COMBO_LANGUAGE)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_RADIO1)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_RADIO2)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_BUTTON_SAVESTT)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_EDIT1)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_COMBO_SEX)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_SLIDER1)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_SLIDER2)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_EDIT7)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_EDIT8)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_EDIT2)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_BUTTON_TTS_START)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_BUTTON_TTS_STOP)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_BUTTON_TTS_CONTINUE)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_BUTTON_TTS_QUIT)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_BUTTON_OPENTXT)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_BUTTON_SAVETTS)->ShowWindow(SW_SHOW);
		pThis->GetDlgItem(IDC_PICWAV)->ShowWindow(SW_SHOW);
	}
	return 0;
}


BOOL C人机交互mfcDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_LanguageCom.AddString(L"中文");
	m_LanguageCom.AddString(L"英文");
	m_LanguageCom.AddString(L"日文");
	m_LanguageCom.SetCurSel(0);
	languageStr=L"zh_cn";
	m_ButtonSTT.SetCheck(1);
	m_SliderSpeed.SetRange(1,100);//设置范围  
    m_SliderSpeed.SetPos(50);//当前停留的位置  
	m_SliderVolume.SetRange(1,100);//设置范围  
    m_SliderVolume.SetPos(50);//当前停留的位置  
	m_SexaCom.AddString(L"女");
	m_SexaCom.AddString(L"男");
	m_SexaCom.SetCurSel(0);
	m_PicWav.Load(_T("喇叭.gif")); 
	m_PicWav.Draw();//不让图片先Draw一下就不会显示
	HANDLE hThread = CreateThread(NULL,0,xiancheng2,this,0,NULL);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}
//出门就是一个唤醒

void C人机交互mfcDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void C人机交互mfcDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR C人机交互mfcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int upload_userwords()
{
	char*			userwords	=	NULL;
	unsigned int	len			=	0;
	unsigned int	read_len	=	0;
	FILE*			fp			=	NULL;
	int				ret			=	-1;

	fp = fopen("userwords.txt", "rb");
	if (NULL == fp)										
	{
		AfxMessageBox(_T("open userwords.txt failed"));
		goto upload_exit;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp); //获取音频文件大小
	fseek(fp, 0, SEEK_SET);  					
	
	userwords = (char*)malloc(len + 1);
	if (NULL == userwords)
	{
		AfxMessageBox(_T("out of memory"));
		goto upload_exit;
	}

	read_len = fread((void*)userwords, 1, len, fp); //读取用户词表内容
	if (read_len != len)
	{
		AfxMessageBox(_T("read userwords.txt failed"));
		goto upload_exit;
	}
	userwords[len] = '\0';
	
	MSPUploadData("userwords", userwords, len, "sub = uup, dtt = userword", &ret); //上传用户词表
	if (MSP_SUCCESS != ret)
	{
		AfxMessageBox(_T("MSPUploadData failed"));
		goto upload_exit;
	}
	
upload_exit:
	if (NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}	
	if (NULL != userwords)
	{
		free(userwords);
		userwords = NULL;
	}
	
	return ret;
}
char	rec_result[BUFFER_SIZE]		=	{NULL};	
void run_iat(const char* audio_file, const char* session_begin_params)
{
	const char*		session_id					=	NULL;

	char			hints[HINTS_SIZE]			=	{NULL}; //hints为结束本次会话的原因描述，由用户自定义
	unsigned int	total_len					=	0; 
	int				aud_stat					=	MSP_AUDIO_SAMPLE_CONTINUE ;		//音频状态
	int				ep_stat						=	MSP_EP_LOOKING_FOR_SPEECH;		//端点检测
	int				rec_stat					=	MSP_REC_STATUS_SUCCESS ;			//识别状态
	int				errcode						=	MSP_SUCCESS ;

	FILE*			f_pcm						=	NULL;
	char*			p_pcm						=	NULL;
	long			pcm_count					=	0;
	long			pcm_size					=	0;
	long			read_size					=	0;

	for(int i=0;i<BUFFER_SIZE;i++)
	{
		rec_result[0]=NULL;
	}
	
	if (NULL == audio_file)
		goto iat_exit;

	f_pcm = fopen(audio_file, "rb");
	if (NULL == f_pcm) 
	{
		AfxMessageBox(_T("open audio file failed"));
		goto iat_exit;
	}
	
	fseek(f_pcm, 0, SEEK_END);
	pcm_size = ftell(f_pcm); //获取音频文件大小 
	fseek(f_pcm, 0, SEEK_SET);		

	p_pcm = (char *)malloc(pcm_size);
	if (NULL == p_pcm)
	{
		AfxMessageBox(_T("out of memory"));
		goto iat_exit;
	}

	read_size = fread((void *)p_pcm, 1, pcm_size, f_pcm); //读取音频文件内容
	if (read_size != pcm_size)
	{
		AfxMessageBox(_T("read audio file error"));
		goto iat_exit;
	}
	
	session_id = QISRSessionBegin(NULL, session_begin_params, &errcode); //听写不需要语法，第一个参数为NULL
	if (MSP_SUCCESS != errcode)
	{
		AfxMessageBox(_T("QISRSessionBegin failed"));
		goto iat_exit;
	}
	
	while (1) 
	{
		unsigned int len = 10 * FRAME_LEN; // 每次写入200ms音频(16k，16bit)：1帧音频20ms，10帧=200ms。16k采样率的16位音频，一帧的大小为640Byte
		int ret = 0;

		if (pcm_size < 2 * len) 
			len = pcm_size;
		if (len <= 0)
			break;

		aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
		if (0 == pcm_count)
			aud_stat = MSP_AUDIO_SAMPLE_FIRST;

		ret = QISRAudioWrite(session_id, (const void *)&p_pcm[pcm_count], len, aud_stat, &ep_stat, &rec_stat);
		if (MSP_SUCCESS != ret)
		{
			AfxMessageBox(_T("QISRAudioWrite failed"));
			goto iat_exit;
		}
			
		pcm_count += (long)len;
		pcm_size  -= (long)len;
		
		if (MSP_REC_STATUS_SUCCESS == rec_stat) //已经有部分听写结果
		{
			const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
			if (MSP_SUCCESS != errcode)
			{
				AfxMessageBox(_T("QISRGetResult failed"));
				goto iat_exit;
			}
			if (NULL != rslt)
			{
				unsigned int rslt_len = strlen(rslt);
				total_len += rslt_len;
				if (total_len >= BUFFER_SIZE)
				{
					AfxMessageBox(_T("no enough buffer for rec_result"));
					goto iat_exit;
				}
				strncat(rec_result, rslt, rslt_len);
			}
		}

		if (MSP_EP_AFTER_SPEECH == ep_stat)
			break;
		Sleep(200); //模拟人说话时间间隙。200ms对应10帧的音频
	}
	errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
	if (MSP_SUCCESS != errcode)
	{
		AfxMessageBox(_T("QISRAudioWrite failed"));
		goto iat_exit;	
	}

	while (MSP_REC_STATUS_COMPLETE != rec_stat) 
	{
		const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
		if (MSP_SUCCESS != errcode)
		{
			AfxMessageBox(_T("QISRGetResult failed"));
			goto iat_exit;
		}
		if (NULL != rslt)
		{
			unsigned int rslt_len = strlen(rslt);
			total_len += rslt_len;
			if (total_len >= BUFFER_SIZE)
			{
				AfxMessageBox(_T("no enough buffer for rec_result"));
				goto iat_exit;
			}
			strncat(rec_result, rslt, rslt_len);
		}
		Sleep(150); //防止频繁占用CPU
	}
	

iat_exit:
	if (NULL != f_pcm)
	{
		fclose(f_pcm);
		f_pcm = NULL;
	}
	if (NULL != p_pcm)
	{	free(p_pcm);
		p_pcm = NULL;
	}

	QISRSessionEnd(session_id, hints);
}


void C人机交互mfcDlg::Onwavtra()
{
	CString strFile = _T("");
    CFileDialog    dlgFile(TRUE, NULL, NULL, OFN_HIDEREADONLY, _T("(*.wav)|*.wav|(*.pcm)|*.pcm||"), NULL);
	if (dlgFile.DoModal())
	{
		strFile = dlgFile.GetPathName();
	}

	USES_CONVERSION;
	char *packet_filter=T2A(strFile);

	int			ret						=	MSP_SUCCESS;
	const char* login_params			=	"appid = 5dac08ae, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动

	/*
	* sub:				请求业务类型
	* domain:			领域
	* language:			语言
	* accent:			方言
	* sample_rate:		音频采样率
	* result_type:		识别结果格式
	* result_encoding:	结果编码格式
	*
	*/
	m_LanguageCom.GetWindowText(languageStr);
	CString languageType;
	if(languageStr==L"中文")
		languageType=L"zh_cn";
	else if(languageStr==L"英文")
		languageType=L"en_us";
	else if(languageStr==L"日文")
		languageType=L"ja_jp";
	CString str=L"sub = iat, domain = iat, language ="+languageType+L", accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";
	char temp[1000];
	::wsprintfA(temp, "%ls",(LPCTSTR)str);
	const char* session_begin_params=temp;

	/* 用户登录 */
	ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数	
	if (MSP_SUCCESS != ret)
	{
		AfxMessageBox(_T("MSPLogin failed"));
		goto exit; //登录失败，退出登录
	}

	run_iat(packet_filter, session_begin_params); 
	m_STTstr=rec_result;
	UpdateData(false);
exit:
	MSPLogout(); //退出登录

}
//上面为wav转文本



CString stringmic;
static void show_result(char *string, char is_over)
{
	CString str;
	COORD current;
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE w = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(w, &info);
	current = info.dwCursorPosition;

	if(current.X == last_pos.X && current.Y == last_pos.Y ) {
		SetConsoleCursorPosition(w, begin_pos);
	} else {
		/* changed by other routines, use the new pos as start */
		begin_pos = current;
	}
	if(is_over)
	{
		SetConsoleTextAttribute(w, FOREGROUND_GREEN);
		str=string;
		stringmic=stringmic+str;
	}
	HWND h= ::FindWindow(NULL, _T("人机交互mfc"));
	::PostMessage(h, 1000, 0,(LPARAM)(LPCTSTR)string);

	if(is_over)
		SetConsoleTextAttribute(w, info.wAttributes);

	GetConsoleScreenBufferInfo(w, &info);
	last_pos = info.dwCursorPosition;
}

	int key=0;
	bool control=false;
/* helper thread: to listen to the keystroke */
static unsigned int  __stdcall helper_thread_proc ( void * para)
{
	int quit = 0;
	int asd=0;//调试过程中使用的变量，方便查看函数是否调用成功
	do {
		if(control){
		switch(key) {
		case 's':
			asd=SetEvent(events[EVT_START]);
			control=false;
			break;
		case 'e':
			quit = 1;
			SetEvent(events[EVT_STOP]);
			control=false;
			PostQuitMessage(0);
			break;
		default:
			break;
		}
		}
		if(quit)
			break;		
	} while (1);

	return 0;
}

static HANDLE start_helper_thread()
{
	HANDLE hdl;

	hdl = (HANDLE)_beginthreadex(NULL, 0, helper_thread_proc, NULL, 0, NULL);

	return hdl;
}

static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;

void on_result(const char *result, char is_last)
{
	if (result) {
		size_t left = g_buffersize - 1 - strlen(g_result);
		size_t size = strlen(result);
		if (left < size) {
			g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
			if (g_result)
				g_buffersize += BUFFER_SIZE;
			else {
				AfxMessageBox(_T("mem alloc failed"));
				return;
			}
		}
		strncat(g_result, result, size);
		show_result(g_result, is_last);
	}
}
void on_speech_begin()
{
	if (g_result)
	{
		free(g_result);
	}
	g_result = (char*)malloc(BUFFER_SIZE);
	g_buffersize = BUFFER_SIZE;
	memset(g_result, 0, g_buffersize);

}
void on_speech_end(int reason)
{
	if (reason != END_REASON_VAD_DETECT)
		AfxMessageBox(_T("Recognizer error"));
}

/* demo send audio data from a file */
static void demo_file(const char* audio_file, const char* session_begin_params)
{
	unsigned int	total_len = 0;
	int				errcode = 0;
	FILE*			f_pcm = NULL;
	char*			p_pcm = NULL;
	unsigned long	pcm_count = 0;
	unsigned long	pcm_size = 0;
	unsigned long	read_size = 0;
	struct speech_rec iat;
	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};

	if (NULL == audio_file)
		goto iat_exit;

	f_pcm = fopen(audio_file, "rb");
	if (NULL == f_pcm)
	{
		AfxMessageBox(_T("open audio file failed"));
		goto iat_exit;
	}

	fseek(f_pcm, 0, SEEK_END);
	pcm_size = ftell(f_pcm); //获取音频文件大小 
	fseek(f_pcm, 0, SEEK_SET);

	p_pcm = (char *)malloc(pcm_size);
	if (NULL == p_pcm)
	{
		AfxMessageBox(_T("out of memory"));
		goto iat_exit;
	}

	read_size = fread((void *)p_pcm, 1, pcm_size, f_pcm); //读取音频文件内容
	if (read_size != pcm_size)
	{
		AfxMessageBox(_T("read audio file error"));
		goto iat_exit;
	}

	errcode = sr_init(&iat, session_begin_params, SR_USER, 0, &recnotifier);
	if (errcode) {
		AfxMessageBox(_T("speech recognizer init failed"));
		goto iat_exit;
	}

	errcode = sr_start_listening(&iat);
	if (errcode) {
		AfxMessageBox(_T("sr_start_listening failed"));
		goto iat_exit;
	}

	while (1)
	{
		unsigned int len = 10 * FRAME_LEN; // 每次写入200ms音频(16k，16bit)：1帧音频20ms，10帧=200ms。16k采样率的16位音频，一帧的大小为640Byte
		int ret = 0;

		if (pcm_size < 2 * len)
			len = pcm_size;
		if (len <= 0)
			break;

		ret = sr_write_audio_data(&iat, &p_pcm[pcm_count], len);

		if (0 != ret)
		{
			AfxMessageBox(_T("write audio data failed"));
			goto iat_exit;
		}

		pcm_count += (long)len;
		pcm_size -= (long)len;		
	}

	errcode = sr_stop_listening(&iat);
	if (errcode) {
		AfxMessageBox(_T("r_stop_listening failed"));
		goto iat_exit;
	}

iat_exit:
	if (NULL != f_pcm)
	{
		fclose(f_pcm);
		f_pcm = NULL;
	}
	if (NULL != p_pcm)
	{
		free(p_pcm);
		p_pcm = NULL;
	}

	sr_stop_listening(&iat);
	sr_uninit(&iat);
}
//wav的搞法
/* demo recognize the audio from microphone */


static void demo_mic(const char* session_begin_params)
{
	int			ret						=	MSP_SUCCESS;
	const char* login_params			=	"appid = 5dac08ae, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动
	ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数	
	if (MSP_SUCCESS != ret)	
	{
		AfxMessageBox(_T("MSPLogin failed"));//登录失败，退出登录
	}
	int isquit = 0;
	int errcode;
	int i = 0;
	HANDLE helper_thread = NULL;

	struct speech_rec iat;
	DWORD waitres;

	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};
	
	errcode = sr_init(&iat, session_begin_params, SR_MIC, DEFAULT_INPUT_DEVID, &recnotifier);
	if (errcode) {
		AfxMessageBox(_T("speech recognizer init failed"));
		return;
	}

	for (i = 0; i < EVT_TOTAL; ++i) {
		events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		//设定事件
	}

	helper_thread = start_helper_thread();
	if (helper_thread == NULL) {
		AfxMessageBox(_T("create thread failed"));
		goto exit;
	}


 	while (1) {
		waitres = WaitForMultipleObjects(EVT_TOTAL, events, FALSE, INFINITE);
		switch (waitres) {
		case WAIT_FAILED:
		case WAIT_TIMEOUT:
			AfxMessageBox(_T("time out"));
			break;
		case WAIT_OBJECT_0 + EVT_START:
			if (errcode = sr_start_listening(&iat)) {
				AfxMessageBox(_T("start listening failed"));
				isquit = 1;
			}
			break;
		case WAIT_OBJECT_0 + EVT_STOP:		
			if (errcode = sr_stop_listening(&iat)) {
				AfxMessageBox(_T("stop listening failed"));
			}
			isquit = 1;
			MSPLogout();
			break;
		default:
			break;
		}
		if (isquit)
			break;
	}

exit:
	if (helper_thread != NULL) {
		WaitForSingleObject(helper_thread, INFINITE);
		CloseHandle(helper_thread);
	}
	
	for (i = 0; i < EVT_TOTAL; ++i) {
		if (events[i])
			CloseHandle(events[i]);
	}

	sr_uninit(&iat);
}


/* main thread: start/stop record ; query the result of recgonization.
 * record thread: record callback(data write)
 * helper thread: ui(keystroke detection)
 */
afx_msg LRESULT C人机交互mfcDlg::OnSTTShow(WPARAM wParam, LPARAM lParam)
{
	m_STTstr=stringmic;
	UpdateData(FALSE);
	return 0;
}

DWORD WINAPI xiancheng(LPVOID lpParameter)
{
	C人机交互mfcDlg *pThis = (C人机交互mfcDlg*)lpParameter;
	pThis->m_LanguageCom.GetWindowText(pThis->languageStr);
	CString languageType;
	if(pThis->languageStr==L"中文")
		languageType=L"zh_cn";
	else if(pThis->languageStr==L"英文")
		languageType=L"en_us";
	else if(pThis->languageStr==L"日文")
		languageType=L"ja_jp";
	CString str=L"sub = iat, domain = iat, language ="+languageType+L", accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";
	char temp[1000];
	::wsprintfA(temp, "%ls",(LPCTSTR)str);
	const char* session_begin_params=temp;
	demo_mic(session_begin_params);
	return 0;
}



void C人机交互mfcDlg::mictra()
{
	// TODO: 在此添加控件通知处理程序代码
	int			ret						=	MSP_SUCCESS;
	const char* login_params			=	"appid = 5dac08ae, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动
	/*
	* sub:				请求业务类型
	* domain:			领域
	* language:			语言
	* accent:			方言
	* sample_rate:		音频采样率
	* result_type:		识别结果格式
	* result_encoding:	结果编码格式
	*
	*/
	m_LanguageCom.GetWindowText(languageStr);
	CString languageType;
	if(languageStr==L"中文")
		languageType=L"zh_cn";
	else if(languageStr==L"英文")
		languageType=L"en_us";
	else if(languageStr==L"日文")
		languageType=L"ja_jp";
	CString str=L"sub = iat, domain = iat, language ="+languageType+L", accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";
	char temp[1000];
	::wsprintfA(temp, "%ls",(LPCTSTR)str);
	const char* session_begin_params=temp;
		
	HANDLE hThread = CreateThread(NULL,0,xiancheng,this,0,NULL);
	UpdateData(false);
exit:
	MSPLogout(); //退出登录
}

/* 
  以下为音频实时转写内容
  s:start speaking
  e:end your speaking
 */

void C人机交互mfcDlg::OnSTTStart()
{
	GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON4)->ShowWindow(SW_SHOW);
	mictra();
	key='s';
	control=true;
}


void C人机交互mfcDlg::OnSTTEnd()
{
	GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_BUTTON4)->ShowWindow(SW_HIDE);
	key='e';
	control=true;
}


void C人机交互mfcDlg::OnBnClickedRadioMic()
{
	GetDlgItem(IDC_BUTTON1)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_BUTTON4)->ShowWindow(SW_HIDE);
	stringmic=L"";
}


void C人机交互mfcDlg::OnBnClickedRadioWav()
{
	GetDlgItem(IDC_BUTTON1)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON4)->ShowWindow(SW_HIDE);
	stringmic=L"";
}


void C人机交互mfcDlg::OnBnClickedButtonSaveSTT()
{
	CString m_File;
	CFileDialog fileDlg(FALSE,L"*.txt",m_File.GetBuffer(),OFN_HIDEREADONLY,L"(*.txt)|*.txt|");
	if(fileDlg.DoModal()==IDOK)
	{
		m_File=fileDlg.GetPathName();
		CStdioFile myFile;
		if(myFile.Open(m_File,CFile::typeText|CFile::modeCreate|CFile::modeReadWrite))
		{
			char* old_locale = _strdup(setlocale(LC_CTYPE,NULL) );
			setlocale( LC_CTYPE, "chs" );//设定f
			myFile.WriteString(m_STTstr);//正常写入
			setlocale( LC_CTYPE, old_locale );
			free( old_locale );//还原区域设定
			myFile.Close();
			MessageBox(L"保存成功",MB_OK);
		}
		else
		{
			MessageBox(L"保存失败",MB_OK);
			return;
		}
	}
}


void C人机交互mfcDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if((&m_SliderSpeed)==(CSliderCtrl *)pScrollBar)
	{
		UpdateData(TRUE);
		m_Speed=m_SliderSpeed.GetPos();//获得滑块的位置
		UpdateData(FALSE);
	}
	else if((&m_SliderVolume)==(CSliderCtrl *)pScrollBar)
	{
		UpdateData(TRUE);
		m_Volume=m_SliderVolume.GetPos();//获得滑块的位置
		UpdateData(FALSE);
	}
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


/* 文本合成 */
int C人机交互mfcDlg::text_to_speech(const char* src_text, const char* des_path, const char* params)
{
	int          ret          = -1;
	FILE*        fp           = NULL;
	const char*  sessionID    = NULL;
	unsigned int audio_len    = 0;
	wave_pcm_hdr wav_hdr      = default_wav_hdr;
	int          synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;
	int i=0;

	if (NULL == src_text || NULL == des_path)
	{
		AfxMessageBox(_T("params is error"));
		return ret;
	}
	fp = fopen(des_path, "wb");
	if (NULL == fp)
	{
		AfxMessageBox(_T("open audio file error"));
		return ret;
	}
	/* 开始合成 */
	sessionID = QTTSSessionBegin(params, &ret);
	if (MSP_SUCCESS != ret)
	{
		AfxMessageBox(_T("QTTSSessionBegin failed"));
		fclose(fp);
		return ret;
	}
	ret = QTTSTextPut(sessionID, src_text, (unsigned int)strlen(src_text), NULL);
	if (MSP_SUCCESS != ret)
	{
		AfxMessageBox(_T("QTTSTextPut failed"));
		QTTSSessionEnd(sessionID, "TextPutError");
		fclose(fp);
		return ret;
	}
	fwrite(&wav_hdr, sizeof(wav_hdr) ,1, fp); //添加wav音频头，使用采样率为16000
	while (1) 
	{
		i++;
		/* 获取合成音频 */
		const void* data = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
		if (MSP_SUCCESS != ret)
			break;
		if (NULL != data)
		{
			fwrite(data, audio_len, 1, fp);
		    wav_hdr.data_size += audio_len; //计算data_size大小
		}
		if (MSP_TTS_FLAG_DATA_END == synth_status)
			break;
	}
	if (MSP_SUCCESS != ret)
	{
		AfxMessageBox(_T("QTTSAudioGet failed"));
		QTTSSessionEnd(sessionID, "AudioGetError");
		fclose(fp);
		return ret;
	}
	/* 修正wav文件头数据的大小 */
	wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);
	
	/* 将修正过的数据写回文件头部,音频文件为wav格式 */
	fseek(fp, 4, 0);
	fwrite(&wav_hdr.size_8,sizeof(wav_hdr.size_8), 1, fp); //写入size_8的值
	fseek(fp, 40, 0); //将文件指针偏移到存储data_size值的位置
	fwrite(&wav_hdr.data_size,sizeof(wav_hdr.data_size), 1, fp); //写入data_size的值
	fclose(fp);
	fp = NULL;
	/* 合成完毕 */
	ret = QTTSSessionEnd(sessionID, "Normal");
	if (MSP_SUCCESS != ret)
	{
		AfxMessageBox(_T("QTTSSessionEnd failed"));
	}

	return ret;
}


void C人机交互mfcDlg::TTS()
{
	int         ret                  = MSP_SUCCESS;
	const char* login_params         = "appid = 5dac08ae, work_dir = .";//登录参数,appid与msc库绑定,请勿随意改动
	/*
	* rdn:           合成音频数字发音方式
	* volume:        合成音频的音量
	* pitch:         合成音频的音调
	* speed:         合成音频对应的语速
	* voice_name:    合成发音人
	* sample_rate:   合成音频采样率
	* text_encoding: 合成文本编码格式
	*
	*/
	m_SexaCom.GetWindowText(m_Sexstr);
	CString sexType;
	if(m_Sexstr==L"女")
		sexType=L"xiaoyan";
	else if(m_Sexstr==L"男")
		sexType=L"xiaofeng";
	CString speedStr;
	speedStr.Format(L"%d",m_Speed);
	CString volumeStr;
	volumeStr.Format(L"%d",m_Volume);
	CString str=L"engine_type = local, voice_name = "+sexType+L", text_encoding = GB2312, tts_res_path = fo|res\\tts\\"+sexType+L".jet;fo|res\\tts\\common.jet, sample_rate = 16000, speed = "+speedStr+L", volume = "+volumeStr+L", pitch = 50, rdn = 0";
	char temp[1000];
	::wsprintfA(temp, "%ls",(LPCTSTR)str);
	const char* session_begin_params=temp;
	const char* filename= "tts_sample.wav"; //合成的语音文件名称
	char temp1[1000];
	::wsprintfA(temp1, "%ls",(LPCTSTR)m_TTSstr);
	const char* text=temp1;
	/* 用户登录 */
	ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，第三个参数是登录参数，用户名和密码可在http://www.xfyun.cn注册获取
	if (MSP_SUCCESS != ret)
	{
		AfxMessageBox(_T("MSPLogin failed"));
	}
	/* 文本合成 */
	ret = text_to_speech(text, filename, session_begin_params);
	MSPLogout(); 
}


DWORD WINAPI xianchengTTS(LPVOID lpParameter)
{
	C人机交互mfcDlg *pThis = (C人机交互mfcDlg*)lpParameter;
	double i=pThis->wavPosition;
	while(pThis->flagPlay)
	{
		if(i>=pThis->wavLength)
			break;
		i+=0.0000032;
	}
	if(pThis->flagPlay)
	{
		pThis->GetDlgItem(IDC_BUTTON_TTS_CONTINUE)->EnableWindow(FALSE);
	    pThis->GetDlgItem(IDC_BUTTON_TTS_STOP)->EnableWindow(FALSE);
		m_PicWav.Stop();
		pThis->flagPlay=false;
	}
	return 0;
}

void C人机交互mfcDlg::OnBnClickedButtonTtsStart()
{
	UpdateData(TRUE);
	if(m_TTSstr==L"")
	{
		MessageBox(L"请先输入文本");
		return;
	}
	TTS();
	mciOpenParms.lpstrDeviceType=(LPCWSTR)(MCI_DEVTYPE_WAVEFORM_AUDIO);
	mciOpenParms.lpstrElementName=(LPCWSTR)(L"tts_sample.wav"); 
	mciSendCommand(NULL,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_WAIT|MCI_OPEN_ELEMENT,(DWORD)(LPVOID)&mciOpenParms);
	m_nDeviceID=mciOpenParms.wDeviceID;
	mciPlayParms.dwFrom=0;
	mciSendCommand(m_nDeviceID,MCI_PLAY,MCI_FROM ,(DWORD)(LPVOID)&mciPlayParms);
	//获取总时长
	StatusParms.dwItem = MCI_STATUS_LENGTH;
	mciSendCommand (m_nDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM,(DWORD)(LPVOID) &StatusParms);
	wavLength=StatusParms.dwReturn;
	StatusParms.dwItem = MCI_STATUS_POSITION;
	mciSendCommand (m_nDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM,(DWORD)(LPVOID) &StatusParms);
	wavPosition=StatusParms.dwReturn;
	flagPlay=true;
	HANDLE hThread = CreateThread(NULL,0,xianchengTTS,this,0,NULL);
	GetDlgItem(IDC_BUTTON_TTS_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_STOP)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_TTS_CONTINUE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_QUIT)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_SAVETTS)->EnableWindow(TRUE);
	m_PicWav.Draw();
}


void C人机交互mfcDlg::OnBnClickedButtonTtsStop()
{
	mciSendCommand(m_nDeviceID, MCI_PAUSE, 0,(DWORD)(LPVOID) &mciPlayParms);
	GetDlgItem(IDC_BUTTON_TTS_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_CONTINUE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_TTS_QUIT)->EnableWindow(TRUE);
	flagPlay=false;
	m_PicWav.Stop();
}


void C人机交互mfcDlg::OnBnClickedButtonTtsContinue()
{
	mciSendCommand(m_nDeviceID, MCI_PLAY, 0,(DWORD)(LPVOID)&mciPlayParms);
	GetDlgItem(IDC_BUTTON_TTS_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_STOP)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_TTS_CONTINUE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_QUIT)->EnableWindow(TRUE);
	StatusParms.dwItem = MCI_STATUS_POSITION;
	mciSendCommand (m_nDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM,(DWORD)(LPVOID) &StatusParms);
	wavPosition=StatusParms.dwReturn;
	flagPlay=true;
	m_PicWav.Draw();
	HANDLE hThread = CreateThread(NULL,0,xianchengTTS,this,0,NULL);
}


void C人机交互mfcDlg::OnBnClickedButtonTtsQuit()
{
	mciSendCommand(m_nDeviceID, MCI_CLOSE, NULL, NULL);
	GetDlgItem(IDC_BUTTON_TTS_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_TTS_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_CONTINUE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_TTS_QUIT)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_SAVETTS)->EnableWindow(FALSE);
	m_PicWav.Stop();
	flagPlay=false;
}

void C人机交互mfcDlg::OnBnClickedButtonOpentxt()
{
	CString strFile;
	CFileDialog fileDlg(TRUE,L"*.txt",strFile.GetBuffer(),OFN_HIDEREADONLY,L"文本文件（*.txt)|*.txt||");
	if(fileDlg.DoModal()==IDOK)
	{
		strFile=fileDlg.GetPathName();
		m_TTSstr=L"";
		CStdioFile myFile;
		CFileException fileException;
		myFile.Open(strFile,CFile::typeText|CFile::modeReadWrite),&fileException;
		myFile.SeekToBegin();
		CString filewrite;
		BOOL tell;
		char* old_locale = _strdup(setlocale(LC_CTYPE,NULL) );
		setlocale( LC_CTYPE, "chs" );//设定f
		tell=myFile.ReadString(m_TTSstr);
		while(tell=myFile.ReadString(filewrite))//ReadString一次只能读一行
		{
			m_TTSstr=m_TTSstr+L"\n"+filewrite;
		}//若未到文件尾，则继续读
		setlocale( LC_CTYPE, old_locale );
		free( old_locale );//还原区域设定
		myFile.Close();
		UpdateData(FALSE);
	}
}


void C人机交互mfcDlg::OnBnClickedButtonSavetts()
{
	CString strFile;
	CFileDialog fileDlg(FALSE,L"*.txt",strFile.GetBuffer(),OFN_HIDEREADONLY,L"(*.wav)|*.wav|(*.pcm)|*.pcm|(*.mp3)|*.mp3||");
	if(fileDlg.DoModal()==IDOK)
	{
		strFile=fileDlg.GetPathName();
		MCI_SAVE_PARMS SaveParms;
		SaveParms.lpfilename = (LPCWSTR)strFile;
		mciSendCommand (m_nDeviceID, MCI_SAVE,MCI_SAVE_FILE | MCI_WAIT,(DWORD)(LPVOID) &SaveParms);
		MessageBox(L"保存成功",MB_OK);
	}
}

BOOL C人机交互mfcDlg::PreTranslateMessage( MSG* pMsg )
{
	if   (m_hAccel)   
	{ 
		if   (::TranslateAccelerator(m_hWnd,m_hAccel,pMsg))   
		{ 
			return(TRUE); 
		} 
	}
	return   CDialog::PreTranslateMessage(pMsg); 
}

void C人机交互mfcDlg::OnAcceleratorE()
{
	if((!flagE)&&(!isawake))
	{
		isawake=true;
		flagE=!flagE;
		MSPLogout();
		m_PicWav.Stop();//因为一开始不让图片动，所以立刻停
		GetDlgItem(IDC_PIC)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC2)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC3)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC4)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC5)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_COMBO_LANGUAGE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO1)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO2)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_SAVESTT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT1)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_COMBO_SEX)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER1)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER2)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT7)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT8)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT2)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_TTS_START)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_TTS_STOP)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_TTS_CONTINUE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_TTS_QUIT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_OPENTXT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_SAVETTS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_PICWAV)->ShowWindow(SW_SHOW);
	}
}
