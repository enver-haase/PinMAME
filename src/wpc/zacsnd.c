#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6502/m6502.h"
#include "cpu/i8039/i8039.h"
#include "cpu/tms7000/tms7000.h"
#include "sound/discrete.h"
#include "sound/sn76477.h"
#include "core.h"
#include "sndbrd.h"
#include "zacsnd.h"

#define SHOW_MAME_BUG 0
/*
  In order to demonstrate a MAME / mingw bug,
  set the define above to 1, recompile and
  start tmachzac. Enter test mode, go to sound test (#5).
  Sound #28 ("Play the Time Machine!") will not play,
  but only produce some garbage sound.
  This is due to this single line, which may not be
  executed for proper behaviour:

    pia_set_input_a(SNS_PIA1, (snslocals.pia1a = tms5220_status_r(0)));

  so if the status value is stored in a variable before the PIA port is set,
  instead of setting it right from the value itself, sound will crash!
*/

extern WRITE_HANDLER(UpdateZACSoundLED);
extern void UpdateZACSoundACT(int data);

/*----------------------------------------
/ Zaccaria Sound-On-Board 1311
/ 4 simple tone generators, solenoid-controlled
/-----------------------------------------*/
DISCRETE_SOUND_START(zac1311_discInt)
	DISCRETE_INPUT(NODE_01,1,0x000f,0)                         // Input handlers, mostly for enable
	DISCRETE_INPUT(NODE_02,2,0x000f,0)
	DISCRETE_INPUT(NODE_04,4,0x000f,0)
	DISCRETE_INPUT(NODE_08,8,0x000f,0)

	DISCRETE_SAWTOOTHWAVE(NODE_10,NODE_01,349,50000,10000,0,0) // F note
	DISCRETE_SAWTOOTHWAVE(NODE_20,NODE_02,440,50000,10000,0,0) // A note
	DISCRETE_SAWTOOTHWAVE(NODE_30,NODE_04,523,50000,10000,0,0) // C' note
	DISCRETE_SAWTOOTHWAVE(NODE_40,NODE_08,698,50000,10000,0,0) // F' note

	DISCRETE_ADDER4(NODE_50,1,NODE_10,NODE_20,NODE_30,NODE_40) // Mix all four sound sources

	DISCRETE_OUTPUT(NODE_50, 50)                               // Take the output from the mixer
DISCRETE_SOUND_END

MACHINE_DRIVER_START(zac1311)
  MDRV_SOUND_ADD(DISCRETE, zac1311_discInt)
MACHINE_DRIVER_END

const struct sndbrdIntf zac1311Intf = {0};

/*----------------------------------------
/ Zaccaria Sound Board 1125
/ SN76477 sound chip, no CPU
/-----------------------------------------*/
static void zac1125_init(struct sndbrdData *brdData);
static WRITE_HANDLER(zac1125_data_w);

static struct SN76477interface  zac1125_sn76477Int = { 1, { 50 }, /* mixing level */
/*						   pin description		*/
	{ RES_K(47)   },	/*	4  noise_res		*/
	{ RES_K(220)  },	/*	5  filter_res		*/
	{ CAP_N(2.2)  },	/*	6  filter_cap		*/
	{ RES_M(1.5)  },	/*	7  decay_res		*/
	{ CAP_U(2.2)  },	/*	8  attack_decay_cap */
	{ RES_K(4.7)  },	/* 10  attack_res		*/
	{ RES_K(47)   },	/* 11  amplitude_res	*/
	{ RES_K(320)  },	/* 12  feedback_res 	*/
	{ 0	/* ??? */ },	/* 16  vco_voltage		*/
	{ CAP_U(0.33) },	/* 17  vco_cap			*/
	{ RES_K(100)  },	/* 18  vco_res			*/
	{ 5.0		  },	/* 19  pitch_voltage	*/
	{ RES_M(1)    },	/* 20  slf_res			*/
	{ CAP_U(2.2)  },	/* 21  slf_cap			*/
	{ CAP_U(2.2)  },	/* 23  oneshot_cap		*/
	{ RES_M(1.5)  }		/* 24  oneshot_res		*/
};

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1125Intf = {
  "ZAC1125", zac1125_init, NULL, NULL, zac1125_data_w, zac1125_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(zac1125)
  MDRV_SOUND_ADD(SN76477, zac1125_sn76477Int)
MACHINE_DRIVER_END

static struct {
  UINT8 ctrl;
  double ne555_voltage;
  mame_timer *ne555;
} s1125locals;

static void ne555_timer(int n) {
  s1125locals.ne555_voltage += 0.02;
  SN76477_set_vco_voltage(0, s1125locals.ne555_voltage);
  if (s1125locals.ne555_voltage > 4.99) // stop timer if maximum voltage is reached
    timer_adjust(s1125locals.ne555, TIME_NEVER, 0, TIME_NEVER);
}

static WRITE_HANDLER(zac1125_data_w) {
  static double states[8][5] = { // pins 7, 18, 20, 24, and optional timer interval
    { RES_K(468), RES_K(100),RES_M(1), RES_K(9.9) },
    { RES_K(4.7), RES_K(32), RES_M(1), RES_K(136) },
    { RES_K(468), RES_K(25), RES_M(1), RES_K(9.9) },
    { RES_K(468), RES_K(32), RES_M(1), RES_K(9.9) },
    { RES_K(192), RES_K(4.5),RES_M(1), RES_M(1.5), 0.015 },
    { RES_K(26.5),RES_K(4.5),RES_M(1), RES_K(9.9), 0.0003 },
    { RES_M(1.5), RES_K(100),RES_M(1), RES_K(358) },
    { RES_K(192), RES_K(9.1),RES_K(32),RES_K(94)  }
  };
  static int vco[8] = { 1, 1, 1, 1, 0, 0, 1, 1 };
  static int mix[8] = { 0, 0, 0, 0, 0, 0, 2, 0 };

  data &= 0x0f;
  if (data) {
    int state = data >> 1;
    SN76477_enable_w       (0, 1);
    logerror("Sound %x plays\n", state);
    SN76477_mixer_w        (0, mix[state]);
    SN76477_set_decay_res  (0, states[state][0]); /* 7 */
    SN76477_set_vco_res    (0, states[state][1]); /* 18 */
    SN76477_set_slf_res    (0, states[state][2]); /* 20 */
    SN76477_set_oneshot_res(0, states[state][3]); /* 24 */
    SN76477_vco_w          (0, vco[state]);
    if (!vco[state]) { // simulate the loading of the capacitors by using a timer
      SN76477_set_vco_voltage(0, 0);
      s1125locals.ne555_voltage = 0;
      timer_adjust(s1125locals.ne555, states[state][4], 0, states[state][4]);
    }
    SN76477_enable_w       (0, 0);
  }
}

static void zac1125_init(struct sndbrdData *brdData) {
  /* MIXER A & C = GND */
  SN76477_mixer_w(0, 0);
  /* ENVELOPE is constant: pin1 = hi, pin 28 = lo */
  SN76477_envelope_w(0, 1);
  /* fake: pulse the enable line to get rid of the constant noise */
//  SN76477_enable_w(0, 1);
//  SN76477_enable_w(0, 0);
  s1125locals.ne555 = timer_alloc(ne555_timer);
}

/*----------------------------------------
/ Zaccaria Sound Board 1346
/ i8035 MCU, no PIAs
/ and sound board 1146
/ with additional SN76477 chip used on Locomotion only
/-----------------------------------------*/
static void sp_init(struct sndbrdData *brdData);
static void sp_diag(int button);
static WRITE_HANDLER(sp1346_data_w);

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1346Intf = {
  "ZAC1346", sp_init, NULL, sp_diag, sp1346_data_w, sp1346_data_w, NULL, NULL, NULL, 0
};
static struct DACinterface sp1346_dacInt = { 1, { 25 }};

static struct {
  struct sndbrdData brdData;
  int lastcmd;
  int tc;
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  splocals.brdData = *brdData;
}

static void sp_diag(int button) {
  cpu_set_irq_line(ZACSND_CPUA, 0, button ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(sp1346_data_w) {
  logerror("Sound %x plays\n", data);
  splocals.tc = data ? -1 : 0;
  i8035_set_reg(I8035_TC, splocals.tc);

  if (Machine->drv->sound[1].sound_type) { // >0 means SN74677 & NE555 chips present
    static double res[8] = { 0, RES_M(4.7), RES_M(2), RES_K(820), RES_K(680), RES_K(560), RES_K(470), RES_K(330) };
    static int currentValue = 0;
    switch (data) {
    case 0:
      if (splocals.lastcmd == 0) {
        SN76477_mixer_w(0, 7);
        currentValue = 0;
      }
      break;
    case 1: // raise the frequency if value < 7
      if (currentValue < 7) currentValue++;
      break;
    case 2: // lower the frequency if value > 1
      if (currentValue > 1) currentValue--;
      break;
    }
    if (currentValue > 0) {
      SN76477_mixer_w(0, 4);
      SN76477_set_slf_res(0, res[currentValue]);
    }
    SN76477_enable_w(0, !currentValue);
    discrete_sound_w(1, data == 3); // enable the NE555 tone
  }

  splocals.lastcmd = data;
}

static MEMORY_READ_START(i8035_readmem)
  { 0x0000, 0x07ff, MRA_ROM },
  { 0x0800, 0x087f, MRA_RAM },
  { 0x0880, 0x08ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(i8035_writemem)
  { 0x0800, 0x087f, MWA_RAM },
MEMORY_END

static WRITE_HANDLER(ram_w) {
  UINT8 *rom = memory_region(ZACSND_CPUAREGION);
  rom[0x800 + offset] = data;
}
static READ_HANDLER(ram_r) {
  UINT8 *rom = memory_region(ZACSND_CPUAREGION);
  return rom[0x800 + offset];
}
static READ_HANDLER(sp1346_data_r) { return (core_getDip(0) << 4) | splocals.lastcmd; }
static READ_HANDLER(test_r) { return splocals.tc; }
static WRITE_HANDLER(dac_w) { DAC_0_data_w(0, core_revbyte(data)); }

static PORT_READ_START(i8035_readport)
  { 0x00, 0x7f, ram_r },
  { 0x80, 0xff, sp1346_data_r },
  { I8039_t1, I8039_t1, test_r },
MEMORY_END

static PORT_WRITE_START(i8035_writeport)
  { 0x00, 0x7f, ram_w },
  { I8039_p1, I8039_p1, dac_w },
MEMORY_END

MACHINE_DRIVER_START(zac1346)
  MDRV_CPU_ADD_TAG("scpu", I8035, 6000000/15) // 8035 has internal divider by 15!
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(i8035_readmem, i8035_writemem)
  MDRV_CPU_PORTS(i8035_readport, i8035_writeport)

  MDRV_SOUND_ADD(DAC, sp1346_dacInt)
  MDRV_INTERLEAVE(500)
MACHINE_DRIVER_END

static struct SN76477interface  zac1146_sn76477Int = { 1, { 30 }, /* mixing level */
/*						   pin description		*/
	{ RES_K(39)   },	/*	4  noise_res		*/
	{ RES_K(100)  },	/*	5  filter_res		*/
	{ CAP_P(470)  },	/*	6  filter_cap		*/
	{ RES_M(1)    },	/*	7  decay_res		*/
	{ CAP_U(22)   },	/*	8  attack_decay_cap */
	{ RES_K(4.7)  },	/* 10  attack_res		*/
	{ RES_K(47)   },	/* 11  amplitude_res	*/
	{ RES_K(87.7) },	/* 12  feedback_res 	*/
	{ 0           },	/* 16  vco_voltage		*/
	{ 0           },	/* 17  vco_cap			*/
	{ 0           },	/* 18  vco_res			*/
	{ 5.0		  },	/* 19  pitch_voltage	*/
	{ 0           },	/* 20  slf_res			*/
	{ CAP_N(220)  },	/* 21  slf_cap			*/
	{ CAP_U(1)    },	/* 23  oneshot_cap		*/
	{ RES_K(330)  }		/* 24  oneshot_res		*/
};

static int type[1] = {0};
DISCRETE_SOUND_START(zac1146_discInt)
  DISCRETE_INPUT(NODE_01,1,0x0003,0)
  DISCRETE_555_ASTABLE(NODE_10,NODE_01,12.0,RES_K(1),RES_K(56),CAP_N(10),NODE_NC,type)
  DISCRETE_GAIN(NODE_20,NODE_10,1250)
  DISCRETE_OUTPUT(NODE_20, 50)
DISCRETE_SOUND_END

MACHINE_DRIVER_START(zac1146)
  MDRV_IMPORT_FROM(zac1346)
  MDRV_SOUND_ADD(SN76477, zac1146_sn76477Int)
  MDRV_SOUND_ADD(DISCRETE, zac1146_discInt)
MACHINE_DRIVER_END

/*----------------------------------------
/ Zaccaria Sound & Speech Board 1B1370
/ (almost identical to BY61, TMS5200 chip)
/ and Zaccaria Sound & Speech Board 1B13136
/ (like 1B1370, with an additional 6802 CPU, DAC, and AY8910 chip).
/ It's reported that the sound board is missing the 2nd CPU & ROM chips,
/ so in reality it acts exactly like the older 1370 board
/ with a slightly different memory map (but it can be integrated).

/ Also Sound & Speech board 1B11178
/ which is a modified 1B1370 with only one PIA for the TMS chip,
/ also with a single DAC and some analog VCO components (CEM 3372/3374)
/ to replace the AY8910 chip.
/ The memory map aligns, and most of the signal lines too,
/ so it was easier to integrate it into the 1B1370 structure
/ than duplicating most of the existing code...
/-----------------------------------------*/
#define SNS_PIA0 0
#define SNS_PIA1 1
#define SNS_PIA2 2

#define TMS11178_IRQFREQ 3580000.0/8192.0

static void sns_init(struct sndbrdData *brdData);
static void sns_diag(int button);
static WRITE_HANDLER(sns_data_w);
static void sns_5220Irq(int state);
static READ_HANDLER(sns_8910a_r);
static WRITE_HANDLER(sns_8910b_w);
static READ_HANDLER(sns2_8910a_r);
static WRITE_HANDLER(sns_dac_w);

static WRITE_HANDLER(dacxfer);
static WRITE_HANDLER(storelatch);
static WRITE_HANDLER(storebyte1);
static WRITE_HANDLER(storebyte2);
static READ_HANDLER(readlatch);
static READ_HANDLER(readcmd);
static READ_HANDLER(read5000);

static INTERRUPT_GEN(tms_irq);

const struct sndbrdIntf zac1370Intf = {
  "ZAC1370", sns_init, NULL, sns_diag, sns_data_w, sns_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf zac13136Intf = {
  "ZAC13136", sns_init, NULL, sns_diag, sns_data_w, sns_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf zac11178Intf = {
  "ZAC11178", sns_init, NULL, sns_diag, sns_data_w, sns_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct TMS5220interface sns_tms5220Int = { 668000, 100, sns_5220Irq }; // the frequency may vary by up to 30 percent!!!
static struct DACinterface     sns_dacInt = { 1, { 20 }};
static struct DACinterface     sns2_dacInt = { 2, { 20, 20 }};
static struct AY8910interface  sns_ay8910Int = { 1, 3580000/4, {25}, {sns_8910a_r}, {0}, {0}, {sns_8910b_w}};
static struct AY8910interface  sns2_ay8910Int = { 2, 3580000/4, {25, 25}, {sns_8910a_r, sns2_8910a_r}, {0}, {0}, {sns_8910b_w}};

static MEMORY_READ_START(sns_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNS_PIA0) },
  { 0x0084, 0x0087, pia_r(SNS_PIA2) }, // 13136 only
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x1800, 0x1800, sns2_8910a_r }, // 13136 only
  { 0x2000, 0x2000, sns_8910a_r },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sns_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNS_PIA0) },
  { 0x0084, 0x0087, pia_w(SNS_PIA2) }, // 13136 only
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
  { 0x1000, 0x1000, sns_dac_w },
  { 0x4000, 0x4000, DAC_1_data_w }, // 13136 only (never accessed)
MEMORY_END

static MEMORY_READ_START(sns3_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x00b0, 0x00b0, readcmd},
  { 0x00ff, 0x00ff, readlatch },
//  { 0x5000, 0x5000, read5000 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sns3_writemem)
  { 0x0000, 0x007f, MWA_RAM },
//  { 0x0080, 0x0087, },
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
//  { 0x00a0, 0x00a7, },
  { 0x00c0, 0x00c0, dacxfer },
  { 0x00d0, 0x00d0, storebyte1 },
  { 0x00e0, 0x00e0, storebyte2 },
  { 0x00f0, 0x00f0, storelatch },
MEMORY_END

static MACHINE_DRIVER_START(zac1370_nosound)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns_readmem, sns_writemem)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac1370)
  MDRV_IMPORT_FROM(zac1370_nosound)
  MDRV_SOUND_ADD(AY8910,  sns_ay8910Int)
  MDRV_SOUND_ADD(DAC,     sns_dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac13136)
  MDRV_IMPORT_FROM(zac1370_nosound)
  MDRV_SOUND_ADD(AY8910,  sns2_ay8910Int)
  MDRV_SOUND_ADD(DAC,     sns2_dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11178)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns3_readmem, sns3_writemem)
  MDRV_CPU_PERIODIC_INT(tms_irq, TMS11178_IRQFREQ)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
MACHINE_DRIVER_END

static READ_HANDLER(sns_pia0a_r);
static WRITE_HANDLER(sns_pia0a_w);
static WRITE_HANDLER(sns_pia0b_w);
static WRITE_HANDLER(sns_pia0ca2_w);
static WRITE_HANDLER(sns_pia0cb2_w);
static READ_HANDLER(sns_pia1a_r);
static WRITE_HANDLER(sns_pia1a_w);
static WRITE_HANDLER(sns_pia1b_w);
static READ_HANDLER(sns_pia1ca2_r);
static READ_HANDLER(sns_pia1cb1_r);
static READ_HANDLER(sns_pia2a_r);
static WRITE_HANDLER(sns_pia2a_w);
static WRITE_HANDLER(sns_pia2b_w);
static WRITE_HANDLER(sns_pia2ca2_w);

static void sns_irq0a(int state);
static void sns_irq0b(int state);
static void sns_irq1a(int state);
static void sns_irq1b(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a, pia1b, pia1cb1, pia2a, pia2b;
  UINT8 lastcmd, daclatch, dacbyte1, dacbyte2;
  int dacMute, sndReturn;
} snslocals;

static const struct pia6821_interface sns_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia0a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sns_pia0a_w, sns_pia0b_w, sns_pia0ca2_w, sns_pia0cb2_w,
  /*irq: A/B           */ sns_irq0a, sns_irq0b
},{
#if SHOW_MAME_BUG
  /*i: A/B,CA/B1,CA/B2 */ sns_pia1a_r, 0, 0, sns_pia1cb1_r, sns_pia1ca2_r, 0,
#else
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, sns_pia1cb1_r, sns_pia1ca2_r, 0,
#endif
  /*o: A/B,CA/B2       */ sns_pia1a_w, sns_pia1b_w, 0, 0,
  /*irq: A/B           */ sns_irq1a, sns_irq1b
},{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia2a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sns_pia2a_w, sns_pia2b_w, sns_pia2ca2_w, 0,
  /*irq: A/B           */ 0, 0
}};

static void sns_init(struct sndbrdData *brdData) {
  snslocals.brdData = *brdData;
  pia_config(SNS_PIA0, PIA_STANDARD_ORDERING, &sns_pia[0]);
  pia_config(SNS_PIA1, PIA_STANDARD_ORDERING, &sns_pia[1]);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC13136) {
    snslocals.pia1a = 0xff;
    pia_config(SNS_PIA2, PIA_STANDARD_ORDERING, &sns_pia[2]);
  }
  if (core_gameData->hw.soundBoard & 0x02) { // true for all 11178
    UpdateZACSoundLED(1, 1);
  }
}

static void sns_diag(int button) {
  cpu_set_nmi_line(ZACSND_CPUA, button ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(tms_irq) {
  snslocals.pia1cb1 = !snslocals.pia1cb1;
  pia_set_input_cb1(SNS_PIA1, snslocals.pia1cb1);
}

static WRITE_HANDLER(sns_dac_w) {
  if (core_gameData->hw.soundBoard != SNDBRD_ZAC1370 || !snslocals.dacMute) DAC_0_data_w(offset, data);
}

static READ_HANDLER(sns_pia0a_r) {
  if ((snslocals.pia0b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}
static WRITE_HANDLER(sns_pia0a_w) {
  snslocals.pia0a = data;
  if (snslocals.pia0b & 0x02) AY8910Write(0, snslocals.pia0b ^ 0x01, snslocals.pia0a);
}
static WRITE_HANDLER(sns_pia0b_w) {
  if ((data & 0xf0) != (snslocals.pia0b & 0xf0)) logerror("DAC1408 modulation: %x\n", data >> 4);
  snslocals.pia0b = data;
  if (snslocals.pia0b & 0x02) AY8910Write(0, snslocals.pia0b ^ 0x01, snslocals.pia0a);
}
static WRITE_HANDLER(sns_pia0ca2_w) {
  UpdateZACSoundLED(1, data);
} // diag led
static WRITE_HANDLER(sns_pia0cb2_w) {
  snslocals.dacMute = data;
} // mute DAC

static READ_HANDLER(sns_pia1a_r) { return snslocals.pia1a; }
static WRITE_HANDLER(sns_pia1a_w) { snslocals.pia1a = data; }
static WRITE_HANDLER(sns_pia1b_w) {
  if (~data & 0x02) { // write
    tms5220_data_w(0, snslocals.pia1a);
    pia_set_input_ca2(SNS_PIA1, 1); pia_set_input_ca2(SNS_PIA1, 0);
  }
  if (~data & 0x01) { // read
#if SHOW_MAME_BUG
    pia_set_input_a(SNS_PIA1, (snslocals.pia1a = tms5220_status_r(0)));
#else
    pia_set_input_a(SNS_PIA1, tms5220_status_r(0));
#endif
    pia_set_input_ca2(SNS_PIA1, 1); pia_set_input_ca2(SNS_PIA1, 0);
  }
  if ((data & 0xf0) != (snslocals.pia1b & 0xf0)) logerror("TMS5200 modulation: %x\n", data >> 4);
  snslocals.pia1b = data;
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC11178) {
    UpdateZACSoundACT(data & 0x04 ? 0 : 0x03);	//both ACTSND & ACTSPK inverted on bit 2
  } else if (core_gameData->hw.soundBoard == SNDBRD_ZAC11178_13181) {
    UpdateZACSoundACT(data & 0x04 ? 0 : 0x02);	//only ACTSND inverted on bit 2, ACTSPK not used?
  } else {
    UpdateZACSoundACT((data>>2) & 0x03);	//ACTSPK & ACTSND on bits 2 & 3
  }
}
static READ_HANDLER(sns_pia1ca2_r) {
//logerror("sns_pia1ca2_r TMS5220 ready \n");
  return tms5220_ready_r();
}
static READ_HANDLER(sns_pia1cb1_r) {
  if (core_gameData->hw.soundBoard & 0x02) // true for all 11178
    return snslocals.pia1cb1;
  else
    return tms5220_int_r();
}

static READ_HANDLER(sns_pia2a_r) {
  if ((snslocals.pia2b & 0x03) == 0x01) return AY8910Read(1);
  return 0;
}
static WRITE_HANDLER(sns_pia2a_w) {
  snslocals.pia2a = data;
  if (snslocals.pia2b & 0x02) AY8910Write(1, snslocals.pia2b ^ 0x01, snslocals.pia2a);
}
static WRITE_HANDLER(sns_pia2b_w) {
  snslocals.pia2b = data;
  if (snslocals.pia2b & 0x02) AY8910Write(1, snslocals.pia2b ^ 0x01, snslocals.pia2a);
}
static WRITE_HANDLER(sns_pia2ca2_w) {
    UpdateZACSoundLED(2, data);
} // diag led

static WRITE_HANDLER(sns_data_w) {
  snslocals.lastcmd = data;
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370)
    pia_set_input_cb1(SNS_PIA0, data & 0x80 ? 1 : 0);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC13136)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC11178)
    pia_set_input_ca1(SNS_PIA1, data & 0x80 ? 1 : 0);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC11178_13181) {
    // CA1 should connect to GND according to schematics... or to DB7! :)
    pia_set_input_ca1(SNS_PIA1, data & 0x80 ? 1 : 0);
    if (~data & 0x40) cpu_set_nmi_line(ZACSND_CPUB, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
//    logerror("sns_data_w command stored %x\n", data);
// cpu reads command from adress b0 after nmi !!!
  }
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC13181x3) {
    if (~data & 0x40) cpu_set_nmi_line(ZACSND_CPUA, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
    if (~data & 0x40) cpu_set_nmi_line(ZACSND_CPUB, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
    if (data & 0x40) cpu_set_nmi_line(ZACSND_CPUC, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
  }
}

static READ_HANDLER(sns_8910a_r) { return ~snslocals.lastcmd; }

static WRITE_HANDLER(sns_8910b_w) {
  static UINT8 lastAy = 0;
  if ((lastAy & 0x0f) != (data & 0x0f)) logerror("AY8910  modulation: %x\n", data & 0x0f);
  lastAy = data;
}

static READ_HANDLER(sns2_8910a_r) { return ~snslocals.lastcmd; }

static void sns_irq0a(int state) {
  logerror("sns_irq0a: state=%x\n",state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void sns_irq0b(int state) {
  logerror("sns_irq0b: state=%x\n",state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void sns_irq1a(int state) {
  logerror("sns_irq1a: state=%x\n",state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
  if (core_gameData->hw.soundBoard & 0x02) // true for 11178
    cpu_set_nmi_line(ZACSND_CPUA, state ? ASSERT_LINE : CLEAR_LINE);
}
static void sns_irq1b(int state) {
  logerror("sns_irq1b: state=%x\n",state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370 || core_gameData->hw.soundBoard & 0x02)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void sns_5220Irq(int state) {
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370 || core_gameData->hw.soundBoard == SNDBRD_ZAC13136) {
    snslocals.pia1cb1 = !state;
    pia_set_input_cb1(SNS_PIA1, snslocals.pia1cb1);
  }
}

// OK: the following addresses are only used by the 11178 sound board variants
static READ_HANDLER(readcmd) {
  logerror("readcmd: %x\n", snslocals.lastcmd ^ 0xff);
  return snslocals.lastcmd ^ 0xff;
}
static READ_HANDLER(readlatch) {
  logerror("readlatch: %x\n", snslocals.daclatch);
  return snslocals.daclatch;
}
static WRITE_HANDLER(storelatch) {
  snslocals.daclatch = data;
  logerror("Storelatch: %x\n", data);
}
static WRITE_HANDLER(storebyte1) {
  snslocals.dacbyte1 = snslocals.daclatch;
  logerror("Storebyte1: %x\n", snslocals.dacbyte1);
}
static WRITE_HANDLER(storebyte2) {
  snslocals.dacbyte2 = snslocals.daclatch;
  logerror("Storebyte2: %x\n", snslocals.dacbyte2);
}
static WRITE_HANDLER(dacxfer) {
// a 12-bit-DAC is used to create the voltage for the VCO.
  logerror("dacxfer: %x\n", (snslocals.dacbyte1 << 4) | (snslocals.dacbyte2 >> 4));
}
static READ_HANDLER(read5000) {
  return snslocals.sndReturn;
}

/*----------------------------------------
/ Sound & Speech board 1B11178 with 1B13181 / 1B11181 daughter board.
/ This daughter board is equipped with a Z80 processor, and
/ plays background music (all done with DACs).
/ The earlier 1B13181/0 single DAC version was used on Zankor only,
/ Spooky already has the full 3 DAC board 1B11181/1.
/
/ Also Sound & Speech board 1B11183 with three Z80 CPUs, used on Star's Phoenix.
/ One CPU does the exact same stuff like on the 1B11181 daughter board,
/ the 2nd CPU handles speech and effects, and the 3rd one produces
/ some siren-like effects (not played back in sound test).
/ That's five DAC chips playing at the same time! :)
/-----------------------------------------*/
static WRITE_HANDLER(snd_act_w);
static WRITE_HANDLER(snd_mod_w);

static struct DACinterface     z80_1dacInt = { 1, { 50 }};
static struct DACinterface     z80_3dacInt = { 3, { 20, 30, 30 }};
static struct DACinterface     z80_5dacInt = { 5, { 20, 30, 30, 20, 20 }};

static MEMORY_READ_START(z80_readmem)
  { 0x0000, 0xfbff, MRA_ROM },
  { 0xfc00, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(z80_writemem)
  { 0xfc00, 0xffff, MWA_RAM },
MEMORY_END

static WRITE_HANDLER(DAC_2_signed_data_w) { DAC_signed_data_w(2, data); }
static WRITE_HANDLER(DAC_3_signed_data_w) { DAC_signed_data_w(3, data); }
static WRITE_HANDLER(DAC_4_signed_data_w) { DAC_signed_data_w(4, data); }

static PORT_READ_START(z80_readport)
  { 0x01, 0x01, readcmd },
PORT_END

static PORT_WRITE_START(z80_writeport_a)
  { 0x00, 0x00, DAC_4_signed_data_w },
  { 0x02, 0x02, snd_mod_w },
PORT_END
static PORT_WRITE_START(z80_writeport_b)
  { 0x00, 0x00, DAC_0_signed_data_w },
  { 0x02, 0x03, snd_mod_w },
  { 0x04, 0x04, DAC_1_signed_data_w },
  { 0x08, 0x08, DAC_2_signed_data_w },
PORT_END

static PORT_READ_START(z80_readport_c)
  { 0x01, 0x01, readcmd },
  { 0x03, 0x03, tms5220_status_r },
PORT_END
static PORT_WRITE_START(z80_writeport_c)
  { 0x00, 0x00, DAC_3_signed_data_w },
  { 0x02, 0x02, snd_act_w },
  { 0x03, 0x03, tms5220_data_w },
PORT_END

static INTERRUPT_GEN(cpu_b_irq) { cpu_set_irq_line(ZACSND_CPUB, 0, PULSE_LINE); }
static INTERRUPT_GEN(cpu_c_irq) { cpu_set_irq_line(ZACSND_CPUC, 0, PULSE_LINE); }

static MACHINE_DRIVER_START(zac11178_13181_nodac)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns3_readmem, sns3_writemem)
  MDRV_CPU_PERIODIC_INT(tms_irq, TMS11178_IRQFREQ)

  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport, z80_writeport_b)
  MDRV_CPU_PERIODIC_INT(cpu_b_irq, 60)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11178_13181)
  MDRV_IMPORT_FROM(zac11178_13181_nodac)
  MDRV_SOUND_ADD(DAC,     z80_1dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11178_11181)
  MDRV_IMPORT_FROM(zac11178_13181_nodac)
  MDRV_SOUND_ADD(DAC,     z80_3dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11183)
  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport, z80_writeport_a)

  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport, z80_writeport_b)

  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport_c, z80_writeport_c)
  MDRV_CPU_PERIODIC_INT(cpu_c_irq, 100)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(DAC,     z80_5dacInt)
MACHINE_DRIVER_END

static WRITE_HANDLER(snd_act_w) {
  logerror("cpu #%d ACT:%d:%02x\n", cpu_getexecutingcpu(), offset, data);
  // ACTSPK & ACTSND
  UpdateZACSoundACT(data & 0x01 ? 0 : 0x02);	// only ACTSND inverted on bit 0, ACTSPK not used?
}

static WRITE_HANDLER(snd_mod_w) {
  // sound modulation for different chips maybe?
  logerror("cpu #%d MOD:%d:%02x\n", cpu_getexecutingcpu(), offset, data);
}



/* TECHNOPLAY sound board -

   Pretty much stole the Gottlieb System 80b Generation 1 sound board, plus..
   added a TMS7000 and an extra DAC for additional sounds */

/*----------------
/  Local variables
/-----------------*/
static struct {
  struct sndbrdData brdData;
  int    ay_latch;			// Data Latch to AY-8913 chips
  int    nmi_rate;			// Programmable NMI rate
  void   *nmi_timer;		// Timer for NMI (NOT USED ANYMORE?)
  int	 nmi_enable;		// Enable NMI triggered by Programmable Circuit
  int    snd_data;			// Command from cpu
  UINT8  dac_volume;
  UINT8  dac_data;
  UINT8  speechboard_drq;
  UINT8  sp0250_latch;
} techno_locals;

// Latch data for AY chips
WRITE_HANDLER(techno_ay8910_latch_w)
{
	techno_locals.ay_latch = data;
}

//NMI Timer - Setup the next frequency for the timer to fire, and Trigger an NMI if enabled
static void nmi_callback(int param)
{
	//Reset the timing frequency
	double interval;
	int cl1, cl2;
	cl1 = 16-(techno_locals.nmi_rate&0x0f);
	cl2 = 16-((techno_locals.nmi_rate&0xf0)>>4);
	interval = (250000>>8);
	if(cl1>0)	interval /= cl1;
	if(cl2>0)	interval /= cl2;

	//Set up timer to fire again
	timer_set(TIME_IN_HZ(interval), 0, nmi_callback);

	//If enabled, fire the NMI for the Y CPU
	if(techno_locals.nmi_enable) {
		//seems to have no effect!
		//cpu_boost_interleave(TIME_IN_USEC(10), TIME_IN_USEC(800));
		cpu_set_nmi_line(ZACSND_CPUA, PULSE_LINE);
	}
}

WRITE_HANDLER(techno_nmi_rate_w)
{
	techno_locals.nmi_rate = data;
	logerror("NMI RATE SET TO %d\n",data);
}

//Fire the NMI for the 2nd 6502
WRITE_HANDLER(techno_cause_dac_nmi_w)
{
	cpu_set_nmi_line(ZACSND_CPUB, PULSE_LINE);
}

READ_HANDLER(techno_cause_dac_nmi_r)
{
	techno_cause_dac_nmi_w(offset, 0);
	return 0;
}

//Latch a command into the Sound Latch and generate the IRQ interrupts
WRITE_HANDLER(techno_sh_w)
{
	techno_locals.snd_data = data;
	cpu_set_irq_line(ZACSND_CPUA, 0, HOLD_LINE);
	cpu_set_irq_line(ZACSND_CPUB, 0, HOLD_LINE);
	//Bit 6 if NOT set, fires TMS IRQ 1
	if(~data & 0x40) cpu_set_irq_line(ZACSND_CPUC, TMS7000_IRQ1_LINE, ASSERT_LINE);
}

/* bits 0-3 are probably unused (future expansion) */
/* bits 4 & 5 are two dip switches. Unused? */
/* bit 6 is the test switch. When 0, the CPU plays a pulsing tone. */
/* bit 7 comes from the speech chip DATA REQUEST pin */
READ_HANDLER(techno_sound_input_r)
{
	int data = 0x40;
	if(techno_locals.speechboard_drq)	data |= 0x80;
	return data;
}

//Common to All Generations - Set NMI Timer Enable
static WRITE_HANDLER( common_sound_control_w )
{
	techno_locals.nmi_enable = data&0x01;
}

//Generation 1 sound control
WRITE_HANDLER( techno_sound_control_w )
{
	static int last;

	common_sound_control_w(offset, data);

	/* bit 2 goes to 8913 BDIR pin  */
	if ((last & 0x04) == 0x04 && (data & 0x04) == 0x00)
	{
		/* bit 3 selects which of the two 8913 to enable */
		if (data & 0x08)
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_0_w(0,techno_locals.ay_latch);
			else
				AY8910_write_port_0_w(0,techno_locals.ay_latch);
		}
		else
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_1_w(0,techno_locals.ay_latch);
			else
				AY8910_write_port_1_w(0,techno_locals.ay_latch);
		}
	}

	/* bit 5 goes to the speech chip DIRECT DATA TEST pin */
	//NO IDEA WHAT THIS DOES - No interface in the sp0250 emulation for it yet?

	/* bit 6 = speech chip DATA PRESENT pin; high then low to make the chip read data */
	if ((last & 0x40) == 0x40 && (data & 0x40) == 0x00)
	{
		sp0250_w(0,techno_locals.sp0250_latch);
	}

	/* bit 7 goes to the speech chip RESET pin */
	//No interface in the sp0250 emulation for it yet?
	last = data & 0x44;
}

// Init
static void tsns_init(struct sndbrdData *brdData) {
	memset(&techno_locals, 0, sizeof(techno_locals));
	techno_locals.brdData = *brdData;
	//Start the programmable timer circuit
	timer_set(TIME_IN_HZ(250000>>8), 0, nmi_callback);
	//Set bank
	cpu_setbank(1, techno_locals.brdData.romRegion + 0x10000);
	//Must be 1 to start or speechboard will never work
	techno_locals.speechboard_drq = 1;
}

// Cleanup
void tsns_exit(int boardNo)
{
	if(techno_locals.nmi_timer)
		timer_remove(techno_locals.nmi_timer);
	techno_locals.nmi_timer = NULL;
}

static WRITE_HANDLER(tsns_data_w) {
  data ^= 0xff;	/*Data is inverted from main cpu*/

  //Bit 7 is the strobe - so commands are sent 2x, once with strobe set, then again, with it cleared..
  //Doesn't seem to matter much if we put this in or leave it out..
  //if(data & 0x80)
	//  return;

  logerror("tsns_data_w = %x\n",data);
  techno_sh_w(0,data);
}

static READ_HANDLER(techno_snd_r)
{
//	cpu_set_irq_line(ZACSND_CPUA, 0, CLEAR_LINE);
	return techno_locals.snd_data;
}

static READ_HANDLER(techno_b_snd_r)
{
//	cpu_set_irq_line(ZACSND_CPUB, 0, CLEAR_LINE);
	return techno_locals.snd_data;
}

//Read data command
READ_HANDLER(tms_porta_r)
{
	int data = techno_locals.snd_data & 0x1f;	//Only bits 0-4 used
	logerror("reading porta =%x\n",data);
	return data;
}

//Should not be used for input
READ_HANDLER(tms_portb_r)
{
	logerror("reading portb\n");
	return 0;
}
//Should not be used since it's used for address and data lines
READ_HANDLER(tms_portc_r)
{
	logerror("reading portc\n");
	return 0;
}
//Should not be used since it's used for address and data lines
READ_HANDLER(tms_portd_r)
{
	logerror("reading portd\n");
	return 0;
}

//Should not be used for output?
WRITE_HANDLER(tms_porta_w) { logerror("writing port a = %x\n",data); }

/*
D0 = U25 ROM Select
D1 = U36 ROM Select
D2 = NA?
D3 = NA?
D4-D7 = Used for Bus Control
*/
WRITE_HANDLER(tms_portb_w)
{
	cpu_setbank(1, techno_locals.brdData.romRegion + (0x10000) + ((data&2)>>1)*0x8000);
}

//should not be used since it's used for address and data lines
WRITE_HANDLER(tms_portc_w){	logerror("writing port c = %x\n",data); }
WRITE_HANDLER(tms_portd_w){	logerror("writing port d = %x\n",data); }

//Speechboard IRQ callback, will be set to 1 while speech is busy..
static void speechboard_drq_w(int level)
{
	techno_locals.speechboard_drq = (level == ASSERT_LINE);
}

//Latch data to the SP0250
static WRITE_HANDLER(sp0250_latch) {
	techno_locals.sp0250_latch = data;
}

//DAC Handling.. Set volume
static WRITE_HANDLER( dac_vol_w )
{
	techno_locals.dac_volume = data;
	DAC_data_16_w(0, techno_locals.dac_volume * techno_locals.dac_data);
}
//DAC Handling.. Set data to send
static WRITE_HANDLER( dac_data_w )
{
	techno_locals.dac_data = data;
	DAC_data_16_w(0, techno_locals.dac_volume * techno_locals.dac_data);
}

//TMS7000 will use dac chip #1
static WRITE_HANDLER(DAC_TMS_w)
{
	DAC_data_w(1,data);
}

const struct sndbrdIntf technoIntf =
{"TECHNO", tsns_init, tsns_exit, sns_diag, tsns_data_w, tsns_data_w, NULL, NULL, NULL, 0 //SNDBRD_NODATASYNC
};

struct AY8910interface techno_ay8910Int = {
	2,			/* 2 chips */
	2000000,	/* 2 MHz */
	{ 25, 25 }, /* Volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

struct DACinterface techno_6502dacInt =
{
  2,			/*2 Chips */
 {50,50}		/* Volume */
};

struct sp0250_interface techno_sp0250_interface =
{
	100,				/*Volume*/
	speechboard_drq_w	/*IRQ Callback*/
};

//6502 #1 CPU
MEMORY_READ_START( m6502_readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6000, 0x6000, techno_sound_input_r },
	{ 0xa800, 0xa800, techno_snd_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( m6502_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, sp0250_latch },	/* speech chip. The game sends strings */
										/* of 15 bytes (clocked by 4000). The chip also */
										/* checks a DATA REQUEST bit in 6000. */
	{ 0x4000, 0x4000, techno_sound_control_w },
	{ 0x8000, 0x8000, techno_ay8910_latch_w },
	{ 0xa000, 0xa000, techno_nmi_rate_w },	   /* set Y-CPU NMI rate */
	{ 0xb000, 0xb000, techno_cause_dac_nmi_w }, /*Trigger D-CPU NMI*/
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END

//6502 #2 CPU
MEMORY_READ_START( m6502_b_readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x8000, 0x8000, techno_b_snd_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( m6502_b_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x4000, dac_vol_w},
	{ 0x4001, 0x4001, dac_data_w},
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END

//TMS7000 CPU
static MEMORY_READ_START(tms_readmem)
  { 0x0000, 0x7fff, MRA_BANK1 },		/* ROM BANK */
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END
static MEMORY_WRITE_START(tms_writemem)
  { 0x0000, 0x7fff, MWA_ROM },
  { 0x8000, 0x8000, DAC_TMS_w },		/* DAC */
MEMORY_END

static PORT_READ_START(tms_readport)
  { TMS7000_PORTA, TMS7000_PORTA, tms_porta_r },
  { TMS7000_PORTB, TMS7000_PORTB, tms_portb_r },
  { TMS7000_PORTA, TMS7000_PORTC, tms_portc_r },
  { TMS7000_PORTB, TMS7000_PORTD, tms_portd_r },
PORT_END
static PORT_WRITE_START(tms_writeport)
  { TMS7000_PORTA, TMS7000_PORTA, tms_porta_w },
  { TMS7000_PORTB, TMS7000_PORTB, tms_portb_w },
  { TMS7000_PORTA, TMS7000_PORTC, tms_portc_w },
  { TMS7000_PORTB, TMS7000_PORTD, tms_portd_w },
PORT_END

MACHINE_DRIVER_START(techno)
  MDRV_CPU_ADD(M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(m6502_readmem, m6502_writemem)
  MDRV_SOUND_ADD(AY8910, techno_ay8910Int)
  MDRV_SOUND_ADD(SP0250, techno_sp0250_interface)

  MDRV_CPU_ADD(M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(m6502_b_readmem, m6502_b_writemem)
  MDRV_SOUND_ADD(DAC, techno_6502dacInt)

  //MDRV_CPU_ADD(TMS7000, 4000000)
  MDRV_CPU_ADD(TMS7000, 1500000)		//Sounds much better at 1.5Mhz than 4Mhz
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(tms_readmem, tms_writemem)
  MDRV_CPU_PORTS(tms_readport, tms_writeport)

MACHINE_DRIVER_END
