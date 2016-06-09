// NSFTrackDialog.cpp : 実装ファイル
//

#include "stdafx.h"
#include "nsfplug_ui.h"
#include "NSFDialogs.h"
#include "NSFTrackDialog.h"
#include "math.h"

// color reader
int read_color(const char* c)
{
    if (c == NULL) return -1;
    //DEBUG_OUT("read_color: %s\n",c);
    if (::strlen(c) != 6) return -1;
    int shift = 6;
    int result = 0;
    do
    {
        --shift;
        char x = *c;
        ++c;
        int val = -1;
        if      (x >= '0' && x <= '9') val = x - '0';
        else if (x >= 'a' && x <= 'f') val = 10 + x - 'a';
        else if (x >= 'A' && x <= 'F') val = 10 + x - 'A';
        if (val < 0 || val > 15) return -1;
        result += (val << (shift * 4));
    } while (shift != 0);
    return result;
}

// NSFTrackDialog ダイアログ

IMPLEMENT_DYNAMIC(NSFTrackDialog, CDialog)
NSFTrackDialog::NSFTrackDialog(CWnd* pParent /*=NULL*/)
	: CDialog(NSFTrackDialog::IDD, pParent)
{
  frame_of_last_update = -1;
  m_active = false;
/*  for(int i=0;i<NSFPlayer::NES_TRACK_MAX;i++)
    m_showtrk[i]=true;*/

  green_pen.CreatePen(PS_SOLID,1,RGB(0,212,0));
  m_pDCtrk = NULL;
  m_nTimer = 0;
  m_nTimer2 = 0;
  m_rmenu.LoadMenu(IDR_TRKINFOMENU);
}

NSFTrackDialog::~NSFTrackDialog()
{
  if(green_pen.DeleteObject()==0)
    DEBUG_OUT("Error in deleting the green_pen object.\n");
}

#define CONFIG (*(pm->cf))

void NSFTrackDialog::UpdateNSFPlayerConfig(bool b)
{
  if(b)
  {
    m_setup.m_freq_value   = CONFIG["INFO_FREQ"];
    m_setup.m_delay_value  = CONFIG["INFO_DELAY"];
    m_speed.SetPos((256 / (CONFIG["MULT_SPEED"]/8)) - 7);
    m_setup.m_freq_mode    = !(int)CONFIG["FREQ_MODE"];
    m_setup.m_graphic_mode = !(int)CONFIG["GRAPHIC_MODE"];
	m_setup.m_synth_speed_value = CONFIG["SYNTH_SPEED"];
	m_setup.m_drums_speed_value = CONFIG["DRUMS_SPEED"];
    for(int trk=0;trk<m_maxtrk;trk++)
	{
        m_trkinfo.SetCheck(trk,( (CONFIG["MASK"]>>m_trkmap[trk])&0x1) == 0 ? true : false);
	}
  }
  else
  {
    CONFIG["INFO_FREQ"]    = m_setup.m_freq_value;
    CONFIG["INFO_DELAY"]   = m_setup.m_delay_value;
    CONFIG["MULT_SPEED"]   = (256 / (m_speed.GetPos()+7))*8;
    CONFIG["FREQ_MODE"]    = !m_setup.m_freq_mode;
    CONFIG["GRAPHIC_MODE"] = !m_setup.m_graphic_mode;
	CONFIG["SYNTH_SPEED"] = m_setup.m_synth_speed_value;
	CONFIG["DRUMS_SPEED"] = m_setup.m_drums_speed_value;
  }
}

void NSFTrackDialog::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_TRACKINFO, m_trkinfo);
  DDX_Control(pDX, IDC_SPEED, m_speed);
}


BEGIN_MESSAGE_MAP(NSFTrackDialog, CDialog)
  ON_WM_TIMER()
  ON_WM_SHOWWINDOW()
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_TRACKINFO, OnLvnItemchangedTrackinfo)
  ON_BN_CLICKED(IDC_SETUP, OnBnClickedSetup)
  ON_WM_DROPFILES()
  ON_WM_HSCROLL()
  ON_WM_SIZE()
//  ON_WM_SIZECLIPBOARD()
ON_WM_SIZING()
ON_WM_VSCROLL()
ON_COMMAND(ID_COPY, OnCopy)
ON_COMMAND(ID_SETTINGS, OnSettings)
//ON_WM_RBUTTONDOWN()
//ON_WM_RBUTTONUP()
ON_NOTIFY(NM_RCLICK, IDC_TRACKINFO, OnNMRclickTrackinfo)
END_MESSAGE_MAP()


// NSFTrackDialog メッセージ ハンドラ

inline static char *note2string(int note)
{
  static char *notename[12]=
  {
    "C ","C#","D ","D#",
    "E ","F ","F#","G ",
    "G#","A ","A#","B "
  };

  if(note)
    return notename[note%12];
  else
    return "* ";
}

static int keyColMap[NSFPlayer::NES_TRACK_MAX] =
{
  0x0000FF, 0x0000FF, //APU1
  0x00FF00, 0x00FF00, 0x000000, //APU2
  0xFF8000, //FDS
  0x00C0FF, 0x00C0FF, 0x000000, //MMC5
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0x000000, //FME7
  0x0080FF, 0x0080FF, 0x0080FF, //VRC6
  0xFF0080, 0xFF0080, 0xFF0080, 0xFF0080, 0xFF0080, 0xFF0080,//VRC7
  0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF  //N106
};

int ms_to_frame(int time_in_ms, double fps)
{
	return (int)((double)time_in_ms * (fps / 1000.0));
}

int frame_to_ms(int time_in_frames, double fps)
{
	return (int)((double)time_in_frames * (1000.0 / fps));
}

int NSFTrackDialog::octaveAdjustment()
{
	return 12*(m_keydlg.m_keywindow.startingOctave+3);
}

int NSFTrackDialog::GetRegion(xgm::UINT8 flags)
{
    int pref = CONFIG["REGION"];

    // user forced region
    if (pref == 3) return REGION_NTSC;
    if (pref == 4) return REGION_PAL;
    if (pref == 5) return REGION_DENDY;

    // single-mode NSF
    if (flags == 0) return REGION_NTSC;
    if (flags == 1) return REGION_PAL;

    if (flags & 2) // dual mode
    {
        if (pref == 1) return REGION_NTSC;
        if (pref == 2) return REGION_PAL;
        // else pref == 0 or invalid, use auto setting based on flags bit
        return (flags & 1) ? REGION_PAL : REGION_NTSC;
    }

    return REGION_NTSC; // fallback for invalid flags
}

void NSFTrackDialog::OnTimer(UINT nIDEvent)
{
  __super::OnTimer(nIDEvent);

  EnterCriticalSection(&parent->cso);

  if(parent->wa2mod && m_active
     && !parent->wa2mod->GetResetFlag() // track info is not reset yet on Play(), it happens in PlayThread
     )
  {
    int time_in_ms;
    time_in_ms = parent->wa2mod->GetOutputTime();
	m_keydlg.m_keywindow.synth_speed = CONFIG["SYNTH_SPEED"];
    m_keydlg.m_keyheader.drums_speed = CONFIG["DRUMS_SPEED"];
	int delay = (int)CONFIG["INFO_DELAY"];
	int time_in_ms_with_delay = time_in_ms-delay;
	
	int region = GetRegion(pm->sdat->pal_ntsc);
	//double FPS = 1000000.0 / ((region == REGION_NTSC)? pm->sdat->speed_ntsc : pm->sdat->speed_pal); //for vsync off
    //double FPS = 10016068.324 / ((region == REGION_NTSC)? pm->sdat->speed_ntsc : pm->sdat->speed_pal); //for vsync on
	double FPS = ((region == REGION_NTSC) ? 60.0 : 50.0); //seems exactly correct, as frame_render is 735 and 735/44100. but measuring the samples in a wav output indicates an fps of about 60.0024 or slightly higher (up to 60.0036).
	int current_frame = ms_to_frame(time_in_ms_with_delay, FPS);

	if (current_frame < frame_of_last_update)
	{
		frame_of_last_update = 0;
	}

    if(nIDEvent==2)
    {
      for(int trk=0;trk<m_maxtrk;trk++)
      {
        ITrackInfo *ti;
        
        ti = dynamic_cast<ITrackInfo *>(pm->pl->GetInfo(frame_to_ms(current_frame, FPS)+1, m_trkmap[trk]));

        if(ti)
        {
          int key = ti->GetNote(ti->GetFreqHz());
          if(ti->GetKeyStatus()||m_lastkey[m_trkmap[trk]]!=key)
          {
            //m_keepkey[m_trkmap[trk]] = 120/(int)CONFIG["INFO_FREQ"];
            m_keepkey[m_trkmap[trk]] = 0; // BS disabling m_keepkey
            // causes erroneous flicker when freq changes even though channel is muted
            // esp. noticable on VRC6 square (enable shares with high freq register)
            m_lastkey[m_trkmap[trk]] = key;
          }
          else if(m_keepkey[m_trkmap[trk]])
            m_keepkey[m_trkmap[trk]]--;
        }
      }
    }
    else if(nIDEvent==1 && current_frame > frame_of_last_update)
    {
      m_keydlg.Reset();
      for(int trk=0;trk<m_maxtrk;trk++)
      {
		//update channel mask
	    /*if (m_trkinfo.GetCheck(trk))
		{
            CONFIG["MASK"] = CONFIG["MASK"]|(1<<m_trkmap[trk]);
		}
		else
		{
            CONFIG["MASK"] = CONFIG["MASK"]&(~(1<<m_trkmap[trk]));
		}*/
        ITrackInfo *ti;
        int delay = (int)CONFIG["INFO_DELAY"];
		//ti = dynamic_cast<ITrackInfo *>(pm->pl->GetInfo(time_in_ms-delay, m_trkmap[trk]));
        ti = dynamic_cast<ITrackInfo *>(pm->pl->GetInfo(frame_to_ms(current_frame, FPS)+1, m_trkmap[trk]));

        if(ti)
        {
          int i;
          CString str;
          CRect rect;

          // 音量表示
          int vol = ti->GetVolume();
          switch(m_trkmap[trk])
          {
          case NSFPlayer::APU1_TRK0:
          case NSFPlayer::APU1_TRK1:
          case NSFPlayer::APU2_TRK1:
          case NSFPlayer::FME7_TRK0:
          case NSFPlayer::FME7_TRK1:
          case NSFPlayer::FME7_TRK2:
            if(vol & 0x10)
                str.Format("%s%2d",((vol&0x20)?"L":"E"),(vol&=0xF));
            else if(vol>=0)
              str.Format("%2d",(vol&0xF)); 
            else
              str="-";
            break;
          default:
            if(vol>=0)
              str.Format("%2d",ti->GetVolume()); 
            else
              str="-";
            break;
          }
          
          if(1)
          {
            m_trkinfo.SetItem(trk,1,LVIF_TEXT,str,0,0,0,0);
          }
          else if(m_trkinfo.GetTopIndex()<=trk)
          {
            m_trkinfo.GetSubItemRect(trk,1,LVIR_BOUNDS,rect);
            rect.top +=1; rect.bottom -=1; rect.left ++; rect.right --;
            m_pDCtrk->FillSolidRect(rect,RGB(0,0,0));
            rect.top +=3; rect.bottom -=3; rect.left +=1; rect.right -=1;
            rect.right = rect.left + rect.Width()*max(vol,0)/ti->GetMaxVolume();
            m_pDCtrk->FillSolidRect(rect,RGB(0,212,0));
          }

          // 周波数表示
          if(!(int)CONFIG["FREQ_MODE"])
          {
            str.Format("%3X",ti->GetFreq());
            m_trkinfo.SetItem(trk,2,LVIF_TEXT,str,0,0,0,0);
          }
          else
          {
            str.Format("%03.1f",ti->GetFreqHz());
            m_trkinfo.SetItem(trk,2,LVIF_TEXT,str,0,0,0,0);
          }
          
          int key = ti->GetNote(ti->GetFreqHz());

          // キー表示
          if(ti->GetKeyStatus()||m_keepkey[m_trkmap[trk]])
            m_trkinfo.SetItem(trk,3,LVIF_TEXT,note2string(key),0,0,0,0);
          else
            m_trkinfo.SetItem(trk,3,LVIF_TEXT,"",0,0,0,0);

          // オクターブ表示
          if(ti->GetFreqHz()!=0.0)
          {
            str.Format("%d",key/12-4);
            m_trkinfo.SetItem(trk,4,LVIF_TEXT,str,0,0,0,0);
          }
          else
          {
            m_trkinfo.SetItem(trk,4,LVIF_TEXT,"",0,0,0,0);
          }

          // 鍵盤表示
          if(m_trkinfo.GetCheck(trk)&&(ti->GetKeyStatus()||m_keepkey[m_trkmap[trk]]))
          {
            if(m_trkmap[trk]==NSFPlayer::APU2_TRK1)
			{
              m_keydlg.m_keyheader.m_nNoiseStatus[0] = 16-(ti->GetFreq()&0xF);
			  m_keydlg.m_keyheader.m_nNoiseVolume[0] = (255*(ti->GetVolume()&0xF) / ti->GetMaxVolume());
			  m_keydlg.m_keyheader.m_nNoiseTone[0] = (ti->GetTone());
			}
			else if (m_trkmap[trk]==NSFPlayer::FME7_TRK4) //5B noise
			{
			  m_keydlg.m_keyheader.m_n5BNoiseStatus[0] = 32-(ti->GetFreq());
			  m_keydlg.m_keyheader.m_n5BNoiseVolume[0] = (255*(ti->GetVolume()&0xF) / ti->GetMaxVolume());
			}
            else if(m_trkmap[trk]==NSFPlayer::APU2_TRK2)
			{
              m_keydlg.m_keyheader.m_nDPCMStatus[0] = (ti->GetFreq()&0xF)+1;
			  m_keydlg.m_keyheader.m_nDPCMVolume[0] = (255*(ti->GetVolume()&0xF) / ti->GetMaxVolume());
			  m_keydlg.m_keyheader.m_nDPCMTone[0] = (ti->GetTone());
			}
            else if(m_trkmap[trk] == NSFPlayer::APU1_TRK0 || m_trkmap[trk] == NSFPlayer::APU1_TRK1)
			{
			  int volume = 255*(ti->GetVolume()&0xF) / ti->GetMaxVolume();
              m_keydlg.m_keywindow.KeyOn(key-octaveAdjustment(), keyColMap[m_trkmap[trk]]);
			  m_keydlg.m_keywindow.TrackOn(trk, ColorForNote(ti, trk), volume, ti->GetFractionalNote(ti->GetFreqHz())-octaveAdjustment(), 0);
			}
			else if (m_trkmap[trk] == NSFPlayer::FME7_TRK0 || m_trkmap[trk] == NSFPlayer::FME7_TRK1 || m_trkmap[trk] == NSFPlayer::FME7_TRK2 || m_trkmap[trk] == NSFPlayer::FME7_TRK3)
			{
                int volume = 255*(ti->GetVolume()&0xF) / ti->GetMaxVolume();
				if (volume > 0)
				{
					m_keydlg.m_keywindow.KeyOn(key-octaveAdjustment(), keyColMap[m_trkmap[trk]]);
					m_keydlg.m_keywindow.TrackOn(trk, ColorForNote(ti, trk), volume, ti->GetFractionalNote(ti->GetFreqHz())-octaveAdjustment(), 0);
				}
			}
            else
			{
              int volume = m_trkmap[trk] == NSFPlayer::VRC6_TRK2 ? min(ti->GetVolume()*8, 255) : (255*ti->GetVolume() / ti->GetMaxVolume());
			  if (volume > 0 || m_trkmap[trk] == NSFPlayer::APU2_TRK0)
			  {
                 m_keydlg.m_keywindow.KeyOn(key-octaveAdjustment(), keyColMap[m_trkmap[trk]]);
			     m_keydlg.m_keywindow.TrackOn(trk, ColorForNote(ti, trk), volume, ti->GetFractionalNote(ti->GetFreqHz())-octaveAdjustment(), 0);
			  }
			}
          }

          // 音色表示
          if(ti->GetTone()>=0)
          {
            switch(m_trkmap[trk])
            {
            case NSFPlayer::APU2_TRK1:
              str = ti->GetTone()?"P":"N";
              m_trkinfo.SetItem(trk,5,LVIF_TEXT,str,0,0,0,0);
              break;

            case NSFPlayer::APU2_TRK2:
              str.Format("%04X",ti->GetTone());
              m_trkinfo.SetItem(trk,5,LVIF_TEXT,str,0,0,0,0);
              break;

            case NSFPlayer::FME7_TRK0:
            case NSFPlayer::FME7_TRK1:
            case NSFPlayer::FME7_TRK2:
              if(ti->GetTone()==0)
                str.Format("TN");
              else if(ti->GetTone()==1)
                str.Format("T-");
              else if(ti->GetTone()==2)
                str.Format("-N");
              else
                str.Format("--");
              m_trkinfo.SetItem(trk,5,LVIF_TEXT,str,0,0,0,0);
              break;

            case NSFPlayer::FME7_TRK3: // envelope
			  if (ti->GetTone()>0xF)
			  {
				str.Format("V%1X",ti->GetTone()&0xF);
			  }
			  else
			  {
                str.Format("%1X",ti->GetTone());
			  }
              m_trkinfo.SetItem(trk,5,LVIF_TEXT,str,0,0,0,0);
              break;

            default:
              str.Format("%d",ti->GetTone());
              m_trkinfo.SetItem(trk,5,LVIF_TEXT,str,0,0,0,0);
              break;
            }
          }
          else 
            m_trkinfo.SetItem(trk,5,LVIF_TEXT,"-",0,0,0,0);

          switch(m_trkmap[trk])
          {
            case NSFPlayer::FDS_TRK0:
              m_tonestr[NSFPlayer::FDS_TRK0]="";
              for(i=0;i<64;i++)
                m_tonestr[NSFPlayer::FDS_TRK0].AppendFormat("%02d ", ((char)((dynamic_cast<TrackInfoFDS *>(ti)->wave[i])&0xFF)));

              if(!(int)CONFIG["GRAPHIC_MODE"])
              {
                m_trkinfo.SetItem(trk,6,LVIF_TEXT,m_tonestr[NSFPlayer::FDS_TRK0],0,0,0,0);
              }
              else if(m_trkinfo.GetTopIndex()<=trk)
              {
                CPen *oldpen = m_pDCtrk->SelectObject(&green_pen);
                m_trkinfo.GetSubItemRect(trk,6,LVIR_BOUNDS,rect);
                rect.top +=1; // rect.bottom -=1;
                rect.left +=1; rect.right -=4;
                m_pDCtrk->FillSolidRect(rect,RGB(0,0,0));
                rect.bottom -=1;
                int length = min(64, rect.Width());
                //int mid = (rect.top + rect.bottom)/2;

                m_pDCtrk->MoveTo(rect.left,rect.bottom);
                m_pDCtrk->LineTo(rect.left+length,rect.bottom);            
                for(i=0;i<length;i++)
                {
                  m_pDCtrk->MoveTo(rect.left+i, rect.bottom); 
                  m_pDCtrk->LineTo(rect.left+i, rect.bottom-
                     (static_cast<TrackInfoFDS *>(ti)->wave[i] * rect.Height()/64));
                }
                m_pDCtrk->SelectObject(oldpen);
              }
              break;

            case NSFPlayer::N106_TRK0:
            case NSFPlayer::N106_TRK1:
            case NSFPlayer::N106_TRK2:
            case NSFPlayer::N106_TRK3:
            case NSFPlayer::N106_TRK4:
            case NSFPlayer::N106_TRK5:
            case NSFPlayer::N106_TRK6:
            case NSFPlayer::N106_TRK7:
              
              m_tonestr[m_trkmap[trk]]="";
              for(i=0;i<dynamic_cast<TrackInfoN106 *>(ti)->wavelen;i++)
                m_tonestr[m_trkmap[trk]].AppendFormat("%02d ", (static_cast<TrackInfoN106 *>(ti)->wave[i])&0xF);

              if(!(int)CONFIG["GRAPHIC_MODE"])
              {
                m_trkinfo.SetItem(trk,6,LVIF_TEXT,m_tonestr[m_trkmap[trk]],0,0,0,0);
              }
              else if(m_trkinfo.GetTopIndex()<=trk)
              {
                CPen *oldpen = m_pDCtrk->SelectObject(&green_pen);
                m_trkinfo.GetSubItemRect(trk,6,LVIR_BOUNDS,rect);
                rect.top +=1; // rect.bottom -=1;
                rect.left +=1; rect.right -=4;
                m_pDCtrk->FillSolidRect(rect,RGB(0,0,0));
                rect.bottom -=1;
                int length = min(dynamic_cast<TrackInfoN106 *>(ti)->wavelen, rect.Width());
                if (ti->GetVolume() == 0 && length >= 128)
                {
                    // hide waves that are muted and long
                    // (engines frequently wipe most or all of the length register when muted)
                    length = 0;
                }
                m_pDCtrk->MoveTo(rect.left,rect.bottom);
                m_pDCtrk->LineTo(rect.left+length,rect.bottom);
                for(i=0;i<length;i++)
                {
                  m_pDCtrk->MoveTo(rect.left+i, rect.bottom); 
                  m_pDCtrk->LineTo(rect.left+i, rect.bottom -
                     (dynamic_cast<TrackInfoN106 *>(ti)->wave[i] * rect.Height() / 16));
                }
                m_pDCtrk->SelectObject(oldpen);
              }
              break;
            default:
              break;
          }
        }
        else
        {
          for(int i=1;i<7;i++)
            m_trkinfo.SetItem(trk,i,LVIF_TEXT,"",0,0,0,0);
        }
      }

	  if (current_frame - frame_of_last_update > 1) //we missed a frame
	  {
		  int frames_needed = min(current_frame - frame_of_last_update, KeyWindow::MAX_FRAMES);

		  //also fill in old buffer
		  m_keydlg.m_keywindow.numberofframes = frames_needed;
		  m_keydlg.m_keyheader.numberofframes = frames_needed;
		  //d-d-d-doubled logic

		  for(int trk=0;trk<m_maxtrk;trk++)
		  for (int j = 1; j < frames_needed; ++j)
		  {
			ITrackInfo *ti;
			int delay = (int)CONFIG["INFO_DELAY"];
			ti = dynamic_cast<ITrackInfo *>(pm->pl->GetInfo(frame_to_ms(current_frame - (j), FPS)+1, m_trkmap[trk]));
			//ti = dynamic_cast<ITrackInfo *>(pm->pl->GetInfo(frame_to_ms(current_frame-j, 60)+1, m_trkmap[trk]));
			//ti = dynamic_cast<ITrackInfo *>(pm->pl->GetInfo(time_in_ms-delay, m_trkmap[trk]));
			//ti = dynamic_cast<ITrackInfo *>(pm->pl->GetInfo(time_in_ms-delay-frame_to_ms(j, 60)+1, m_trkmap[trk]));

			if(ti)
			{
				CString str;
				CRect rect;

				// 音量表示
				int vol = ti->GetVolume();
          
				int key = ti->GetNote(ti->GetFreqHz());

				if(m_trkinfo.GetCheck(trk)&&(ti->GetKeyStatus()||m_keepkey[m_trkmap[trk]]))
				{
					if(m_trkmap[trk]==NSFPlayer::APU2_TRK1)
					{
						m_keydlg.m_keyheader.m_nNoiseStatus[j] = 16-(ti->GetFreq()&0xF);
						m_keydlg.m_keyheader.m_nNoiseVolume[j] = (255*(ti->GetVolume()&0xF) / ti->GetMaxVolume());
						m_keydlg.m_keyheader.m_nNoiseTone[j] = (ti->GetTone());
					}
					else if (m_trkmap[trk]==NSFPlayer::FME7_TRK4) //5B noise
					{
						m_keydlg.m_keyheader.m_n5BNoiseStatus[j] = 32-(ti->GetFreq());
						m_keydlg.m_keyheader.m_n5BNoiseVolume[j] = (255*(ti->GetVolume()&0xF) / ti->GetMaxVolume());
					}
					else if(m_trkmap[trk]==NSFPlayer::APU2_TRK2)
					{
						m_keydlg.m_keyheader.m_nDPCMStatus[j] = (ti->GetFreq()&0xF)+1;
						m_keydlg.m_keyheader.m_nDPCMVolume[j] = (255*(ti->GetVolume()&0xF) / ti->GetMaxVolume());
						m_keydlg.m_keyheader.m_nDPCMTone[j] = (ti->GetTone());
					}
					else if(m_trkmap[trk] == NSFPlayer::APU1_TRK0 || m_trkmap[trk] == NSFPlayer::APU1_TRK1)
			        {
			            int volume = 255*(ti->GetVolume()&0xF) / ti->GetMaxVolume();
                        m_keydlg.m_keywindow.KeyOn(key-octaveAdjustment(), keyColMap[m_trkmap[trk]]);
			            m_keydlg.m_keywindow.TrackOn(trk, ColorForNote(ti, trk), volume, ti->GetFractionalNote(ti->GetFreqHz())-octaveAdjustment(), j);
					}
					else if (m_trkmap[trk] == NSFPlayer::FME7_TRK0 || m_trkmap[trk] == NSFPlayer::FME7_TRK1 || m_trkmap[trk] == NSFPlayer::FME7_TRK2 || m_trkmap[trk] == NSFPlayer::FME7_TRK3)
			        {
                       int volume = 255*(ti->GetVolume()&0xF) / ti->GetMaxVolume();
					   if (volume > 0)
					   {
                           m_keydlg.m_keywindow.KeyOn(key-octaveAdjustment(), keyColMap[m_trkmap[trk]]);
			               m_keydlg.m_keywindow.TrackOn(trk, ColorForNote(ti, trk), volume, ti->GetFractionalNote(ti->GetFreqHz())-octaveAdjustment(), j);
					   }
					}
			        else
			        {
                       int volume = m_trkmap[trk] == NSFPlayer::VRC6_TRK2 ? min(ti->GetVolume()*8, 255) : (255*ti->GetVolume() / ti->GetMaxVolume());
					   if (volume > 0 || m_trkmap[trk] == NSFPlayer::APU2_TRK0)
					   {
                           m_keydlg.m_keywindow.KeyOn(key-octaveAdjustment(), keyColMap[m_trkmap[trk]]);
			               m_keydlg.m_keywindow.TrackOn(trk, ColorForNote(ti, trk), volume, ti->GetFractionalNote(ti->GetFreqHz())-octaveAdjustment(), j);
					   }
			        }
				}
			}
		}
	  }
	  else
	  {
		  //notify only newest buffer relevant
		  m_keydlg.m_keywindow.numberofframes = 1;
		  m_keydlg.m_keyheader.numberofframes = 1;
	  }

	frame_of_last_update = current_frame;

	m_keydlg.OnTimer(nIDEvent);
  }

  }

  LeaveCriticalSection(&parent->cso);

 
}

#define DESATURATE_MAX 255
//desaturate towards white
static COLORREF RGBdesaturate(byte r, byte g, byte b, int desaturateamount)
{
	if (r > g && r > b)
	{
		g = g + ((255-g)*desaturateamount)/DESATURATE_MAX;
		b = b + ((255-b)*desaturateamount)/DESATURATE_MAX;
	}
	else if (b > r && b > g)
	{
		r = r + ((255-r)*desaturateamount)/DESATURATE_MAX;
		g = g + ((255-g)*desaturateamount)/DESATURATE_MAX;
	}
	else
	{
		r = r + ((255-r)*desaturateamount)/DESATURATE_MAX;
		b = b + ((255-b)*desaturateamount)/DESATURATE_MAX;
	}
	return RGB(r, g, b);
}

//desaturate towards gray
static COLORREF RGBgrayify(byte r, byte g, byte b, int grayamount)
{
	r = r*(255-grayamount)/DESATURATE_MAX + 127*grayamount/DESATURATE_MAX;
	g = g*(255-grayamount)/DESATURATE_MAX + 127*grayamount/DESATURATE_MAX;
	b = b*(255-grayamount)/DESATURATE_MAX + 127*grayamount/DESATURATE_MAX;
	return RGB(r, g, b);
}

COLORREF NSFTrackDialog::ColorForNote(xgm::ITrackInfo *ti, int trk)
{
	//int vol = (255*ti->GetVolume() / ti->GetMaxVolume());
	int tone = ti->GetTone();
	COLORREF result; //= RGB(240, 120, 120);
	int desaturateamount;
	switch(m_trkmap[trk])
	{
		case NSFPlayer::APU1_TRK0:
        case NSFPlayer::APU1_TRK1:
		case NSFPlayer::MMC5_TRK0:
		case NSFPlayer::MMC5_TRK1:
		case NSFPlayer::MMC5_TRK2:
			switch (tone)
			{
				case 0:
					result = RGB(240, 0, 0);
				break;
				case 1:
					result = RGB(240, 240, 0);
				break;
				case 2:
					result = RGB(120, 0, 240);
				break;
				case 3:
					result = RGB(240, 240, 0);
				break;
			}
		break;
		case NSFPlayer::VRC6_TRK0:
		case NSFPlayer::VRC6_TRK1:
			switch (tone)
			{
				case 0:
					result = RGB(240, 0, 120);
				break;
				case 1:
					result = RGB(240, 0, 0);
				break;
				case 2:
					result = RGB(240, 120, 0);
				break;
				case 3:
					result = RGB(240, 240, 0);
				break;
				case 4:
					result = RGB(0, 240, 0);
				break;
				case 5:
					result = RGB(0, 240, 240);
				break;
				case 6:
					result = RGB(0, 0, 240);
				break;
				case 7:
					result = RGB(120, 0, 240);
				break;
			}
		break;
		case NSFPlayer::APU2_TRK0:
			result = RGB(240, 240, 240);
		break;
		case NSFPlayer::VRC7_TRK0:
		case NSFPlayer::VRC7_TRK1:
		case NSFPlayer::VRC7_TRK2:
		case NSFPlayer::VRC7_TRK3:
		case NSFPlayer::VRC7_TRK4:
		case NSFPlayer::VRC7_TRK5:
			desaturateamount = (ti->GetMisc())&0xFF;
			desaturateamount = desaturateamount*DESATURATE_MAX/12;
			switch ((int)(0.0001+(tone / 42.51)))
			{
				//0 magenta, 42 blue
				case 0:
					result = RGBgrayify(255-tone*6,0,255, desaturateamount);
				break;
				//43 blue, 85 cyan
				case 1:
					result = RGBgrayify(0,(tone-43)*6,255, desaturateamount);
				break;
				//86 cyan, 127 green
				case 2:
					result = RGBgrayify(0,255,255-(tone-86)*6, desaturateamount);
				break;
				//128 green, 169 yellow
				case 3:
					result = RGBgrayify((tone-128)*6,255,0, desaturateamount);
				break;
				//170 yellow, 212 orange
				case 4:
					result = RGBgrayify(255,255-(tone-170)*3,0, desaturateamount);
				break;
				//213 orange, 255 red
				case 5:
					result = RGBgrayify(255,128-(tone-213)*3,0, desaturateamount);
				break;
			}
		break;
		case NSFPlayer::FDS_TRK0:
			result = ColorForWave(64, (static_cast<TrackInfoFDS *>(ti)->wave));
		break;
		case NSFPlayer::N106_TRK0:
		case NSFPlayer::N106_TRK1:
		case NSFPlayer::N106_TRK2:
		case NSFPlayer::N106_TRK3:
		case NSFPlayer::N106_TRK4:
		case NSFPlayer::N106_TRK5:
		case NSFPlayer::N106_TRK6:
		case NSFPlayer::N106_TRK7:
			result = ColorForWave((static_cast<TrackInfoN106 *>(ti)->wavelen), (static_cast<TrackInfoN106 *>(ti)->wave));
		break;
		case NSFPlayer::FME7_TRK0:
		case NSFPlayer::FME7_TRK1:
		case NSFPlayer::FME7_TRK2:
			switch (tone)
			{
				case 0: //TN
					return RGB(180, 120, 240); //logical AND of tone and noise 
					break;
				case 1: //T-
					if (ti->GetVolume() == (64|32|1))
					{
						result = RGB(239, 146, 208);
					}
					else if (ti->GetVolume() == (128|32|1))
					{
						result = RGB(236, 240, 177);
					}
					else
					{
						return RGB(120, 0, 240); //tone. if high bit of volume is set, volume is controlled by envelope BTW otherwise it's just volume
					}
					break;
				case 2: //-N
					return RGB(180, 240, 180); //noise only, will be displayed in 5B noise section of KeyHeader instead
					break;
				case 3: //--
					return RGB(40, 40, 40); //black, 'the channel outputs a constant signal at the specified volume'
					break;
			}
		break;
		case NSFPlayer::FME7_TRK3: //env
			if ((tone&0xF) == 0x8 || (tone&0xF) == 0xC) //'strong sawtooth'
			{
				if ((tone&0x10) > 0)
				{
					result = RGB(239, 146, 208);
				}
				else
				{
					result = RGB(240, 90, 150);
				}
			}
			else if ((tone&0xF) == 0xA || (tone&0xF) == 0xE) //'weak sawtooth'
			{
				if ((tone&0x10) > 0)
				{
					result = RGB(236, 240, 177);
				}
				else
				{
					result = RGB(240, 175, 128);
				}
			}
			else //'flick', probably shouldn't draw this
			{
				result = RGB(240, 240, 240);
			}
		break;
		case NSFPlayer::FME7_TRK4: //noise
			result = RGB(180, 240, 180); //will be displayed in 5B noise section of KeyHeader instead
		break;
		default:
			result = RGB(240, 120, 120);
		break;
	}
	return result;
}

COLORREF NSFTrackDialog::ColorForWave(int wavelen, xgm::INT16 wave[])
{
	const COLORREF tricolor = RGB(240, 240, 240);
	const COLORREF sinecolor = RGB(240, 0, 240);

	//triangle wave check
	const xgm::INT16 tri_64_fds[64] = { -32, -30, -28, -26, -24, -22, -20, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0, -2, -4, -6, -8, -10, -12, -14, -16, -18, -20, -22, -24, -26, -28, -30, -32};
	const xgm::INT16 tri_32_n106[32] = { -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8};
	const xgm::INT16 tri_16_n106[16] = { -8, -6, -4, -2, 0, 2, 4, 6, 6, 4, 2, 0, -2, -4, -6, -8};
	const xgm::INT16 tri_8_n106[8] = { -8, -4, 0, 4, 4, 0, -4, -8};

	//sine wave check
	const xgm::INT16 sine_64_fds[64] = { 1, 4, 7, 10, 13, 16, 18, 21, 23, 25, 27, 28, 29, 30, 31, 31, 31, 31, 30, 29, 28, 27, 25, 23, 21, 18, 16, 13, 10, 7, 4, 1, -2, -5, -8, -11, -14, -17, -19, -22, -24, -26, -28, -29, -30, -31, -32, -32, -32, -32, -31, -30, -29, -28, -26, -24, -22, -19, -17, -14, -11, -8, -5, -2};
	const xgm::INT16 sine_32_n106[32] = { 0, 1, 3, 4, 5, 6, 7, 7, 7, 7, 6, 6, 5, 3, 2, 1, -1, -2, -4, -5, -6, -7, -8, -8, -8, -8, -7, -7, -6, -4, -3, -2};
	const xgm::INT16 sine_16_n106[16] = { 0, 3, 5, 7, 7, 6, 5, 2, -1, -4, -6, -8, -8, -7, -6, -3};
	const xgm::INT16 sine_8_n106[8] = { 0, 5, 7, 5, -1, -6, -8, -6};

	if (wavelen == 8)
	{
		if (memcmp(wave, tri_8_n106, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return tricolor;
		}
		else if (memcmp(wave, sine_8_n106, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return sinecolor;
		}
	}
	else if (wavelen == 16)
	{
		if (memcmp(wave, tri_16_n106, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return tricolor;
		}
		else if (memcmp(wave, sine_16_n106, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return sinecolor;
		}
	}
	else if (wavelen == 32) //largest 2a03
	{
		if (memcmp(wave, tri_32_n106, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return tricolor;
		}
		else if (memcmp(wave, sine_32_n106, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return sinecolor;
		}
	}
	else if (wavelen == 64) //fds
	{
		if (memcmp(wave, tri_64_fds, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return tricolor;
		}
		else if (memcmp(wave, sine_64_fds, sizeof(xgm::INT16)*wavelen) == 0)
		{
			return sinecolor;
		}
	}

	//square wave check
	const int TOLERANCE = 4;

	int leftstart = wave[0];
	int rightstart = wave[wavelen-1];
	int leftlength = 1;
	int rightlength = 1;
	int leftlargestdifference = 0;
	int rightlargestdifference = 0;
	for (int i = 1; i < wavelen; ++i)
	{
		int diff = abs(wave[i] - leftstart);
		if (diff < TOLERANCE)
		{
			leftlength++;
			if (diff > leftlargestdifference)
			{
				leftlargestdifference = diff;
			}
		}
		else
		{
			break;
		}
	}

	for (int i = wavelen-2; i > -1; --i)
	{
		int diff = abs(wave[i] - rightstart);
		if (diff < TOLERANCE)
		{
			rightlength++;
			if (diff > rightlargestdifference)
			{
				rightlargestdifference = diff;
			}
		}
		else
		{
			break;
		}
	}

	if (leftlength > wavelen/2 && rightlength > wavelen/2) //not a wave at all!
	{
		return RGB(40, 40, 40); //black
	}
	else if(leftlength < wavelen/2-1 && rightlength < wavelen/2-1) //not a square wave, triangle wave or sine wave
	{
		//note: as coded this means that as long as one side of the wave reaches past the midpoint, it's considered a square wave
		return RGB(240, 120, 120); //pink
	}
	else
	{
		double dutycycle = (double)(wavelen-max(leftlength, rightlength)) / (double)wavelen;
		double dutycycleremainder = dutycycle*16 - (int)(dutycycle*16);
		//same as VRC6, so:
		//RGB(120,	0,	240) at 8/16
		//RGB(0,	0,	240) at 7/16
		//RGB(0,	240,240) at 6/16
		//RGB(0,	240,0) at 5/16
		//RGB(240,	240,0) at 4/16
		//RGB(240,	120,0) at 3/16
		//RGB(240,	0,	0) at 2/16
		//RGB(240,	0,	120) at 1/16
		//RGB(240,	0,	240) at 0/16

		//add saturation based on how bad of a match it was
		int desaturateamount = (leftlargestdifference + rightlargestdifference+max(wavelen-leftlength-rightlength, 2))*DESATURATE_MAX/((TOLERANCE+1)*2);
		switch ((int)(dutycycle*16))
		{
			case 0:
				return RGBgrayify(240, 0, 240-120*dutycycleremainder, desaturateamount);
			break;
			case 1:
				return RGBgrayify(240, 0, 120-120*dutycycleremainder, desaturateamount);
			break;
			case 2:
				return RGBgrayify(240, 120*dutycycleremainder, 0, desaturateamount);
			break;
			case 3:
				return RGBgrayify(240, 120+120*dutycycleremainder, 0, desaturateamount);
			break;
			case 4:
				return RGBgrayify(240-240*dutycycleremainder, 240, 0, desaturateamount);
			break;
			case 5:
				return RGBgrayify(0, 240, 240*dutycycleremainder, desaturateamount);
			break;
			case 6:
				return RGBgrayify(0, 240-240*dutycycleremainder, 240, desaturateamount);
			break;
			case 7:
				return RGBgrayify(120*dutycycleremainder, 0, 240, desaturateamount);
			break;
			case 8:
				return RGBgrayify(120, 0, 240, desaturateamount);
			break;
			default:
				return RGB(128, 255, 128); //should never happen
			break;
		}
	}
}

void NSFTrackDialog::InitList()
{
  NSF *nsf = pm->pl->nsf;
  int i,j; 
  char *tname[][16] = { 
    {"SQR0","SQR1","TRI","NOISE","DMC",NULL},
    {"FDS",NULL},
    {"MMC5:0","MMC5:1","MMC5:P",NULL},
    {"5B:0","5B:1","5B:2","5B:E","5B:N",NULL},
    {"VRC6:0","VRC6:1","VRC6:2",NULL},
    {"VRC7:0", "VRC7:1", "VRC7:2", "VRC7:3", "VRC7:4", "VRC7:5", NULL},
    {"N163:0", "N163:1", "N163:2", "N163:3","N163:4", "N163:5", "N163:6", "N163:7",NULL},
    NULL 
  };

  m_initializing_list = true;
  m_trkinfo.DeleteAllItems();
  int idx=0, num=0;
  for(i=0;tname[i][0]!=NULL;i++)
  {
    for(j=0;tname[i][j]!=NULL;j++)
    {
      m_trkmap[idx]=num++;
      switch(i)
      {
      case 0:
        break;
      case 1:
        if(!nsf->use_fds) continue;
        break;
      case 2:
        if(!nsf->use_mmc5) continue;
        break;
      case 3:
        if(!nsf->use_fme7) continue;
        break;
      case 4:
        if(!nsf->use_vrc6) continue;
        break;
      case 5:
        if(!nsf->use_vrc7) continue;
        break;
      case 6:
        if(!nsf->use_n106) continue;
        break;
      default:
        continue;
      }
      m_trkinfo.InsertItem(idx,tname[i][j]);
	  m_trkinfo.SetCheck(idx,( (CONFIG["MASK"]>>m_trkmap[idx])&0x1) == 0 ? true : false);
      idx++;
    }
  }
  m_maxtrk = idx;
  m_keydlg.m_keywindow.numberoftracks = m_maxtrk;
  m_initializing_list = false;

}

void NSFTrackDialog::LocateDialogItems()
{  
  CRect rect_c, rect1, rect2;

  if(m_keydlg.m_hWnd&&m_trkinfo.m_hWnd)
  {
    GetClientRect(&rect_c);
    rect2.SetRect(0, 0, rect_c.Width(), (rect_c.Width()/14)+m_keydlg.m_keyheader.MaxHeight()+m_keydlg.SynthesiaHeight());
    rect2.MoveToXY(0, rect_c.bottom-rect2.Height());
    m_keydlg.MoveWindow(&rect2);

    m_trkinfo.GetWindowRect(&rect1);
    rect1.right = rect_c.right;
    rect1.left = 0;
    rect1.top = 32;
    rect1.bottom = rect2.top - 1;
    m_trkinfo.MoveWindow(&rect1);
  }
}

BOOL NSFTrackDialog::OnInitDialog()
{
  m_keydlg.m_keywindow.startingOctave = CONFIG["STARTING_OCTAVE"];
  m_keydlg.m_keywindow.synthesiaWidth = CONFIG["SYNTHESIA_WIDTH"];
  m_keydlg.m_keywindow.synthesiaHeight = CONFIG["SYNTHESIA_HEIGHT"];
  m_keydlg.m_keyheader.synthesiaWidth = CONFIG["SYNTHESIA_WIDTH"]; 
  m_keydlg.m_keyheader.drumsHeight = CONFIG["DRUMS_HEIGHT"];
  //m_keydlg.m_keywindow.OnInitDialog();
  //m_keydlg.m_keyheader.OnInitDialog();
  

  __super::OnInitDialog();

  HICON hIcon = AfxGetApp()->LoadIcon(IDI_NSF);
  SetIcon(hIcon, TRUE);

  char *cname[] = { "Track  ", "Vol", "     Freq", "Key", "Oct", "Tone", "Wave", NULL };

  m_pDCtrk = m_trkinfo.GetDC();
  for(int i=0; cname[i]!=NULL; i++)
    m_trkinfo.InsertColumn(i,cname[i],(i==0||i==6)?LVCFMT_LEFT:LVCFMT_RIGHT,i==6?256:m_pDCtrk->GetTextExtent(cname[i]).cx+12+(i?0:12),i);
  m_trkinfo.SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_HEADERDRAGDROP|LVS_EX_DOUBLEBUFFER);
  m_trkinfo.SetBkColor(RGB(0,0,0));
  m_trkinfo.SetTextColor(RGB(0,212,0));
  m_trkinfo.SetTextBkColor(RGB(0,0,0));
  
  m_keydlg.Create(m_keydlg.IDD,this);
  m_keydlg.ShowWindow(SW_SHOW);

  m_speed.SetRange(1,41);
  m_speed.SetTicFreq(4);
  m_speed.SetPageSize(1);
  m_speed.SetLineSize(1);

  CRect rect, rect2;
  GetClientRect(&rect);
  GetWindowRect(&rect2);
  int min_height = rect2.Height()-rect.Height();
  int min_width = rect2.Width()-rect.Width();
  int max_width = rect2.Width()-rect.Width();
  m_keydlg.GetWindowRect(&rect);
  min_height += rect.Height();
  min_width += m_keydlg.MinWidth();
  max_width += m_keydlg.MaxWidth();
  SetWindowPos(NULL,0,0,max_width,256+m_keydlg.m_keyheader.MaxHeight()+m_keydlg.SynthesiaHeight(),SWP_NOMOVE|SWP_NOZORDER);
  LocateDialogItems();

  // setup colors
  for (int i=0; i < NES_CHANNEL_MAX; ++i)
  {
      int t = pm->cf->channel_track[i];
      std::string sc = pm->cf->GetChannelConfig(i,"COL").GetStr();
      int c = read_color(sc.c_str());
      //DEBUG_OUT("Channel %02d colour: %06X (%s)\n", i, c, pm->cf->channel_name[i]);
      if (c >= 0 && c <= 0xFFFFFF) keyColMap[t] = c;
  }
  {
      std::string sc = pm->cf->GetValue("5B_ENVELOPE_COL").GetStr();
      int c = read_color(sc.c_str());
      //DEBUG_OUT("5B envelope colour: %06X\n", c);
      if (c >= 0 && c <= 0xFFFFFF) keyColMap[NSFPlayer::FME7_TRK3] = c;
  }
  {
      std::string sc = pm->cf->GetValue("5B_NOISE_COL").GetStr();
      int c = read_color(sc.c_str());
      //DEBUG_OUT("5B noise colour: %06X\n", c);
      if (c >= 0 && c <= 0xFFFFFF) keyColMap[NSFPlayer::FME7_TRK4] = c;
  }

  return TRUE;  // return TRUE unless you set the focus to a control
  // 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void NSFTrackDialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
  __super::OnShowWindow(bShow, nStatus);

  m_trkinfo.SetFocus(); // in case someone uses mouse scroll before clicking on it,
  // prevents the scroll from doing time expansion instead

  if(bShow==TRUE&&m_nTimer==0)
  {
    m_nTimer = SetTimer(1,1000/(int)CONFIG["INFO_FREQ"], NULL);
    m_nTimer2 = SetTimer(2,1000/120,NULL);
    m_keydlg.Start(1000/(int)CONFIG["INFO_FREQ"]);
  }
  else if(bShow==FALSE&&m_nTimer!=0)
  {
    m_keydlg.Stop();
    KillTimer(m_nTimer);
    KillTimer(m_nTimer2);
    m_nTimer = 0;
  }
}

void NSFTrackDialog::OnLvnItemchangedTrackinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  if(!m_initializing_list)
  {
    if (m_trkinfo.GetCheck(pNMLV->iItem))
	{
		CONFIG["MASK"] = CONFIG["MASK"]&(~(1<<m_trkmap[pNMLV->iItem]));
	}
	else
	{
		CONFIG["MASK"] = CONFIG["MASK"]|(1<<m_trkmap[pNMLV->iItem]);
	}
	parent->UpdateNSFPlayerConfig(true);
	pm->cf->Notify(-1);
    //m_showtrk[m_trkmap[pNMLV->iItem]] = m_trkinfo.GetCheck(pNMLV->iItem);
  }
  *pResult = 0;
}


void NSFTrackDialog::OnBnClickedSetup()
{
  if(m_setup.DoModal()==IDOK)
  {
    for(int i=0;i<m_maxtrk;i++)
    {
      for(int j=1;j<7;j++)
        m_trkinfo.SetItem(i,j,LVIF_TEXT,"",0,0,0,0);
    }

    UpdateNSFPlayerConfig(false);
    if(m_nTimer)
    {
      KillTimer(m_nTimer);
      m_nTimer = SetTimer(1,1000/(int)CONFIG["INFO_FREQ"],NULL);
      m_keydlg.Start(1000/(int)CONFIG["INFO_FREQ"].GetInt());
    }
  }
  else
  {
    UpdateNSFPlayerConfig(true);
  }
}

void NSFTrackDialog::OnDropFiles(HDROP hDropInfo)
{
  CArray<char,int> aryFile;
  UINT nSize, nCount = DragQueryFile(hDropInfo, -1, NULL, 0);

  if(parent->wa2mod)
  {
    parent->wa2mod->ClearList();
    for(UINT i=0;i<nCount;i++)
    {
      nSize = DragQueryFile(hDropInfo, i, NULL, 0);
      aryFile.SetSize(nSize+1);
      DragQueryFile(hDropInfo, i, aryFile.GetData(), nSize+1);
      parent->wa2mod->QueueFile(aryFile.GetData());
    }
    parent->wa2mod->PlayStart();
  }

  __super::OnDropFiles(hDropInfo);
}

void NSFTrackDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  if((CSliderCtrl *)pScrollBar == &m_speed)
  {
    UpdateNSFPlayerConfig(false);
  }

  __super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void NSFTrackDialog::OnSize(UINT nType, int cx, int cy)
{
  __super::OnSize(nType, cx, cy);
  LocateDialogItems();
}

void NSFTrackDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
  __super::OnSizing(fwSide, pRect);

  CRect rect, rect2;
  GetClientRect(&rect);
  GetWindowRect(&rect2);
  int min_height = rect2.Height()-rect.Height();
  int min_width = rect2.Width()-rect.Width();
  int max_width = rect2.Width()-rect.Width();
  m_keydlg.GetWindowRect(&rect);
  min_height += rect.Height();
  min_width += m_keydlg.MinWidth();
  max_width += m_keydlg.MaxWidth();


  switch(fwSide)
  {
  case WMSZ_LEFT:
  case WMSZ_TOPLEFT:
  case WMSZ_BOTTOMLEFT:
    if(pRect->bottom-pRect->top < min_height)
      pRect->top = pRect->bottom - min_height;
    if(pRect->right-pRect->left < min_width)
      pRect->left = pRect->right - min_width;
    if(pRect->right-pRect->left > max_width)
      pRect->left = pRect->right - max_width;
    break;
  case WMSZ_RIGHT:
  case WMSZ_TOPRIGHT:
  case WMSZ_BOTTOMRIGHT:
    if(pRect->bottom-pRect->top < min_height)
      pRect->bottom = pRect->top + min_height;
    if(pRect->right-pRect->left < min_width)
      pRect->right = pRect->left + min_width;
    if(pRect->right-pRect->left > max_width)
      pRect->right = pRect->left + max_width;
    break;

  case WMSZ_TOP:
    if(pRect->bottom-pRect->top < min_height)
      pRect->top = pRect->bottom - min_height;
    break;

  case WMSZ_BOTTOM:
    if(pRect->bottom-pRect->top < min_height)
      pRect->bottom = pRect->top + min_height;
    break;
  default:
    break;
  }
}

void NSFTrackDialog::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  __super::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL NSFTrackDialog::DestroyWindow()
{
  return __super::DestroyWindow();
}

void NSFTrackDialog::OnCopy()
{
  CString str;

  if ( m_trk_selected < 0 || NSFPlayer::NES_TRACK_MAX <= m_trk_selected) return;

  str = m_tonestr[m_trk_selected] + "\r\n";

  if( !::OpenClipboard(NULL) ) 
  {
    MessageBox("Error to open the clipboard!");
	  return;
  }

  HGLOBAL hMem = ::GlobalAlloc(GMEM_FIXED, str.GetLength()+1);
	LPTSTR pMem = (LPTSTR)hMem;
	::lstrcpy(pMem, (LPCTSTR)str);

	::EmptyClipboard();
	::SetClipboardData(CF_TEXT, hMem);
	::CloseClipboard();
}

void NSFTrackDialog::OnSettings()
{
  OnBnClickedSetup();
}

void NSFTrackDialog::OnNMRclickTrackinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) pNMHDR;
  
  CMenu* sub_menu = m_rmenu.GetSubMenu(0);    
  ASSERT(sub_menu);


  if( 0 <= lpnmitem->iItem && lpnmitem->iItem <m_maxtrk )
  {
    m_trk_selected = m_trkmap[lpnmitem->iItem];  
    if( NSFPlayer::FDS_TRK0 == m_trk_selected 
        || (NSFPlayer::N106_TRK0 <= m_trk_selected && m_trk_selected <= NSFPlayer::N106_TRK7))
    {
      CPoint point(lpnmitem->ptAction);
      m_trkinfo.ClientToScreen(&point);
      sub_menu->TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
    }
  }
  
  *pResult = 0;
}
