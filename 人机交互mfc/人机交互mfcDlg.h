
// �˻�����mfcDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include <mmstream.h>
#include<mmsystem.h>
#include<mmreg.h>
#include "PictureEx.h"
#pragma comment(lib, "winmm.lib")


// C�˻�����mfcDlg �Ի���
class C�˻�����mfcDlg : public CDialogEx
{
// ����
public:
	C�˻�����mfcDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnSTTShow(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void Onwavtra();
	void mictra();
	afx_msg void OnSTTStart();
	afx_msg void OnSTTEnd();
	CComboBox m_LanguageCom;
	CString languageStr;
	CString m_STTstr;
	CButton m_ButtonSTT;
	afx_msg void OnBnClickedRadioMic();
	afx_msg void OnBnClickedRadioWav();
	afx_msg void OnBnClickedButtonSaveSTT();
	int m_Speed;
	int m_Volume;
	CSliderCtrl m_SliderSpeed;
	CSliderCtrl m_SliderVolume;
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedButtonTtsStart();
	afx_msg void OnBnClickedButtonTtsStop();
	afx_msg void OnBnClickedButtonTtsContinue();
	afx_msg void OnBnClickedButtonTtsQuit();
	int text_to_speech(const char* src_text, const char* des_path, const char* params);
	void TTS();
	MCI_OPEN_PARMS mciOpenParms;
	MCI_PLAY_PARMS mciPlayParms;
	MCI_STATUS_PARMS StatusParms;
	DWORD m_nDeviceID;
	CComboBox m_SexaCom;
	CString m_Sexstr;
	CString m_TTSstr;
	long wavLength;
	long wavPosition;
	afx_msg void OnBnClickedButtonOpentxt();
	BOOL flagPlay;
	afx_msg void OnBnClickedButtonSavetts();
	HACCEL m_hAccel;
	afx_msg void OnAcceleratorE();
};
