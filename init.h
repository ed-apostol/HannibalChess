/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

#pragma once

/* init.c */
extern void initPST(uci_option_t *opt);
extern void initArr(void);
extern void InitTrapped();
extern void InitMateBoost();