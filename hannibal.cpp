/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: shamilton@distributedinfinity.com    */
/*  Contact: ed_apostol@yahoo.hom                 */
/*  Description: A chess playing program.         */
/**************************************************/

// error check shows could use minor debugging work

// rbp v rb opp bishop drawish
/*
TODO ED:
-implement Search/Thread/Protocol/BitBoard/ classes
-fix all Level 4 warnings in MSVC
-fix all Code Analysis warnings in MSVC
-implement one score for midgame/endgame in eval, more efficient
-add Chess 960 support
*/
/* TODO EVAL
rook vs pawn cut off on 4th rank
try transstore on ALL pruning
write situation specific transtores
can knight catch pawns code from LL
//68 at 3:29
*/
#define TWEAK_030813 true
#define OPT_EASY true
#define VERSION            "20130827_nr"
#define NUM_THREADS			    1
#define MIN_SPLIT_DEPTH			6
#define TCEC true

//#define SPEED_TEST
//#define NEW_EASY true
//#define DEBUG_EASY true
//#define OPTIMIZE true
//#define DEBUG
//#define EVAL_DEBUG true
//#define DEBUG_ML true
//#define DEBUG_SEE true
//#define DEBUG_EVAL_TABLE true
//#define DEBUG_RAZOR true
//#define DEBUG_INDEPTH true

//#define SELF_TUNE2 1 //number of simultaneous training games
//#define TUNE_MAT TRUE

#define NP2 TRUE
#define USE_PHASH TRUE
#define MIN_TRANS_SIZE 16

#ifdef TCEC
#define INIT_EVAL 64
#define INIT_PAWN 32
#define INIT_HASH 64
#define INIT_PVHASH (1 << 16)

#else
#define INIT_EVAL 64
#define INIT_PAWN 32
#define INIT_HASH 128

#define DEFAULT_LEARN_THREADS 0
#define DEFAULT_LEARN_TIME 3
#define DEFAULT_BOOK_EXPLORE 2
#define MAXLEARN_OUT_OF_BOOK 2
#define LEARN_PAWN_HASH_SIZE 32
#define LEARN_EVAL_HASH_SIZE 32
#define DEFAULT_HANNIBAL_BOOK "HannibalBook.han"
#define DEFAULT_HANNIBAL_LEARN "HannibalLearn.lrn"
#define DEFAULT_POLYGLOT_BOOK "HannibalPoly.bin"
#define MAX_BOOK 60 //could be MAXPLY
#define MAX_CONVERT 20
#define HANNIBAL_BOOK_RANDOM (Guci_options->bookExplore*10)
#define MIN_RANDOM -20
#define DEFAULT_BOOK_SCORE INF
#define LEARN_NODES 10000000
#define SHOW_LEARNING false
#define LOG_LEARNING true
#endif

#define DEBUG_BOOK false
#define DEBUG_LEARN false

#define ERROR_FILE "errfile.txt"

#ifdef SPEED_TEST
#define SHOW_SEARCH FALSE
#define RETURN_MOVE FALSE
#else
#ifdef SELF_TUNE2
#define SHOW_SEARCH FALSE
#define RETURN_MOVE FALSE
#else
#define SHOW_SEARCH TRUE
#define RETURN_MOVE TRUE
#endif
#endif



#if defined(__x86_64) || defined(_WIN64)
#define VERSION64BIT
#endif

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <sys/timeb.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type

#include "macros.h"
#include "typedefs.h"
#include "protos.h"
#include "constants.h"
#include "data.h"
#include "init.h"
#include "material.h"
#include "utils.h"
#include "bitutils.h"
#include "attacks.h"
#include "movegen.h"
#include "position.h"
#include "endgame.h"
#include "eval.h"
#include "trans.h"
#include "smp.h"
#include "movepicker.h"
#include "search.h"
#if defined SELF_TUNE2 || defined OPTIMIZE
#include "tune.h"
#endif
#ifdef DEBUG
#include "debug.h"
#include "tests.h"
#endif
#include "uci.h"
#include "book.h"
#include "main.h"
