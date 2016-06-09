// KeyWindow.cpp : 実装ファイル
//
#include "stdafx.h"
#include "nsfplug_ui.h"
#include "resource.h"
#include "KeyWindow.h"
#include "algorithm"

// KeyWindow ダイアログ
IMPLEMENT_DYNAMIC(KeyWindow, CDialog)
KeyWindow::KeyWindow(CWnd* pParent /*=NULL*/)
	: CDialog(KeyWindow::IDD, pParent)
{
  m_bShowOctave = true;
  m_bInit = false;
  //lastnumberofframes = 1;
  numberofframes = 1;
  synth_speed = 1;
  synthesiaHeight = 400;
  synthesiaWidth = 841;
  startingOctave = 1;
  memset(m_nKeyStatus, 0, sizeof(m_nKeyStatus));
  memset(m_nTrackStatus, 0, sizeof(m_nTrackStatus));
}

KeyWindow::~KeyWindow()
{
}

// キーボードを書く
void KeyWindow::draw_keyboard(CDC *pDC, int x, int y, int nx, int ny)
{
  static int key[14] = {1,2,1,2,1,0,1,2,1,2,1,2,1,0};
  CRect rect;
  int i;

  rect.left = 0;
  rect.top = 0;
  rect.right = 14*NUM_OCTAVES/2*nx;
  rect.bottom = ny;
  pDC->FillSolidRect(&rect, RGB(0,0,0));

  for(i=0; i<14*NUM_OCTAVES; i+=2)
  {
    switch(key[i%14])
    {
    case 1:
      rect.left = x + i/2*nx;
      rect.top = y+1;
      rect.right = rect.left + nx-1;
      rect.bottom = rect.top + ny-2;
      pDC->FillSolidRect(&rect, RGB(240,240,240));
      pDC->SetPixel(rect.left-1,rect.bottom,RGB(240,240,240));
      pDC->SetPixel(rect.right,rect.bottom,RGB(240,240,240));
      break;
    default:
      break;
    }
  }

  for(i=0; i<14*NUM_OCTAVES; i++)
  {
    switch(key[i%14])
    {
    case 2:
      rect.left = x + i/2*nx + nx/2 ;
      rect.top = y+1;
      rect.right = rect.left + nx - 1;
      rect.bottom = rect.top + ny*4/7;
      pDC->FillSolidRect(&rect, RGB(0,0,0));
      pDC->Draw3dRect(&rect, RGB(128,128,128), RGB(0,0,0));
      break;
    default:
      break;
    }
  }

  // オクターブ描画
  if(m_bShowOctave)
  {
    for(i=0; i<NUM_OCTAVES; i++)
      pDC->BitBlt(x+i*7*nx+1,y+1+ny-2-6,4,5,&m_digDC,((i+startingOctave+40)*4)%40,0,SRCCOPY);
  }

}

long leftfromkey(int key, int x, int nx)
{
	if(key&1)
      {
        return x + key/2*nx+nx/2 + nx/4;
      }
      else
      {
        return x + key/2*nx + nx/4;
      }
}

void KeyWindow::draw_notes(CDC *pDC, int x, int y, int nx, int ny)
{
  int i;
  int j;
  //draw notes
  for(i=0; i<14*NUM_OCTAVES; i++)
  {
    if(m_nKeyStatus[i])
    {
	  CRect rect;
      if(i&1)
      {
        rect.left = leftfromkey(i, x, nx);
        rect.top = y-1+ny/4;
      }
      else
      {
        rect.left = leftfromkey(i, x, nx);
        rect.top = y+4+ny*4/7;
      }
      rect.right = rect.left + nx/2;
      rect.bottom = rect.top + nx/2;

      pDC->FillSolidRect(&rect, RGB((m_nKeyColor[i])&0xFF,(m_nKeyColor[i]>>8)&0xFF,(m_nKeyColor[i]>>16)&0xFF));
	}
	
  }
  //draw tracks
  for (j = 0; j < numberofframes; ++j)
  {
	  std::vector<int> priorities;
	  std::vector<int>::reverse_iterator itr;
	  for (i = 0; i < numberoftracks; ++i)
	  {
		//first, sort by volume
		if (m_nTrackStatus[i][j])
		{
			priorities.push_back((m_nTrackVolume[i][j] << 8) + i);
		}
	  }
	  //now draw from volumousmost to volumousleast

	  stable_sort(priorities.begin(), priorities.end());
	  for (itr = priorities.rbegin(); itr != priorities.rend(); ++itr)
	  {
		i = (*itr)&0xFF;
		int note = (int)m_nTrackFractionalNote[i][j];
		int tbl[12] = { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12 };
	    int key = (tbl[note%12]+(note/12)*14); //insert magic here
		int higherkey;
		int lowerkey;
		double interpolation = m_nTrackFractionalNote[i][j] - note;

		if (interpolation > 0.5)
		{
			//interpolate with key to the right
			lowerkey = key;
			higherkey = (tbl[(note+1)%12]+((note+1)/12)*14);
		}
		else
		{
			//interpolate with key to the left
			lowerkey = (tbl[(note-1)%12]+((note-1)/12)*14);
			higherkey = key;
		}

		static int keys[14] = {1,2,1,2,1,0,1,2,1,2,1,2,1,0};
		bool bothwhite = keys[lowerkey%14] == 1 && keys[higherkey%14] == 1;
		bool higherblack = keys[lowerkey%14] == 1 && keys[higherkey%14] == 2;
		//else lowerblack is true

        int lowerleft = leftfromkey(lowerkey, x, nx);
		int higherleft = leftfromkey(higherkey, x, nx);
		int middleleft;
		if (interpolation > 0.5)
		{
			//0.5 is full lowerleft, 1.0 is half way to higher left
			middleleft = lowerleft + (int)((double)(higherleft - (double)lowerleft)*(interpolation-0.5));
		}
		else
		{
			//0.0 is half way to lower left, 0.5 is full higherleft
			middleleft = lowerleft + (int)((double)(higherleft - (double)lowerleft)*(interpolation+0.5));
		}
	    int middleright = middleleft + nx/2;

	    CRect synthesiarect;
	    double minsizeadjust = 5.0;
        int widthpixels = (int)((max(m_nTrackVolume[i][j]/17.0, 1.0) - minsizeadjust)*(nx/12.0));

	    synthesiarect.top = ny + 1 + j*synth_speed;
	    synthesiarect.bottom = synthesiarect.top + synth_speed;

	    synthesiarect.left = middleleft-(widthpixels/2);
	    synthesiarect.right = middleright+((widthpixels+1)/2);

	    if (synthesiarect.left == synthesiarect.right) synthesiarect.right += 1;
	  
	    if (key&1)
		  pDC->FillSolidRect(&synthesiarect, RGB((m_nTrackColor[i][j]&0x0000FF)>>1,(m_nTrackColor[i][j]&0x00FF00)>>9,(m_nTrackColor[i][j]&0xFF0000)>>17));
	    else
		  pDC->FillSolidRect(&synthesiarect, RGB((m_nTrackColor[i][j])&0xFF,(m_nTrackColor[i][j]>>8)&0xFF,(m_nTrackColor[i][j]>>16)&0xFF));
    }
  }
}

void KeyWindow::KeyOn(int note, int color)
{
  int tbl[12] = { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12 };
  if(note<=0) return;
  m_nKeyStatus[(tbl[note%12]+(note/12)*14)&0xFF] = 1;
  m_nKeyColor[(tbl[note%12]+(note/12)*14)&0xFF] = color;
}

void KeyWindow::TrackOn(int track, int color, int volume, double fractionalNote, int frame)
{
	if (fractionalNote < 1.0) return;
	if (frame >= MAX_FRAMES) return;
	m_nTrackStatus[track][frame] = 1;
	m_nTrackColor[track][frame] = color;
	m_nTrackVolume[track][frame] = volume;
	m_nTrackFractionalNote[track][frame] = fractionalNote;
}

void KeyWindow::KeyOff(int note)
{
  int tbl[12] = { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12 };
  if(note<=0) return;
  m_nKeyStatus[(tbl[note%12]+(note/12)*14)&0xFF] = 0;
}

void KeyWindow::Reset()
{
  /*for(int i=0;i<256;i++)
  for(int j = 0; j < MAX_FRAMES; ++j)
  {
    m_nKeyStatus[i][j]=0;
  }*/

  memset(m_nKeyStatus, 0, sizeof(m_nKeyStatus));
  memset(m_nTrackStatus, 0, sizeof(m_nTrackStatus));
}

void KeyWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(KeyWindow, CDialog)
  ON_WM_SIZE()
  ON_WM_PAINT()
  ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


// KeyWindow メッセージ ハンドラ
BOOL KeyWindow::OnInitDialog()
{
  CDialog::OnInitDialog();

  CDC *pDC = GetDC();

  m_digBitmap.LoadBitmap(IDB_DIGIT);
  m_digDC.CreateCompatibleDC(pDC);
  m_digDC.SelectObject(&m_digBitmap);
  m_keyBitmap.CreateCompatibleBitmap(pDC,MaxWidth(),SynthesiaHeight()+MaxWidth()/14);
  m_keyDC.CreateCompatibleDC(pDC);
  m_keyDC.SelectObject(&m_keyBitmap);
  m_memBitmap.CreateCompatibleBitmap(pDC,MaxWidth(),SynthesiaHeight()+MaxWidth()/14);
  m_memDC.CreateCompatibleDC(pDC);
  m_memDC.SelectObject(&m_memBitmap);

  ReleaseDC(pDC);
  m_bInit = true;
  SetWindowPos(NULL,0,0,MinWidth(),24+SynthesiaHeight(),SWP_NOMOVE|SWP_NOZORDER);

  return TRUE;  // return TRUE unless you set the focus to a control
  // 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void KeyWindow::OnSize(UINT nType, int cx, int cy)
{
  CDialog::OnSize(nType, cx, cy);

  if(m_bInit)
  {
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	int keyheight = rect.Height()-SynthesiaHeight();
	m_keyDC.FillSolidRect(0, 0, rect.Width(), keyheight+1, RGB(0, 0, 0));
	m_memDC.FillSolidRect(0, keyheight-1, rect.Width(), SynthesiaHeight(), RGB(0, 0, 0));

	cy = cy - SynthesiaHeight();
    draw_keyboard(&m_keyDC,1,0,cy/(NUM_OCTAVES/2),cy);
	RedrawWindow(0, 0, RDW_INVALIDATE);
  }
}

void KeyWindow::OnPaint()
{
  CPaintDC dc(this); // device context for painting
  CRect rect;
  GetClientRect(rect);
  int keyheight = rect.Height()-SynthesiaHeight();

  /*
  m_keyDC.BitBlt(0,keyheight+(usingoldframe ? 2 : 1),rect.Width(),rect.Height(),&m_memDC,0,keyheight,SRCCOPY);
  dc.BitBlt(0,0,rect.Width(),rect.Height(),&m_memDC,0,0,SRCCOPY);
  m_memDC.BitBlt(0,0,rect.Width(),rect.Height(),&m_keyDC,0,0,SRCCOPY);
  draw_notes(&m_memDC,1,0,(keyheight)/4,keyheight);
  */

  m_keyDC.BitBlt(0,keyheight+(numberofframes*synth_speed),rect.Width(),rect.Height(),&m_memDC,0,keyheight,SRCCOPY);
  m_memDC.BitBlt(0,0,rect.Width(),rect.Height(),&m_keyDC,0,0,SRCCOPY);
  m_memDC.FillSolidRect(0, keyheight+1, rect.Width(), numberofframes*synth_speed, RGB(0, 0, 0));
  draw_notes(&m_memDC,1,0,(keyheight)/(NUM_OCTAVES/2),keyheight);
  dc.BitBlt(0,0,rect.Width(),rect.Height(),&m_memDC,0,0,SRCCOPY);

  /*
  m_memDC.BitBlt(0,0,rect.Width(),rect.Height(),&m_keyDC,0,0,SRCCOPY);
  m_memDC.BitBlt(0, keyheight+(usingoldframe ? 2 : 1), rect.Width(), rect.Height(), &m_keyDC, 0, keyheight, SRCCOPY);
  m_memDC.FillSolidRect(0, keyheight+1, 0, (usingoldframe ? 2 : 1), RGB(0, 0, 0));
  draw_notes(&m_memDC,1,0,(keyheight)/4,keyheight);
  m_keyDC.BitBlt(0,keyheight,rect.Width(),rect.Height(),&m_memDC,0,keyheight,SRCCOPY);
  dc.BitBlt(0,0,rect.Width(),rect.Height(),&m_memDC,0,0,SRCCOPY);*/

  //lastnumberofframes = numberofframes;

  // 描画メッセージで CDialog::OnPaint() を呼び出さないでください。
}

void KeyWindow::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  m_bShowOctave = !m_bShowOctave;
  CDialog::OnLButtonDblClk(nFlags, point);
}

BOOL KeyWindow::DestroyWindow()
{
  return CDialog::DestroyWindow();
}
