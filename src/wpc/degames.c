#include "driver.h"
#include "sim.h"
#include "de.h"
#include "de2.h"
#include "de1sound.h"

#define TESTBSMT 0

#define INITGAME(name, gen, disptype, flippers, balls) \
	static core_tGameData name##GameData = { gen, disptype, {flippers}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
DE_INPUT_PORTS_START(name, balls) DE_INPUT_PORTS_END

//Used for games after Frankenstein
#define INITGAME2(name, gen, disptype, flippers, balls) \
	static core_tGameData name##GameData = { gen, disptype, {flippers}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
DE2_INPUT_PORTS_START(name, balls) DE_INPUT_PORTS_END


/*Common Flipper Switch Settings*/
#define FLIP4746    FLIP_SWNO(DE_SWNO(47),DE_SWNO(46))
#define FLIP3031	FLIP_SWNO(DE_SWNO(30),DE_SWNO(31))
#define FLIP1516	FLIP_SWNO(DE_SWNO(15),DE_SWNO(16))
#define FLIP6364	FLIP_SWNO(DE_SWNO(63),DE_SWNO(64))

/* NO OUTPUT */
core_tLCDLayout de_NoOutput[] = {{0}};

/* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows, 1 X 4 Numeric*/
core_tLCDLayout de_dispAlpha1[] = {
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG7), DISP_SEG_7(1,1, CORE_SEG7),
  DISP_SEG_CREDIT(16,24,CORE_SEG7),DISP_SEG_BALLS(0,8,CORE_SEG7H),
  {0}
};

/* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows */
core_tLCDLayout de_dispAlpha2[] = {
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG7), DISP_SEG_7(1,1, CORE_SEG7),
  {0}
};

/* 2 X 16 AlphaNumeric Rows */
core_tLCDLayout de_dispAlpha3[] = {
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};

/* 128x16 DMD OUTPUT */
core_tLCDLayout de_128x16DMD[] = {
	{0,0,16,128,CORE_DMD}, {0}
};

/* 192x64 DMD OUTPUT */
core_tLCDLayout de_192x64DMD[] = {
	{0,0,64,192,CORE_DMD}, {0}
};


/***************************************************/
/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */
/***************************************************/

/*-------------------------------------------------------------------
/ Laser War - CPU Rev 1 /Alpha Type 1 - 32K ROM - 32/64K Sound Roms
/-------------------------------------------------------------------*/
INITGAME(lwar,DE_CPUREV1 | DE_ALPHA1, de_dispAlpha1, FLIP4746, 3/*?*/)
DE32_ROMSTART(lwar,		"lwar.c5",0xeee158ee) 
DESOUND3264_ROMSTART(	"lwar_e9.snd",0x9a6c834d,	//F7 on schem (sound)
						"lwar_e6.snd",0x7307d795,	//F6 on schem (voice1)
						"lwar_e7.snd",0x0285cff9)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(lwar,"Laser War",1987,"Data East",de_mDE_AS,0)


/*-------------------------------------------------------------------------
/ Secret Service - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32K/64K Sound Roms
/-------------------------------------------------------------------------*/
INITGAME(ssvc,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP3031, 3/*?*/)
DE3232_ROMSTART(ssvc,	"ssvc4-6.b5",0xe5eab8cd,
						"ssvc4-6.c5",0x171b97ae)
DESOUND3264_ROMSTART(	"sssndf7.rom",0x980778d0,	//F7 on schem (sound)
						"ssv1f6.rom",0xccbc72f8,	//F6 on schem (voice1)
						"ssv2f4.rom",0x53832d16)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(ssvc,"Secret Service",1988,"Data East",de_mDE_AS,0)

/*-----------------------------------------------------------------------
/ Torpedo Alley - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/------------------------------------------------------------------------*/
INITGAME(torpe,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP1516, 3/*?*/)
DE3232_ROMSTART(torpe,	"torpe2-1.b5",0xac0b03e3,
						"torpe2-1.c5",0x9ad33882)
DESOUND3264_ROMSTART(	"torpef7.rom",0x26f4c33e,	//F7 on schem (sound)
						"torpef6.rom",0xb214a7ea,	//F6 on schem (voice1)
						"torpef4.rom",0x83a4e7f3)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(torpe,"Torpedo Alley",1988,"Data East",de_mDE_AS,0)

/*--------------------------------------------------------------------------
/ Time Machine - CPU Rev 2 /Alpha Type 2 16/32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------*/
INITGAME(tmach,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP1516, 3/*?*/)
DE1632_ROMSTART(tmach,	"tmach2-4.b5",0x6ef3cf07,
                        "tmach2-4.c5",0xb61035f5)
DESOUND3264_ROMSTART(	"tmachf7.rom",0x5d4994bb,	//F7 on schem (sound)
						"tmachf6.rom",0xc04b07ad,	//F6 on schem (voice1)
						"tmachf4.rom",0x70f70888)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(tmach,"Time Machine",1988,"Data East",de_mDE_AS,0)


/*-----------------------------------------------------------------------------------
/ Playboy 35th Anniversary - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------------------------*/
INITGAME(play,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP1516, 3/*?*/)
DE3232_ROMSTART(play,	"play2-4.b5",0xbc8d7b32,
						"play2-4.c5",0x47c30bc2)
DESOUND3264_ROMSTART(	"pbsnd7.dat",0xc2cf2cc5,	//F7 on schem (sound)
						"pbsnd6.dat",0xc2570631,	//F6 on schem (voice1)
						"pbsnd5.dat",0x0fd30569)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(play,"Playboy 35th Anniversary",1989,"Data East",de_mDE_AS,0)

/*-----------------------------------------------------------------------------------
/ Monday Night Football - CPU Rev 2 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/----------------------------------------------------------------------------------*/
INITGAME(mnfb,DE_CPUREV2 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE1632_ROMSTART(mnfb,	"mnfb2-7.b5",0x995eb9b8,
						"mnfb2-7.c5",0x579d81df)
DESOUND3264_ROMSTART(	"mnf-f7.256",0xfbc2d6f6,	//F7 on schem (sound)
						"mnf-f5-6.512",0x0c6ea963,	//F6 on schem (voice1)
						"mnf-f4-5.512",0xefca5d80)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(mnfb,"Monday Night Football",1989,"Data East",de_mDE_AS,0)
/*------------------------------------------------------------------
/ Robocop - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------*/
INITGAME(robo,DE_CPUREV3 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE3232_ROMSTART(robo,	"robob5.a34",0x5a611004,
						"roboc5.a34",0xc8705f47)
DESOUND3264_ROMSTART(	"robof7.rom",0xfa0891bd,	//F7 on schem (sound)
						"robof6.rom",0x9246e107,	//F6 on schem (voice1)
						"robof4.rom",0x27d31df3)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(robo,"Robocop",1989,"Data East",de_mDE_AS,0)

/*-------------------------------------------------------------------------------
/ Phantom of the Opera - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/-------------------------------------------------------------------------------*/
INITGAME(poto,DE_CPUREV2 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE1632_ROMSTART(poto,	"potob5.3-2",0xbdc39205,
						"potoc5.3-2",0xe6026455)
DESOUND3264_ROMSTART(	"potof7.rom",0x2e60b2e3,	//7f
						"potof6.rom",0x62b8f74b,	//6f
						"potof5.rom",0x5a0537a8)	//4f
DE_ROMEND
CORE_GAMEDEFNV(poto,"The Phantom of the Opera",1990,"Data East",de_mDE_AS,0)

/*--------------------------------------------------------------------------------
/ Back To the Future - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------------*/
INITGAME(bttf,DE_CPUREV3 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE3232_ROMSTART(bttf,	"bttfb5.2-0",0xc0d4df6b,
						"bttfc5.2-0",0xa189a189)
DESOUND3264_ROMSTART(	"bttfsf7.rom",0x7673146e,	//7f
						"bttfsf6.rom",0x468a8d9c,	//6f
						"bttfsf5.rom",0x37a6f6b8)	//4f
DE_ROMEND
CORE_GAMEDEFNV(bttf,"Back To the Future",1990,"Data East",de_mDE_AS,0)

/*------------------------------------------------------------------------
/ The Simpsons - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/128K Sound Roms
/------------------------------------------------------------------------*/
INITGAME(simp,DE_CPUREV3 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE1632_ROMSTART(simp,	"simpb5.2-7",0x701c4a4b,
						"simpc5.2-7",0x400a98b2)
DESOUND32128_ROMSTART(	"simpf7.rom",0xa36febbc,	//7f
						"simpf6.rom",0x2eb32ed0,	//6f
						"simpf5.rom",0xbd0671ae)	//4f
DE_ROMEND
CORE_GAMEDEFNV(simp,"The Simpsons",1990,"Data East",de_mDE_AS,0)


/***********************************************************************/
/*************** GAMES USING 128X16 DMD DISPLAY ************************/
/***********************************************************************/

/*------------------------------------------------------------
/ Checkpoint - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAME(chkpnt,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE1632_ROMSTART(chkpnt,	"chkpntb5.107",0x9fbae8e3,
						"chkpntc5.107",0x082dc283)
DE_DMD64_ROMSTART(		"chkpntds.512",0x14d9c6d6)
//DE_DMD64_ROMSTART(		"chkpntds.80",0x14d9c6d6)	&& Doesn't work in mame for some reason. Rename to .512!
DESOUND32128D_ROMSTART(	"chkpntf7.rom",0xe6f6d716,	//7f
						"chkpntf6.rom",0x2d08043e,	//6f
						"chkpntf5.rom",0x167daa2c)	//4f

DE_ROMEND
CORE_GAMEDEFNV(chkpnt,"Checkpoint",1991,"Data East",de_mDE_DMD1S,0)


/*-----------------------------------------------------------------------------
/ Teenage Mutant Ninja Turtles - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/-----------------------------------------------------------------------------*/
INITGAME(tmnt,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE1632_ROMSTART(tmnt,	"tmntb5a.104",0xf508eeee,
						"tmntc5a.104",0xa33d18d4)
DE_DMD64_ROMSTART(		"tmntdsp.104",0x545686b7)
DESOUND32128D_ROMSTART(	"tmntf7.rom",0x59ba0153,	//7f
						"tmntf6.rom",0x5668d45a,	//6f
						"tmntf4.rom",0xf96b0539)	//4f

DE_ROMEND
CORE_GAMEDEFNV(tmnt,"Teenage Mutant Ninja Turtles",1991,"Data East",de_mDE_DMD1S,0)


/***************************************************************************/
/** ALL FOLLOWING GAMES BELOW STARTED USING NEW SOUND BOARD WITH BSMT2000 **/
/***************************************************************************/

/*-------------------------------------------------------------
/ Batman - CPU Rev 3 /DMD Type 1 128K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAME(batmn,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE3232_ROMSTART(batmn,	"batcpub5.101",0xa7f5754e,
						"batcpuc5.101",0x1fcb85ca)
DE_DMD128_ROMSTART(		"batdsp.106",0x4c4120e7)
#if TESTBSMT
DE_BSMT2K_32_ROMSTART(	"batman.u7" ,0xb2e88bf5)
#endif
DE_ROMEND
CORE_GAMEDEFNV(batmn,"Batman",1992,"Data East",de_mDE_DMD1,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Star Trek - CPU Rev 3 /DMD Type 1 128K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(trek,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE64_ROMSTART(trek,		"trekcpuu.201",0xea0681fe)
DE_DMD128_ROMSTART(		"trekdspa.109",0xa7e7d44d)
#if TESTBSMT
DE_BSMT2K_32_ROMSTART(  "treksnd.u7" ,0x642f45b3)
#endif
DE_ROMEND
CORE_GAMEDEFNV(trek,"Star Trek 25th Anniversary",1992,"Data East",de_mDE_DMD1,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Hook - CPU Rev 3 /DMD  Type 1 128K Rom - CPU Rom
/------------------------------------------------------------*/
INITGAME(hook,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE64_ROMSTART(hook,		"hokcpua.408",0x46477fc7)
DE_DMD128_ROMSTART(		"hokdspa.401",0x59a07eb5)
#if TESTBSMT
DE_BSMT2K_32_ROMSTART(  "hooksnd.u7" ,0x642f45b3)
#endif
DE_ROMEND
CORE_GAMEDEFNV(hook,"Hook",1992,"Data East",de_mDE_DMD1,GAME_NO_SOUND)


/***********************************************************************/
/*************** GAMES USING 128X32 DMD DISPLAY ************************/
/***********************************************************************/

/*----------------------------------------------------------------
/ Lethal Weapon 3 - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/---------------------------------------------------------------*/
INITGAME(lw3,DE_CPUREV3, 0, FLIP1516, 3)
DE64_ROMSTART(lw3,		"lw3cpuu.208",0xa3041f8a)
DE_DMD256_ROMSTART(		"lw3drom1.a26",0x44a4cf81,
						"lw3drom0.a26",0x22932ed5)
#if TESTBSMT
DE_BSMT2K_32_ROMSTART(  "lw3snd.u7" ,0x642f45b3)
#endif
DE_ROMEND
CORE_GAMEDEFNV(lw3,"Lethal Weapon 3",1992,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Star Wars - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(stwarde,DE_CPUREV3, 0, FLIP1516, 3)
DE64_ROMSTART(stwarde,	"starcpua.103",0x318085ca)
DE_DMD512_ROMSTART(		"sw4mrom.a15",0x00c87952)
#if TESTBSMT
DE_BSMT2K_32_ROMSTART(  "stwarsnd.u7" ,0x642f45b3)
#endif
DE_ROMEND
CORE_GAMEDEFNV(stwarde,"Star Wars",1992,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Rocky & Bullwinkle - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(rab,DE_CPUREV3b, 0, FLIP1516, 3)
DE64_ROMSTART(rab,	"rabcpua.130",0xf59b1a53)
DE_DMD512_ROMSTART(	"rbdspa.130",0xb6e2176e)
DE_ROMEND
CORE_GAMEDEFNV(rab,"Rocky & Bullwinkle",1993,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Jurassic Park - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(jurpark,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(jurpark,"jpcpua.513",0x9f70a937)
DE_DMD512_ROMSTART(	  "jpdspa.510",0x9ca61e3c)
#if TESTBSMT
DE_BSMT2K_64_ROMSTART(  "jpcpua.513" ,0x642f45b3)
#endif
DE_ROMEND
CORE_GAMEDEFNV(jurpark,"Jurassic Park",1993,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Last Action Hero - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(lah,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(lah,	"lahcpua.112",0xe7422236)
DE_DMD512_ROMSTART(	"lahdispa.106",0xca6cfec5)
DE_ROMEND
CORE_GAMEDEFNV(lah,"Last Action Hero",1993,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Tales From the Crypt - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(tftc,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(tftc,	"tftccpua.303",0xe9bec98e)
DE_DMD512_ROMSTART(	"tftcdspa.301",0x3888d06f)
DE_ROMEND
CORE_GAMEDEFNV(tftc,"Tales From the Crypt",1993,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Tommy - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(tommy,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(tommy,"tomcpua.400",0xd0310a1a)
DE_DMD512_ROMSTART(	"tommydva.400",0x9e640d09)
DE_ROMEND
CORE_GAMEDEFNV(tommy,"Tommy",1994,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ WWF Royal Rumble - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(wwfrumb,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(wwfrumb,	"wwfcpua.106",0x5f1c7da2)
DE_DMD512_ROMSTART(		"wwfdspa.102",0x4b629a4f)
DE_ROMEND
CORE_GAMEDEFNV(wwfrumb,"WWF Royal Rumble",1994,"Data East",de_mDE_DMD2,GAME_NO_SOUND)

/*-------------------------------------------------------------
/ Guns N Roses - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
//INITGAME(gnr,DE_CPUREV3b, 0, FLIP6364, 3)

extern int de_data;
#define sLMagnet   CORE_CUSTSOLNO(1) /* 33 */
#define sTMagnet   CORE_CUSTSOLNO(2) /* 34 */
#define sRMagnet   CORE_CUSTSOLNO(3) /* 35 */

static int  gnr_getSol(int solNo);
/*-- return status of custom solenoids --*/
static int gnr_getSol(int solNo) {
  if (solNo == sLMagnet)    return (de_data & 0x01) > 0;
  if (solNo == sTMagnet)    return (de_data & 0x02) > 0;
  if (solNo == sRMagnet)	return (de_data & 0x04) > 0;
  return 0;
}

static core_tGameData gnrGameData = {
	DE_CPUREV3b, 0, {
    FLIP6364,
    0,0,3,				//We need 3 custom solenoids!
    gnr_getSol,NULL, NULL, NULL,
    NULL,NULL
  },
  NULL,
  {{0}},
  {0}
};
static void init_gnr(void) {
  core_gameData = &gnrGameData;
}
DE_INPUT_PORTS_START(gnr, 6) DE_INPUT_PORTS_END
DE64_ROMSTART(gnr,		"gnrcpua.300",0xfaf0cc8c)
DE_DMD512_ROMSTART(		"gnrdispa.300",0x4abf29e3)
#if TESTBSMT
DE_BSMT2K_64_ROMSTART(  "gnrsnd.u7" ,0x642f45b3)
#endif
DE_ROMEND
CORE_GAMEDEFNV(gnr,"Guns N Roses",1994,"Data East",de_mDE_DMD2,GAME_NO_SOUND)



/***********************************************************************/
/*************** GAMES USING 192X64 DMD DISPLAY ************************/
/***********************************************************************/

/*-------------------------------------------------------------
/ Maverick - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(maverick,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(maverick,	"mavcpua.404",0x9f06bd8d)
DE_DMD1024_ROMSTART(	"mavdsar0.401",0x35b811af,
						"mavdsar3.401",0xc4c126ae)
DE_ROMEND
CORE_GAMEDEFNV(maverick,"Maverick",1994,"Data East",de_mDE_DMD3,GAME_NO_SOUND)

/*****************************************************************************************************************************/
/* NOTE: SEGA Began Distribution of the following games, although they run on Data East Hardware, so they stay in this file! */
/*****************************************************************************************************************************/

/*-------------------------------------------------------------
/ Frankenstein - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(frankst,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(frankst,	"franka.103",0xa9aba9be)
DE_DMD1024_ROMSTART(	"frdspr0a.103",0x9dd09c7d,
						"frdspr3a.103",0x73b538bb)
DE_ROMEND
CORE_GAMEDEFNV(frankst,"Mary Shelley's Frankenstein",1994,"Sega",de_mDE_DMD3,GAME_NO_SOUND)


/*-------------------------------------------------------------
/ Baywatch - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(baywatch,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(baywatch,	"baycpua.400",0x89facfda)
DE_DMD1024_ROMSTART(	"bayrom0a.400",0x43d615c6,
						"bayrom3a.400",0x41bcb66b)
#if TESTBSMT
DE_BSMT2K_64_ROMSTART(  "baywsnd.u7" ,0x642f45b3)
#endif
DE_ROMEND
CORE_GAMEDEFNV(baywatch,"Baywatch",1995,"Sega",de_mDE_DMD3,GAME_NO_SOUND)


/*-------------------------------------------------------------
/ Batman Forever - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(batmanf,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(batmanf,	"batcpua.302",0x5ae7ce69)
DE_DMD1024_ROMSTART(	"bmfrom0a.300",0x764bb217,
						"bmfrom3a.300",0xb4e3b515)
DE_ROMEND
CORE_GAMEDEFNV(batmanf,"Batman Forever",1995,"Sega",de_mDE_DMD3,GAME_NO_SOUND)



/***********************************************************************/
/*************** SPECIAL TEST CHIP - NO DISPLAY ************************/
/***********************************************************************/
/*-------------------------------------------------------------
/ Data East Test Chip 64K ROM
/------------------------------------------------------------*/
INITGAME(detest,DE_CPUREV3, de_NoOutput, FLIP1516, 3/*?*/)
DE64_ROMSTART(detest,"de_test.512",0xbade8ca8)
DE_ROMEND
CORE_GAMEDEFNV(detest,"Data East Test Chip",1998,"Data East",de_mDE_NO,GAME_NO_SOUND)

