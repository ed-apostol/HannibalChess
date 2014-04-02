/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#define DRAW_ADJUST 0

#define MAX_DRAW 100
#define DRAWN 100
#define DRAWN1 (90 - (90 * DRAW_ADJUST) / 100) 
#define DRAWN2 (75 - (75 * DRAW_ADJUST) / 100)
#define DRAWN3 (60 - (60 * DRAW_ADJUST) / 100)
#define DRAWN4 (50 - (50 * DRAW_ADJUST) / 100)
#define DRAWN5 (40 - (40 * DRAW_ADJUST) / 100)
#define DRAWN6 (32 - (32 * DRAW_ADJUST) / 100)
#define DRAWN7 (24 - (24 * DRAW_ADJUST) / 100)
#define DRAWN8 (17 - (17 * DRAW_ADJUST) / 100)
#define DRAWN9 (10 - (10 * DRAW_ADJUST) / 100)
#define DRAWN10 (5 - (5 * DRAW_ADJUST) / 100)
#define DRAWN11 (3 - (3 * DRAW_ADJUST) / 100)

#define NP_M1 1
#define NP_M2 0
#define NP_M3 0
#define NP_M4 1
#define NP_M5 0
#define NP_M6 0
#define NP_R1 2
#define NP_R2 0
#define NP_Q 2
#define NP_QM1 0
#define NP_QM2 0
#define NP_QM3 0
#define NP_QM4 0
#define NP_QM5 0
#define NP_QR1 0
#define NP_QR2 0

#define DRAWISH (DRAWN2)
#define PRETTY_DRAWISH ((DRAWN2+DRAWN1)/2 )
#define VERY_DRAWISH (DRAWN1 )
#define SUPER_DRAWISH (((DRAWN1+DRAWN)/2))

#define DEBUG_DRAW false

extern  void initMaterial(void);

