/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

#define FUTILITY_MOVE 64
#define REDUCE_MIN 1.5
#define REDUCE_SCALE 2.5
#define PV_REDUCE_MIN 0.5
#define PV_REDUCE_SCALE 7.5

#define LATE_PRUNE_MIN 3
#define FUTILITY_SCALE 18

extern void initPST();
extern void initArr(void);
extern void InitTrapped();
extern void InitMateBoost();