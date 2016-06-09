#pragma once
#include "NSFDialog.h"
#include "afxcmn.h"
#include "NSFTrackSetupDialog.h"
#include "keydialog.h"

// NSFTrackDialog ダイアログ

class NSFTrackDialog : public CDialog, public NSFDialog
{
	DECLARE_DYNAMIC(NSFTrackDialog)
public:
	NSFTrackDialog(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~NSFTrackDialog();
  BOOL m_active;
  KeyDialog m_keydlg;
  int m_trkmap[NSFPlayer::NES_TRACK_MAX];
  int m_maxtrk;
  //BOOL m_showtrk[NSFPlayer::NES_TRACK_MAX];
  int m_keepkey[NSFPlayer::NES_TRACK_MAX];
  int m_lastkey[NSFPlayer::NES_TRACK_MAX];
  CString m_tonestr[NSFPlayer::NES_TRACK_MAX];
  COLORREF ColorForNote(xgm::ITrackInfo*, int);
  COLORREF ColorForWave(int, xgm::INT16[]);
  int octaveAdjustment();
  void InitList();
  BOOL m_initializing_list;
  NSFTrackSetupDialog m_setup;
  UINT_PTR m_nTimer, m_nTimer2;
  CPen green_pen;
  CMenu m_rmenu;
  int m_trk_selected;
  int frame_of_last_update;
  //copied from nsfplay.h
  int GetRegion(xgm::UINT8);
  enum {
    REGION_NTSC = 0,
    REGION_PAL,
    REGION_DENDY
  };

// ダイアログ データ
	enum { IDD = IDD_TRKINFO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
  void LocateDialogItems();

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnTimer(UINT nIDEvent);
  virtual BOOL OnInitDialog();
  void UpdateNSFPlayerConfig(bool b);
  afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
  CListCtrl m_trkinfo;
  CDC *m_pDCtrk; // ListCtrlのデバイスコンテキスト

  afx_msg void OnLvnItemchangedTrackinfo(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnBnClickedSetup();
  afx_msg void OnDropFiles(HDROP hDropInfo);
  CSliderCtrl m_speed;
  afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
  afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  virtual BOOL DestroyWindow();
  afx_msg void OnCopy();
  afx_msg void OnSettings();
  afx_msg void OnNMRclickTrackinfo(NMHDR *pNMHDR, LRESULT *pResult);
};
