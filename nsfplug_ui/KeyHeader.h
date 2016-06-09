#pragma once
#include "NSFDialog.h"
#include "NSFTrackSetupDialog.h"

// KeyHeader ダイアログ

class KeyHeader : public CDialog
{
	DECLARE_DYNAMIC(KeyHeader)

public:
	KeyHeader(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~KeyHeader();

  // キーボードヘッダの元絵
  CBitmap m_hedBitmap;
  CDC m_hedDC;

  // 裏画面領域
  CBitmap m_memBitmap;
  CDC m_memDC;

  // ペン
  CPen softgray_pen;

  int m_nNoiseStatus[10];
  int m_nNoiseVolume[10];
  int m_nNoiseTone[10];
  int m_nDPCMStatus[10];
  int m_nDPCMVolume[10];
  int m_nDPCMTone[10];
  int m_n5BNoiseStatus[10];
  int m_n5BNoiseVolume[10];

  int numberofframes;
  int drums_speed;

  virtual void Reset();

  inline int MinWidth(){ return 336+1; }
  inline int MaxWidth(){ return synthesiaWidth; }
  inline int MinHeight(){ return 60; }
  inline int MaxHeight(){ return drumsHeight; }
  int drumsHeight;
  int synthesiaWidth;

// ダイアログ データ
	enum { IDD = IDD_KEYHEADER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  afx_msg void OnPaint();
};
