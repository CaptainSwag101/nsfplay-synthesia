// KeyHeader.cpp : 実装ファイル
//

#include "stdafx.h"
#include "nsfplug_ui.h"
#include "resource.h"
#include "KeyHeader.h"


// KeyHeader ダイアログ

IMPLEMENT_DYNAMIC(KeyHeader, CDialog)
KeyHeader::KeyHeader(CWnd* pParent /*=NULL*/)
	: CDialog(KeyHeader::IDD, pParent)
{
  numberofframes = 1;
  drums_speed = 1;
  drumsHeight = 60;
  synthesiaWidth = 841;
  memset(m_nNoiseStatus, 0, sizeof(m_nNoiseStatus));
  memset(m_nDPCMStatus, 0, sizeof(m_nDPCMStatus));
  memset(m_n5BNoiseStatus, 0, sizeof(m_n5BNoiseStatus));
  softgray_pen.CreatePen(PS_SOLID,1,RGB(212,212,212));
}

KeyHeader::~KeyHeader()
{
  if(softgray_pen.DeleteObject()==0)
    xgm::DEBUG_OUT("Error in deleting the softgray_pen object.\n");
}

void KeyHeader::Reset()
{
  //m_nNoiseStatus = 0;
  //m_nDPCMStatus = 0;
	memset(m_nNoiseStatus, 0, sizeof(m_nNoiseStatus));
	memset(m_nDPCMStatus, 0, sizeof(m_nDPCMStatus));
	memset(m_n5BNoiseStatus, 0, sizeof(m_n5BNoiseStatus));
}

void KeyHeader::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(KeyHeader, CDialog)
  ON_WM_PAINT()
END_MESSAGE_MAP()


// KeyHeader メッセージ ハンドラ

BOOL KeyHeader::OnInitDialog()
{
  CDialog::OnInitDialog();

  CDC *pDC = GetDC();

  //m_hedBitmap.LoadBitmap(IDB_KEYHEADER);
  //m_hedDC.CreateCompatibleDC(pDC);
  //m_hedDC.SelectObject(&m_hedBitmap);

  m_hedBitmap.CreateCompatibleBitmap(pDC,MaxWidth(),MaxHeight());
  m_hedDC.CreateCompatibleDC(pDC);
  m_hedDC.SelectObject(&m_hedBitmap);
  m_memBitmap.CreateCompatibleBitmap(pDC,MaxWidth(),MaxHeight());
  m_memDC.CreateCompatibleDC(pDC);
  m_memDC.SelectObject(&m_memBitmap);
  m_memDC.SelectObject(&softgray_pen);

  ReleaseDC(pDC);
  return TRUE;  // return TRUE unless you set the focus to a control
}

static COLORREF Noise5BColours[32] =
{
	RGB(0,97,255), RGB(0,101,255), RGB(0,104,255), RGB(0,110,255), RGB(0,114,255), RGB(0,118,255), RGB(0,123,255), RGB(0,126,255), //between 2-# and 3-#
	RGB(0,129,255), RGB(0,132,255), RGB(0,136,255), RGB(0,139,255), RGB(0,143,255), RGB(0,148,255), RGB(0,153,255), RGB(0,158,255), //between 3-# and 4-#
	RGB(0,165,255), RGB(0,172,255), RGB(0,180,255), RGB(0,190,255), //between 4-# and 5-#
	RGB(0,196,255), RGB(0,203,255), RGB(0,212,255), RGB(0,222,255), //between 5-# and 6-#
	RGB(0,238,251), //between 6-# and 7-#
	RGB(0,255,242), //between 7-# and 8-#
	RGB(0,255,217), RGB(0,255,186), RGB(0,255,155), RGB(0,255,124), RGB(63,255,93), //exactly 8-#, 9-#, A-#, B-# and C-#
	RGB(255,255,170) //higher than even F-#
};

void KeyHeader::OnPaint()
{
  CPaintDC dc(this); // device context for painting
  CRect rect;
  GetClientRect(rect);

  //m_memDC.FillSolidRect(rect,RGB(0,0,0));
  //m_memDC.FillSolidRect(60, 119, 60, 1, RGB(100, 100, 100));
  
  //m_memDC.BitBlt(0,0,336,20,&m_hedDC,0,0,SRCCOPY);

  //copy onto header and back to displace everything down
  m_hedDC.BitBlt(0,numberofframes*drums_speed,rect.Width(),rect.Height(),&m_memDC,0,0,SRCCOPY);
  m_memDC.BitBlt(0,0,rect.Width(),rect.Height(),&m_hedDC,0,0,SRCCOPY);
  m_memDC.FillSolidRect(0, 0, rect.Width(), numberofframes*drums_speed, RGB(0, 0, 0));

  for (int i = 0; i < numberofframes; ++i)
  {
	if(m_nNoiseStatus[i])
	{
		/*CRect rect(0,0,5-(14-m_nNoiseVolume[i])/3,m_nNoiseVolume > 0 ? 4 : 1);
		rect.MoveToXY(92+(16-m_nNoiseStatus[i])*8,5);
		m_memDC.FillSolidRect(rect, m_nNoiseTone[0] ? RGB(240,0,0) : RGB(0,240,0));*/
		//status 0 is x co-ordinate 24, +16 for each 1 in status
		//y co-ordinate is equal to i, height is 1 = bottom is top + 1
		//width is equal to 1 per volume, increase left by volume/2, increase right by volume/2 + 1
		//colour is 0,0,255 at left, 0,255,0 at right for tone 0, 255,0,0 at left, 0,255,0 at right for tone 1

		CRect rect;
		rect.top = i*drums_speed;
		rect.bottom = rect.top + drums_speed;
		int width = (m_nNoiseVolume[i] + 1) / 17;
		int middle = 8 + (m_nNoiseStatus[i])*16;
		rect.left = middle - width/2;
		rect.right = middle + (width+1)/2;
		COLORREF color;
		if (m_nNoiseTone[i] != 0)
		{
			if (m_nNoiseStatus[i] < 8)
			{
				color = RGB(255, m_nNoiseStatus[i]*16-1, 0);
			}
			else
			{
				color = RGB(255, m_nNoiseStatus[i]*16-1, 0);
			}
		}
		else
		{
			if (m_nNoiseStatus[i] < 8)
			{
				color = RGB(0, m_nNoiseStatus[i]*32-1, 255);
			}
			else
			{
				//int divisor = m_nNoiseStatus[i] > 14 ? 3 - (m_nNoiseStatus[i]-14) : 1;
				color = RGB(m_nNoiseStatus[i] > 12 ? 63*(m_nNoiseStatus[i]-12) : 0, 255, ((16-m_nNoiseStatus[i])*31) + (m_nNoiseStatus[i] > 14 ? 80*(m_nNoiseStatus[i]-14) : 0) );
				
			}
		}
		m_memDC.FillSolidRect(rect, color);
	}
	if(m_n5BNoiseStatus[i])
	{
		//32 frequencies starting after the previous 16.
		CRect rect;
		rect.top = i*drums_speed;
		rect.bottom = rect.top + drums_speed;
		int width = (m_n5BNoiseVolume[i] + 1) / 17;
		int middle = 8 + 16*16 + (m_n5BNoiseStatus[i])*16;
		rect.left = middle - width/2;
		rect.right = middle + (width+1)/2;
		COLORREF color;
		color = Noise5BColours[m_n5BNoiseStatus[i]-1];
		m_memDC.FillSolidRect(rect, color);
	}
	if(m_nDPCMStatus[i] && m_nDPCMTone[i])
	{
		CRect rect;
		rect.top = i*drums_speed;
		rect.bottom = rect.top + drums_speed;
		int width = 8; //(m_nDPCMVolume[i] + 1) / 17;
		int middle = 8;
		rect.left = middle - width/2;
		rect.right = middle + (width+1)/2;
		COLORREF color = RGB(((m_nDPCMTone[i])&0xFF), ((m_nDPCMTone[i]>>8)&0xF)*16, 255);
		m_memDC.FillSolidRect(rect, color);
	}
  }

  //move everything down by numberofframes or something

  //m_memDC.Draw3dRect(0,0,rect.right,rect.bottom,RGB(255,255,255),RGB(128,128,128)); //turn back on?

  //m_memDC.MoveTo(0,20);
  //m_memDC.LineTo(rect.right,20);
  dc.BitBlt(0,0,rect.right,rect.bottom,&m_memDC,0,0,SRCCOPY);
}
