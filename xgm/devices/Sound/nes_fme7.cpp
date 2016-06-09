#include "nes_fme7.h"

using namespace xgm;

const int DIVIDER = 8; // TODO this is not optimal, rewrite PSG output

NES_FME7::NES_FME7 ()
{
  psg = PSG_new ((e_uint32)DEFAULT_CLOCK, DEFAULT_RATE);

  for(int c=0;c<2;++c)
    for(int t=0;t<3;++t)
      sm[c][t] = 128;
}

NES_FME7::~NES_FME7 ()
{
  if (psg)
    PSG_delete (psg);
}

void NES_FME7::SetClock (double c)
{
  this->clock = c * 2.0;
}

void NES_FME7::SetRate (double r)
{
  //rate = r ? r : DEFAULT_RATE;
  rate = DEFAULT_CLOCK / double(DIVIDER); // TODO rewrite PSG to integrate with clock
  if (psg)
    PSG_set_rate (psg, (e_uint32)rate);
}

void NES_FME7::Reset ()
{
  for (int i=0; i<16; ++i) // blank all registers
  {
    Write(0xC000,i);
    Write(0xE000,0);
  }
  Write(0xC000,0x07); // disable all tones
  Write(0xE000,0x3F);

  divider = 0;
  if (psg)
    PSG_reset (psg);
}

bool NES_FME7::Write (xgm::UINT32 adr, xgm::UINT32 val, xgm::UINT32 id)
{
  if (adr == 0xC000)
  {
    if (psg)
      PSG_writeIO (psg, 0, val);
    return true;
  }
  if (adr == 0xE000)
  {
    if (psg)
      PSG_writeIO (psg, 1, val);
    return true;
  }
  else
    return false;
}

bool NES_FME7::Read (xgm::UINT32 adr, xgm::UINT32 & val, xgm::UINT32 id)
{
  // not sure why this was here - BS
  //if (psg)
  //  val = PSG_readIO (psg);

  return false;
}

void NES_FME7::Tick (xgm::UINT32 clocks)
{
  divider += clocks;
  while (divider >= DIVIDER)
  {
      divider -= DIVIDER;
      if (psg) PSG_calc(psg);
  }
}

xgm::UINT32 NES_FME7::Render (xgm::INT32 b[2])
{
  b[0] = b[1] = 0;

  for (int i=0; i < 3; ++i)
  {
    // note negative polarity
    b[0] -= psg->cout[i] * sm[0][i];
    b[1] -= psg->cout[i] * sm[1][i];
  }
  b[0] >>= (7-4);
  b[1] >>= (7-4);

  // master volume adjustment
  const INT32 MASTER = INT32(0.64 * 256.0);
  b[0] = (b[0] * MASTER) >> 8;
  b[1] = (b[1] * MASTER) >> 8;

  return 2;
}

void NES_FME7::SetStereoMix(int trk, xgm::INT16 mixl, xgm::INT16 mixr)
{
      if (trk < 0) return;
      if (trk > 2) return;
      sm[0][trk] = mixl;
      sm[1][trk] = mixr;
}

ITrackInfo *NES_FME7::GetTrackInfo(int trk)
{
  assert(trk<5);

  if(psg)
  {
    if (trk<3)
    {
      trkinfo[trk]._freq = psg->freq[trk];
      if(psg->freq[trk])
        trkinfo[trk].freq = psg->clk/32.0/psg->freq[trk];
      else
        trkinfo[trk].freq = 0;

      trkinfo[trk].output = psg->cout[trk];
      trkinfo[trk].max_volume = 15;
	  if (!(psg->volume[trk] & 32) || (psg->tmask[trk]))
	  {
		trkinfo[trk].volume = psg->volume[trk] >> 1;
	  }
	  else
	  {
		if ((psg->clk/512.0/psg->env_freq) > 10.0 && (psg->volume[trk] & 32) && !(psg->tmask[trk]) && psg->env_continue && !psg->env_hold) //volume modulation by envelope has created a new instrument!
		{
			trkinfo[trk].volume = 1 | 32 | (psg->env_alternate ? 128 : 64);
		}
		else
		{
			trkinfo[trk].volume = psg->env_ptr >> 1;
		}
	  }
      trkinfo[trk].key = !(psg->tmask[trk]);
      trkinfo[trk].tone = (psg->tmask[trk]?2:0)+(psg->nmask[trk]?1:0);
    }
    else if (trk == 3) // envelope
    {
      trkinfo[trk]._freq = psg->env_freq;
      if(psg->env_freq)
        trkinfo[trk].freq = psg->clk/512.0/psg->env_freq;
      else
        trkinfo[trk].freq = 0;

      if (psg->env_continue && psg->env_alternate && !psg->env_hold) // triangle wave
      {
        trkinfo[trk].freq *= 0.5f; // sounds an octave down
      }
      
      trkinfo[trk].output = psg->voltbl[psg->env_ptr];
      trkinfo[trk].max_volume = 15; //was 0
	  trkinfo[trk].volume = (psg->env_continue == 0 || psg->env_hold == 1) ? 0 : 8; //was 0
      trkinfo[trk].key = (((psg->volume[0]|psg->volume[1]|psg->volume[2])&32) != 0) || (psg->tmask[0]&&psg->nmask[0]) || (psg->tmask[1]&&psg->nmask[1]) || (psg->tmask[2]&&psg->nmask[2]);
      trkinfo[trk].tone =
		  ((psg->clk/512.0/psg->env_freq) > 10.0 && ((psg->volume[0] & 32 && !(psg->tmask[0])) || (psg->volume[1] & 32 && !(psg->tmask[01])) || (psg->volume[2] & 32 && !(psg->tmask[2])) ) && psg->env_continue && !psg->env_hold ? 16:0) | //new flag for volume modulated instrument
          (psg->env_continue ?8:0) |
          (psg->env_attack   ?4:0) |
          (psg->env_alternate?2:0) |
          (psg->env_hold     ?1:0) ;
    }
    else if (trk == 4) // noise
    {
      trkinfo[trk]._freq = psg->noise_freq >> 1;
      if(trkinfo[trk]._freq > 0)
        trkinfo[trk].freq = psg->clk/16.0/psg->noise_freq;
      else
        trkinfo[trk].freq = 0;

      trkinfo[trk].output = psg->noise_seed & 1;
      trkinfo[trk].max_volume = 15; //was 0
	  trkinfo[trk].volume = 0;
	  trkinfo[trk].key = false;
	  for (int i = 0; i < 3; ++i)
	  {
		if (psg->nmask[i] == 0)
		{
	      if ((psg->volume[i] & 32) != 0) //envelope is volume modulating noise via this channel
		  {
			trkinfo[trk].volume = psg->env_ptr >> 1;
			trkinfo[trk].key = true;
			break;
	      }
		  else if (psg->volume[i] != 0) //directly playing noise on this channel using its normal volume
		  {
		    trkinfo[trk].volume = (psg->volume[i] >> 1)&15;
			trkinfo[trk].key = true;
			break;
	      }
		}
	  }
      trkinfo[trk].tone = 0;
    }
  }
  return &trkinfo[trk];
}
