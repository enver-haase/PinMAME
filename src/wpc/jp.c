/************************************************************************************************
 Juegos Populares (Spain)
 ------------------------
   NOTES:
   This system doesn't seem to have any kind of diagnostic tests, which makes emulation a pain in the a..
   Also, the display board is just being fed 3 serial bits, and everything else goes from there.
   The external sound board gets its morse-code style sound commands from an undocumented lamp output!!!

   Hardware:
   ---------
		CPU:     Z80 @ 4 MHz
			INT: IRQ @ 977 Hz (4MHz/2048/2)
		IO:      DMA, AY8910 input ports
		DISPLAY: 7-digit 8-segment panels with direct segment access, driven by 4x 4094 serial controllers.
		SOUND:	 AY8910 @ 2 MHz on CPU board,
		         Z80 CPU with ADPCM chip (probably an MSM5205 @ 384 kHz) on separate board.
 ************************************************************************************************/
//Games: America 1492 1986, Aqualand 1986, Faeton 1985, Halley Comet 1986,
//       Lortium 1987, Olympus 1986, Petaco 1984

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "jp.h"
#include "sound/ay8910.h"
#include "sndbrd.h"
#include "machine/4094.h"

#define JP_VBLANKFREQ   60 /* VBLANK frequency */
#define JP_CPUFREQ 4000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount, solCount;
  UINT32 solenoids;
  core_tSeg segments;
  UINT32 dispData;
  int    swCol, lampCol;
} locals;

static INTERRUPT_GEN(JP_irq) {
  cpu_set_irq_line(JP_CPU, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(JP_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % JP_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((++locals.solCount % JP_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = 0;
  }
  /*-- display --*/
  if ((locals.vblankCount % JP_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));
    memset(locals.segments, 0xff, sizeof(locals.segments));
  }

  core_updateSw(core_getSol(7));
}

static SWITCH_UPDATE(JP) {
#ifdef MAME_DEBUG
  static char s[4];
  static int sndcmd = 3;
  int i;
  if (keyboard_pressed_memory_repeat(KEYCODE_Z, 4) && sndcmd > 3) {
    sndcmd--;
    sprintf(s, "%2d", sndcmd);
    core_textOut(s, 2, 35, 5, 5);
    for (i=0; i < sndcmd; i++) {
      cpu_set_nmi_line(1, PULSE_LINE);
    }
  } else if (keyboard_pressed_memory_repeat(KEYCODE_X, 4) && sndcmd < 0x20) {
    sndcmd++;
    sprintf(s, "%2d", sndcmd);
    core_textOut(s, 2, 35, 5, 5);
    for (i=0; i < sndcmd; i++) {
      cpu_set_nmi_line(1, PULSE_LINE);
    }
  }
#endif /* MAME_DEBUG */
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x7e, 4);
  }
}

// bits 24 to 27 decide about the lit segment (4 bits decoded by a 4028 chip),
// the rest of the bits enable the single digits (therefore 28 digits possible).
static void dispStrobe(void) {
  static int pos[32] = { 31, 30, 29, 28, 26, 25, 24, 23, 22, 19, 18, 17, 16, 15,
    12, 11, 10, 9, 8, 5, 4, 3, 2, 1, 33, 34, 35, 36, 21, 14, 7, 0 };
  int i, digit = (locals.dispData >> 24) & 0x0f;
  switch (digit) {
  case 9: case 10: case 11: case 12: case 13: case 14: case 15:
    logerror("Undocumented write to display register %d!\n", digit);
    break;
  case 8:
    // number of balls: data is bit 6 - 13 reversed
    locals.segments[32].w = core_revbyte((locals.dispData >> 6) ^ 0xff);
    // LEDs: strobed by bits 14 - 23
    coreGlobals.tmpLampMatrix[14] = core_revbyte((locals.dispData >> 14) ^ 0xff);
    coreGlobals.tmpLampMatrix[15] = core_revbyte(~(locals.dispData >> 22)) >> 6;
    break;
  case 7:
    // all comma segments
    for (i=0; i < 32; i++)
      if (!locals.dispData & (1 << i)) locals.segments[pos[i]].w &= ~0x80;
    break;
  default:
    // all player scores, match & credits displays
    for (i=0; i < 32; i++)
      if (locals.dispData & (1 << i)) locals.segments[pos[i]].w &= ~(1 << (6-digit));
    // fake single points zero digits
    for (i=0; i < 4; i++)
      locals.segments[i*7 + 6].w = (locals.segments[i*7 + 5].w) ? core_bcd2seg7[0] : 0;
  }
}

// The schematics won't tell, but the order should be:
// DATA, CLOCK, LATCH, X0, X1, X2, X3, X4
static WRITE_HANDLER(disp_w) {
  data ^= 0xff;
  // top 5 bits: switch column strobes
  locals.swCol = core_BitColToNum(data >> 3);
  // bottom 3 bits: serial display data
  HC4094_data_w (0, GET_BIT0);
  HC4094_strobe_w(0, GET_BIT2);
  HC4094_strobe_w(1, GET_BIT2);
  HC4094_strobe_w(2, GET_BIT2);
  HC4094_strobe_w(3, GET_BIT2);
  HC4094_clock_w(0, GET_BIT1);
  HC4094_clock_w(1, GET_BIT1);
  HC4094_clock_w(2, GET_BIT1);
  HC4094_clock_w(3, GET_BIT1);
  dispStrobe();
}

// As usually on spanish-made games, lamps and solenoids are heavily mixed up...
static WRITE_HANDLER(lamp1_w) {
  if (offset == 0 && (data & 0x07)) {
    locals.solCount = 0;
    locals.solenoids |= (data & 0x07) << 8;
  } else if (offset == 2 && (data & 0x07)) {
    locals.solCount = 0;
    locals.solenoids |= (data & 0x07) << 11;
  } else if (offset == 4 && (data & 0x03)) {
    locals.solCount = 0;
    locals.solenoids |= (data & 0x03) << 14;
  }
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(lamp2_w) {
  // morse code; pulses the NMI of the external sound board CPU a couple of times!
  if (offset == 5 && (data & 0x02)) {
    cpu_set_nmi_line(1, PULSE_LINE);
  }
  coreGlobals.tmpLampMatrix[6+offset] = data;
}

static WRITE_HANDLER(sol_w) {
  locals.solCount = 0;
  locals.solenoids |= data;
}

static WRITE_HANDLER(parallel_0_out) {
  locals.dispData = (locals.dispData & 0xffffff00) | data;
}
static WRITE_HANDLER(parallel_1_out) {
  locals.dispData = (locals.dispData & 0xffff00ff) | (data << 8);
}
static WRITE_HANDLER(parallel_2_out) {
  locals.dispData = (locals.dispData & 0xff00ffff) | (data << 16);
}
static WRITE_HANDLER(parallel_3_out) {
  locals.dispData = (locals.dispData & 0x00ffffff) | (data << 24);
}
static WRITE_HANDLER(qs1pin_0_out) {
  HC4094_data_w(1, data);
}
static WRITE_HANDLER(qs1pin_1_out) {
  HC4094_data_w(2, data);
}
static WRITE_HANDLER(qs1pin_2_out) {
  HC4094_data_w(3, data);
}

static HC4094interface hc4094jp = {
  4, // 4 chips
  { parallel_0_out, parallel_1_out, parallel_2_out, parallel_3_out },
  { 0 },
  { qs1pin_0_out, qs1pin_1_out, qs1pin_2_out }
};

static WRITE_HANDLER(ay8910_ctrl_w) { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_data_w) { AY8910Write(0,1,data); }
static READ_HANDLER (ay8910_r)      { return AY8910Read(0); }

// Funny: they use the ports of the AY8910 sound chip as switch inputs! :)
static READ_HANDLER (ay8910_portA_r) {
	if (locals.swCol == 4)
		return ~core_getDip(2);
	else if (locals.swCol)
		return ~coreGlobals.swMatrix[4+locals.swCol];
	else
		return ~core_getDip(1);
}
static READ_HANDLER (ay8910_portB_r) {
	if (locals.swCol)
		return ~coreGlobals.swMatrix[locals.swCol];
	else
		return ~core_getDip(0);
}

struct AY8910interface JP_ay8910Int = {
	1,					/* 1 chip */
	2000000,			/* 2 MHz */
	{ 30 },				/* Volume */
	{ ay8910_portA_r },	/* Input Port A callback */
	{ ay8910_portB_r },	/* Input Port B callback */
};

static MEMORY_READ_START(JP_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x47ff, MRA_RAM},
  {0x6001,0x6001, ay8910_r},
MEMORY_END

static MEMORY_WRITE_START(JP_writemem)
  {0x4000,0x47ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x6000,0x6000, ay8910_ctrl_w},
  {0x6002,0x6002, ay8910_data_w},
  {0xa000,0xa000, sol_w},
  {0xa001,0xa001, disp_w},
  {0xa002,0xa007, lamp1_w},
  {0xc000,0xc007, lamp2_w},
MEMORY_END

static MACHINE_INIT(JP) {
  memset(&locals, 0, sizeof locals);
  HC4094_init(&hc4094jp);
  HC4094_oe_w(0, 1);
  HC4094_oe_w(1, 1);
  HC4094_oe_w(2, 1);
  HC4094_oe_w(3, 1);
}

MACHINE_DRIVER_START(JP)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, JP_CPUFREQ)
  MDRV_CPU_MEMORY(JP_readmem, JP_writemem)
  MDRV_CPU_VBLANK_INT(JP_vblank, 1)
  MDRV_CPU_PERIODIC_INT(JP_irq, JP_CPUFREQ / 4096)
  MDRV_CORE_INIT_RESET_STOP(JP,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(20)
  MDRV_SWITCH_UPDATE(JP)
  MDRV_SOUND_ADD(AY8910, JP_ay8910Int)
MACHINE_DRIVER_END

// petaco: different hardware generation

static MEMORY_READ_START(JP2_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x6000,0x67ff, MRA_RAM},
  {0x8000,0x8000, ay8910_r},
MEMORY_END

static MEMORY_WRITE_START(JP2_writemem)
  {0x6000,0x67ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x8000,0x8000, ay8910_ctrl_w},
  {0x8001,0x8001, ay8910_data_w},
MEMORY_END

static READ_HANDLER(dip_r) {
  return ~core_getDip(offset);
}

static READ_HANDLER(sw_r) {
  return ~coreGlobals.swMatrix[core_BitColToNum(locals.swCol)];
}

static PORT_READ_START(JP2_readport)
  {0x08,0x0a, dip_r},
  {0x0d,0x0d, sw_r},
MEMORY_END

static WRITE_HANDLER(lampcol_w) {
	locals.lampCol = data;
}

static WRITE_HANDLER(swcol_w) {
	locals.swCol = data;
}

static WRITE_HANDLER(lampdata_w) {
	coreGlobals.tmpLampMatrix[locals.lampCol] = data;
}

static PORT_WRITE_START(JP2_writeport)
  {0x0c,0x0c, lampcol_w},
  {0x0e,0x0e, lampdata_w},
//  {0x10,0x10, col3_w},
//  {0x12,0x12, data3_w},
  {0x14,0x14, swcol_w},
//  {0x18,0x18, data2_w},
MEMORY_END

MACHINE_DRIVER_START(JP2)
  MDRV_IMPORT_FROM(JP)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(JP2_readmem, JP2_writemem)
  MDRV_CPU_PORTS(JP2_readport, JP2_writeport)
  MDRV_DIPS(24)
MACHINE_DRIVER_END


// external sound board section

static MACHINE_INIT(JPS) {
  machine_init_JP();
  /* init sound */
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(JP_MEMREG_SND),NULL,NULL);
}

static MACHINE_STOP(JPS) {
  sndbrd_0_exit();
}

static INTERRUPT_GEN(JPS_irq) {
  cpu_set_irq_line(1, 0, PULSE_LINE);
}

/* MSM5205 interrupt callback */
static UINT8 mdata;
static int which;
static void JP_msmIrq(int data) {
	MSM5205_data_w(0, (which = !which) ? mdata >> 4 : mdata & 0x0f);
}

static struct MSM5205interface JP_msm5205Int = {
	1,					//# of chips
	384000,				//Clock Frequency
	{JP_msmIrq},		//VCLK Int. Callback
	{MSM5205_S48_4B},	//Sample Mode
	{60}				//Volume
};

static WRITE_HANDLER(bank_w) {
//logerror("bank_w: %02x\n", data);
  cpu_setbank(1, memory_region(REGION_SOUND1) + 0x8000 * (data & 0x0f));
}

static WRITE_HANDLER(snd_w) {
//logerror("snd_w: %02x\n", data);
  mdata = data;
  which = 0;
}

static WRITE_HANDLER(enable_w) {
//logerror("enable_w: %02x\n", data);
  MSM5205_reset_w(0, data & 1);
}

static MEMORY_READ_START(jpsnd_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x47ff, MRA_RAM},
  {0x8000,0xffff, MRA_BANKNO(1) },
MEMORY_END

static MEMORY_WRITE_START(jpsnd_writemem)
  {0x4000,0x47ff, MWA_RAM},
  {0x5000,0x5000, bank_w},
  {0x6000,0x6000, snd_w},
  {0x7000,0x7000, enable_w},
MEMORY_END

MACHINE_DRIVER_START(JPS)
  MDRV_IMPORT_FROM(JP)
  MDRV_CPU_ADD_TAG("scpu", Z80, JP_CPUFREQ)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(jpsnd_readmem, jpsnd_writemem)
  MDRV_CPU_PERIODIC_INT(JPS_irq, 4000)
  MDRV_CORE_INIT_RESET_STOP(JPS,NULL,JPS)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(MSM5205, JP_msm5205Int)
MACHINE_DRIVER_END
