
// �˻�����mfc.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// C�˻�����mfcApp:
// �йش����ʵ�֣������ �˻�����mfc.cpp
//

class C�˻�����mfcApp : public CWinApp
{
public:
	C�˻�����mfcApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern C�˻�����mfcApp theApp;