/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2013                               */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com    */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/
#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"

#if defined OPTIMIZE || SELF_TUNE2

#define NUM_START_POS 543 
const char *FenStartString[NUM_START_POS] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r1bq1rk1/pp2ppbp/2n2np1/2pp4/5P2/1P2PN2/PBPPB1PP/RN1Q1RK1 w - - 2 8",
    "rnbq1rk1/ppp1ppbp/3p1np1/8/4P3/3P1NP1/PPP2PBP/RNBQ1RK1 b - e3 0 6 ",
    "r1bqkb1r/pppp1ppp/2n2n2/4p3/2P5/2N2N2/PP1PPPPP/R1BQKB1R w KQkq - 4 4 ",
    "rn1qk2r/1b2bppp/pp1ppn2/8/2PQ4/2N2NP1/PP2PPBP/R1BR2K1 w kq - 0 10 ",
    "rn1qkb1r/3ppp1p/b4np1/2pP4/8/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 7 ",
    "rnbqkb1r/pp3p1p/3p1np1/2pP4/4P3/2N5/PP3PPP/R1BQKBNR w KQkq - 0 7 ",
    "rnb1qrk1/ppp1p1bp/3p1np1/3P1p2/2P5/2N2NP1/PP2PPBP/R1BQ1RK1 b - - 0 8 ",
    "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 1 ",
    "rnb1kb1r/pp2pppp/2p2n2/q7/2BP4/2N2N2/PPP2PPP/R1BQK2R b KQkq - 1 6 ",
    "rnbqkb1r/ppp1pppp/3p4/3nP3/3P4/5N2/PPP2PPP/RNBQKB1R b KQkq - 1 4 ",
    "rnbq1rk1/pp2ppbp/2pp1np1/8/3PP3/2N2N2/PPP1BPPP/R1BQ1RK1 w - - 0 7 ",
    "rnbq1rk1/ppp1ppbp/3p1np1/8/3PPP2/2N2N2/PPP3PP/R1BQKB1R w KQ - 3 6 ",
    "rn1qkbnr/pp3ppp/4p3/2ppPb2/3P4/5N2/PPP1BPPP/RNBQK2R w KQkq - 0 6 ",
    "rnbqkb1r/pp3ppp/4pn2/3p4/2PP4/2N2N2/PP3PPP/R1BQKB1R b KQkq - 1 6 ",
    "r2qkbnr/pp1nppp1/2p4p/7P/3P4/3Q1NN1/PPP2PP1/R1B1K2R b KQkq - 0 10 ",
    "rnb1kb1r/pp3ppp/4pn2/2pq4/3P4/2P2N2/PP3PPP/RNBQKB1R w KQkq - 0 6 ",
    "rnbq1rk1/ppp1ppbp/3p1np1/8/4P3/3P1NP1/PPP2PBP/RNBQ1RK1 b - e3 0 6 ",
    "r1bqkb1r/pppp1ppp/2n2n2/4p3/2P5/2N2N2/PP1PPPPP/R1BQKB1R w KQkq - 4 4 ",
    "rn1qk2r/1b2bppp/pp1ppn2/8/2PQ4/2N2NP1/PP2PPBP/R1BR2K1 w kq - 0 10 ",
    "rn1qkb1r/3ppp1p/b4np1/2pP4/8/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 7 ",
    "rnbqkb1r/pp3p1p/3p1np1/2pP4/4P3/2N5/PP3PPP/R1BQKBNR w KQkq - 0 7 ",
    "rnb1qrk1/ppp1p1bp/3p1np1/3P1p2/2P5/2N2NP1/PP2PPBP/R1BQ1RK1 b - - 0 8 ",
    "rnb1k2r/pp2q1pp/2pbpn2/3p1p2/2PP4/1P3NP1/P3PPBP/RNBQ1RK1 w kq - 1 8 ",
    "rnb1kb1r/pp2pppp/2p2n2/q7/2BP4/2N2N2/PPP2PPP/R1BQK2R b KQkq - 1 6 ",
    "rnbqkb1r/ppp1pppp/3p4/3nP3/3P4/5N2/PPP2PPP/RNBQKB1R b KQkq - 1 4 ",
    "rnbq1rk1/pp2ppbp/2pp1np1/8/3PP3/2N2N2/PPP1BPPP/R1BQ1RK1 w - - 0 7 ",
    "rnbq1rk1/ppp1ppbp/3p1np1/8/3PPP2/2N2N2/PPP3PP/R1BQKB1R w KQ - 3 6 ",
    "rn1qkbnr/pp3ppp/4p3/2ppPb2/3P4/5N2/PPP1BPPP/RNBQK2R w KQkq - 0 6 ",
    "rnbqkb1r/pp3ppp/4pn2/3p4/2PP4/2N2N2/PP3PPP/R1BQKB1R b KQkq - 1 6 ",
    "r2qkbnr/pp1nppp1/2p4p/7P/3P4/3Q1NN1/PPP2PP1/R1B1K2R b KQkq - 0 10 ",
    "rnb1kb1r/pp3ppp/4pn2/2pq4/3P4/2P2N2/PP3PPP/RNBQKB1R w KQkq - 0 6 ",
    "r1bq1rk1/pp2npbp/2npp1p1/2p5/4PP2/2NP1NP1/PPP3BP/R1BQ1RK1 w - - 4 9 ",
    "r1bqkb1r/5p1p/p1np4/1p1Npp2/4P3/N7/PPP2PPP/R2QKB1R w KQkq - 0 11 ",
    "r1bqkb1r/pp2pp1p/2np1np1/8/2PNP3/2N5/PP2BPPP/R1BQK2R b KQkq - 1 7 ",
    "r1bqkbnr/1p1p1ppp/p1n1p3/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6 ",
    "r1bqkb1r/1p3pp1/p1nppn1p/6B1/3NP3/2N5/PPPQ1PPP/2KR1B1R w kq - 0 9 ",
    "2rq1rk1/pp1bppbp/3p1np1/4n3/3NP3/1BN1BP2/PPPQ2PP/2KR3R w - - 9 12 ",
    "rnbq1rk1/1p2bppp/p2ppn2/8/3NPP2/2N5/PPP1B1PP/R1BQ1RK1 w - - 1 9 ",
    "rnbqkb1r/1p2pppp/p2p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6 ",
    "r1b1kbnr/pp3ppp/1qn1p3/3pP3/2pP4/P1P2N2/1P3PPP/RNBQKB1R w KQkq - 0 7 ",
    "r1bqkb1r/pp1n1ppp/2n1p3/2ppP3/3P4/2PB4/PP1N1PPP/R1BQK1NR w KQkq - 2 7 ",
    "rnbqk2r/pp2nppp/4p3/2ppP3/3P4/P1P5/2P2PPP/R1BQKBNR w KQkq - 1 7 ",
    "rnbqk2r/ppp1bppp/3p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 3 6 ",
    "rnbqkb1r/ppp2ppp/8/3p4/3Pn3/3B1N2/PPP2PPP/RNBQK2R b KQkq - 1 6 ",
    "r1bqkbnr/pppp1ppp/2n5/8/3NP3/8/PPP2PPP/RNBQKB1R b KQkq - 0 4 ",
    "r1bqk2r/1pp2ppp/pbnp1n2/4p3/PPB1P3/2PP1N2/5PPP/RNBQK2R w KQkq - 0 8 ",
    "r1b1kbnr/1pp2ppp/p1p5/8/3NP3/8/PPP2PPP/RNB1K2R b KQkq - 0 7 ",
    "r1bqk2r/2pp1ppp/p1n2n2/1pb1p3/4P3/1B3N2/PPPP1PPP/RNBQ1RK1 w kq - 2 7 ",
    "r2qkb1r/2p2ppp/p1n1b3/1p1pP3/4n3/1B3N2/PPP2PPP/RNBQ1RK1 w kq - 1 9 ",
    "r2qr1k1/1bp1bppp/p1np1n2/1p2p3/3PP3/1BP2N1P/PP1N1PP1/R1BQR1K1 b - - 2 11 ",
    "rnbqkb1r/pp3ppp/4pn2/2pp4/3P4/2PBPN2/PP3PPP/RNBQK2R b KQkq - 1 5 ",
    "rn1qkb1r/pp2pppp/2p2n2/5b2/P1pP4/2N2N2/1P2PPPP/R1BQKB1R w KQkq - 1 6 ",
    "r1bq1rk1/pp2bppp/2n2n2/2pp2B1/3P4/2N2NP1/PP2PPBP/R2Q1RK1 b - - 6 9 ",
    "r1bqkb1r/pp1n1ppp/2p1pn2/3p4/2PP4/2NBPN2/PP3PPP/R1BQK2R b KQkq - 2 6 ",
    "rnbq1rk1/p1p1bpp1/1p2pn1p/3p4/2PP3B/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8 ",
    "rnbq1rk1/ppp1ppbp/6p1/3n4/3P4/5NP1/PP2PPBP/RNBQ1RK1 b - - 1 7 ",
    "rnbqk2r/pp2ppbp/6p1/2p5/2BPP3/2P5/P3NPPP/R1BQK2R b KQkq - 1 8 ",
    "rnbq1rk1/ppp1bppp/4pn2/3p4/2PP4/5NP1/PP2PPBP/RNBQK2R w KQ - 3 6 ",
    "rnb1k2r/ppppqppp/4pn2/8/1bPP4/5NP1/PP1BPP1P/RN1QKB1R b KQkq - 0 5 ",
    "rnbqkb1r/p1pp1ppp/1p2pn2/8/2PP4/5NP1/PP2PP1P/RNBQKB1R b KQkq - 0 4 ",
    "rnbq1rk1/pppp1ppp/4pn2/8/2PP4/P1Q5/1P2PPPP/R1B1KBNR b KQ - 0 6 ",
    "rnbq1rk1/pp3ppp/4pn2/2pp4/1bPP4/2NBPN2/PP3PPP/R1BQ1RK1 b - - 1 7 ",
    "r1bq1rk1/pppn1pbp/3p1np1/4p3/2PPP3/2N2NP1/PP3PBP/R1BQ1RK1 b - e3 0 8 ",
    "rnbq1rk1/pp3pbp/2pp1np1/4p3/2PPP3/2N1BP2/PP2N1PP/R2QKB1R w KQ - 0 8 ",
    "r1bq1rk1/pppnn1bp/3p4/3Pp1p1/2P1Pp2/2N2P2/PP2BBPP/R2QNRK1 w - - 0 13 ",
    "rnbqk2r/ppp2ppp/3bpn2/3p4/3P1B2/2N2P2/PPPQP1PP/R3KBNR b KQkq - 0 5 ",
    "r1bqkbnr/ppp1pppp/2n5/3p4/2PP4/8/PP2PPPP/RNBQKBNR w KQkq d6 0 3 ",
    "r1b1kbnr/pppp1ppp/2n5/8/3NP2q/8/PPP2PPP/RNBQKB1R w KQkq - 1 5 ",
    "r1bqkbnr/pppp1ppp/2n5/8/4P3/2p2N2/PP3PPP/RNBQKB1R w KQkq - 0 5 ",
    "r2qk2r/5pbp/p1np4/1p1Npb2/8/N1P5/PP3PPP/R2QKB1R w KQkq - 0 14 ",
    "r1bqk1nr/pppp1ppp/2n5/4p3/1bB1P3/5N2/P1PP1PPP/RNBQK2R w KQkq - 0 5 ",
    "r1b2rk1/1pq1bppp/p1nppn2/8/3NP3/1BN1B3/PPP1QPPP/2KR3R w - - 6 11 ",
    "r3k2r/p1qbnppp/1pn1p3/2ppP3/P2P4/2PB1N2/2P2PPP/R1BQ1RK1 b kq - 5 11 ",
    "r1bqkb1r/pppp1ppp/2n2n2/4p1N1/2B1P3/8/PPPP1PPP/RNBQK2R b KQkq - 5 4 ",
    "r1b2rk1/2q1bppp/p2p1n2/npp1p3/3PP3/2P2N1P/PPBN1PP1/R1BQR1K1 b - - 2 12 ",
    "r1bqrnk1/pp2bppp/2p2n2/3p2B1/3P4/2NBPN2/PPQ2PPP/R4RK1 w - - 5 11 ",
    "rnbqkb1r/p1pp1ppp/1p2pn2/8/2PP4/P4N2/1P2PPPP/RNBQKB1R b KQkq - 0 4 ",
    "rnbq1rk1/pp2ppbp/6p1/8/3PP3/5N2/P3BPPP/1RBQK2R b K - 0 10 ",
    "2rq1rk1/p2nbppp/bpp1p3/3p4/2PPP3/1PB3P1/P2N1PBP/R2Q1RK1 b - e3 0 13 ",
    "r1bqnrk1/ppp1npbp/3p2p1/3Pp3/2P1P3/2N1B3/PP2BPPP/R2QNRK1 b - - 4 10 ",
    "rnbqkbnr/p1pppppp/1p6/8/3PP3/8/PPP2PPP/RNBQKBNR b KQkq e3 0 2 ",
    "r1bq1rk1/ppp1npbp/2np2p1/4p3/2P5/2NPP1P1/PP2NPBP/R1BQ1RK1 b - - 0 8 ",
    "rnb1kb1r/1p3ppp/p2ppn2/6B1/3NPP2/q1N5/P1PQ2PP/1R2KB1R w Kkq - 2 10 ",
    "rn1qk2r/pp3ppp/2p1pn2/4Nb2/PbpPP3/2N2P2/1P4PP/R1BQKB1R b KQkq e3 0 8 ",
    "r2qkb1r/pp1n1ppp/2p2n2/3pp2b/4P3/3P1NPP/PPPN1PB1/R1BQ1RK1 b kq e3 0 8 ",
    "rnbqkb1r/ppppppp1/5n1p/6B1/3P4/8/PPP1PPPP/RN1QKBNR w KQkq - 0 3 ",
    "r2q1rk1/ppp1bppp/1nn1b3/4p3/1P6/P1NP1NP1/4PPBP/R1BQ1RK1 b - b3 0 10 ",
    "rnbq1rk1/p3ppbp/1p1p1np1/2p5/3P1B2/2P1PN1P/PP2BPP1/RN1QK2R w KQ - 0 8 ",
    "r1b1qrk1/ppp1p1bp/n2p1np1/3P1p2/2P5/2N2NP1/PP2PPBP/1RBQ1RK1 b - - 2 9 ",
    "rnbq1rk1/1p2ppbp/2pp1np1/p7/P2PP3/2N2N1P/1PP1BPP1/R1BQ1RK1 b - - 0 8 ",
    "2kr3r/ppqn1pp1/2pbp2p/7P/3PQ3/5NP1/PPPB1P2/2KR3R w - - 1 16 ",
    "r3kb1r/pp2pppp/1nnqb3/8/3p4/NBP2N2/PP3PPP/R1BQ1RK1 b kq - 3 10 ",
    "r2qk2r/5pbp/p1npb3/3Np2Q/2B1Pp2/N7/PP3PPP/R4RK1 b kq - 0 15 ",
    "r1bqk2r/4bpp1/p2ppn1p/1p6/3BPP2/2N5/PPPQ2PP/2KR1B1R w kq b6 0 12 ",
    "r1b1k2r/1pq3pp/p1nbpn2/3p4/3P4/2NB1N2/PP3PPP/R1BQ1RK1 w kq - 0 13 ",
    "rn1q1rk1/pp3ppp/3b4/3p4/3P2b1/2PB1N2/P4PPP/1RBQ1RK1 b - - 2 12 ",
    "r1b1kb1r/ppp2ppp/2p5/4Pn2/8/2N2N2/PPP2PPP/R1B2RK1 w - - 2 10 ",
    "r2qrbk1/1b1n1p1p/p2p1np1/1p1Pp3/P1p1P3/2P2NNP/1PB2PP1/R1BQR1K1 w - - 0 17 ",
    "r2q1rk1/pp1n1ppp/2p1pnb1/8/PbBPP3/2N2N2/1P2QPPP/R1B2RK1 w - - 1 11 ",
    "r1bq1rk1/1p2bppp/p1n1pn2/8/P1BP4/2N2N2/1P2QPPP/R1BR2K1 b - - 2 11 ",
    "r1b2rk1/pp3ppp/2n1pn2/q1bp4/2P2B2/P1N1PN2/1PQ2PPP/2KR1B1R b - - 2 10 ",
    "rnbq1rk1/2pnppbp/p5p1/1p2P3/3P4/1QN2N2/PP3PPP/R1B1KB1R w KQ - 1 10 ",
    "rnb2rk1/ppp1qppp/3p1n2/3Pp3/2P1P3/5NP1/PP1N1PBP/R2Q1RK1 w - - 1 11 ",
    "r2q1rk1/pbpn1pp1/1p2pn1p/3p4/2PP3B/P1Q1PP2/1P4PP/R3KBNR w KQ - 1 11 ",
    "r3qrk1/1ppb1pbn/n2p2pp/p2Pp3/2P1P2B/P1N5/1P1NBPPP/R2Q1RK1 w - - 1 13 ",
    "r1bqkb1r/pp1n1ppp/2p1pn2/3p4/2PP4/2N1PN2/PPQ2PPP/R1B1KB1R b KQkq - 2 6 ",
    "r1bqk1nr/p2pppbp/2p3p1/2p5/4P3/2P2N2/PP1P1PPP/RNBQ1RK1 b kq - 0 6 ",
    "rnbqkb1r/pp3pp1/2p1pB1p/3p4/2PP4/2N2N2/PP2PPPP/R2QKB1R b KQkq - 0 6 ",
    "r1bqk2r/1pppbppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQR1K1 b kq - 5 6 ",
    "rnbqk1nr/pp3ppp/4p3/2ppP3/3P4/P1P5/2P2PPP/R1BQKBNR b KQkq - 0 6 ",
    "rnbqkb1r/pp3ppp/3p1n2/2pP4/8/2N2N2/PP2PPPP/R1BQKB1R b KQkq - 1 6 ",
    "rn1qk2r/p1pp1ppp/bp2pn2/8/1bPP4/1P3NP1/P2BPP1P/RN1QKB1R b KQkq - 2 6 ",
    "rnbqk2r/ppp2ppp/4pn2/6B1/1bpPP3/2N2N2/PP3PPP/R2QKB1R b KQkq - 2 6 ",
    "rnbqk2r/ppp2ppp/4pn2/6B1/1bpPP3/2N2N2/PP3PPP/R2QKB1R b KQkq - 2 6 ",
    "rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2 ",
    "rnbq1rk1/ppp1bppp/4pn2/3p4/2PP4/5NP1/PP2PPBP/RNBQ1RK1 b - - 2 6 ",
    "rnb1kb1r/1p3ppp/pq1ppn2/6B1/3NPP2/2N5/PPPQ2PP/R3KB1R b KQkq - 2 8 ",
    "rnbqk2r/4bpp1/3ppn1p/pN1P4/1pP4B/8/1P2PPPP/RN1QKB1R w KQkq - 0 11 ",
    "r1bq1rk1/bp1p1ppp/p1n1p1n1/8/4PP2/1N1B4/PPP1Q1PP/RNB2R1K w - - 3 11 ",
    "r1b1k2r/pp1pqppp/2p3n1/1Bn4Q/8/2N5/PPP2PPP/R1B2RK1 w kq - 0 11 ",
    "r1bqk2r/4npbp/1pp3p1/2p1p3/p3P3/P2PBN2/1PPN1PPP/R2QR1K1 w kq - 0 11 ",
    "r1bqr1k1/pp1n1pbp/2pp1np1/8/2PpP3/2N1BNPP/PP3PB1/R2Q1RK1 w - - 0 11 ",
    "r2qk2r/2pb1pb1/p1np1npp/1p2p1B1/3PP3/1BP2N2/PP3PPP/RN1QR1K1 w kq - 0 11 ",
    "r2qk2r/3b1pbp/p1np1np1/1p6/3pP3/4BN1P/PPB2PP1/RN1QR1K1 w kq - 0 13 ",
    "r2q1rk1/1bpn1ppp/pp1b4/3p4/3Pn3/1PNBPN2/PB2QPPP/2R2RK1 w - - 2 13 ",
    "rnbqkbnr/pppppppp/8/8/2P5/8/PP1PPPPP/RNBQKBNR b KQkq c3 0 1 ",
    "r1b2r1k/ppp1b1pB/8/3qn3/8/P3P3/1PQ2PPP/R1B1K1NR w KQ - 0 13 ",
    "rbbq1rk1/ppn3pp/2p5/2Pp1p2/3Pn3/P1NB1N2/1PQ2PPP/R1B2RK1 w - - 3 13 ",
    "r4rk1/pp1n1ppp/1qp1p3/5b2/1b1Pn2N/1QN3P1/PP2PPBP/R1B1R1K1 w - - 12 13 ",
    "r1bqk2r/5ppp/p3pn2/1p1P4/nP2P3/PQ3P2/6PP/R1B1KBNR w KQkq - 0 13 ",
    "r1bq1rk1/5ppp/p1np4/1p1Np1b1/4P3/2P5/PPN2PPP/R2QKB1R w KQ - 3 13 ",
    "3qk2r/1bpn1pp1/1p1ppn1p/r7/p1PP3B/P1PBPN2/2Q2PPP/R3K2R w KQk - 3 13 ",
    "rnbqkbnr/pppp1ppp/8/4p3/4PP2/8/PPPP2PP/RNBQKBNR b KQkq f3 0 2 ",
    "r3kbnr/ppqn1pp1/2p1p2p/7P/3P4/3Q1NN1/PPPB1PP1/R3K2R w KQkq - 4 13 ",
    "rnbqkbnr/pp1ppppp/2p5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2 ",
    "r1b1kbr1/1p3p1p/p1q1pp2/2p5/2NP4/2P5/PP2BPPP/R2QK2R w KQq - 4 13 ",
    "r1bqr1k1/3n1pbp/pp1p1np1/2pP4/P3P3/2NB1N1P/1P3PP1/R1BQR1K1 w - - 0 13 ",
    "r2q1rk1/pb1nbppp/1p2pn2/3pN3/3P1B2/2N3P1/PP2PPBP/2RQ1RK1 w - - 4 13 ",
    "r2qnrk1/1bp2ppp/p2p2n1/1pb1pN2/P3P3/1BPP1Q2/1P3PPP/RNB2RK1 w - - 7 13 ",
    "rnbqkb1r/ppp1pp1p/5np1/3p4/2PP4/2N5/PP2PPPP/R1BQKBNR w KQkq d6 0 4 ",
    "rnbqk2r/ppp1ppbp/3p1np1/8/2PPPP2/2N5/PP4PP/R1BQKBNR b KQkq f3 0 5 ",
    "rnbqkbnr/pppppppp/8/8/5P2/8/PPPPP1PP/RNBQKBNR b KQkq f3 0 1 ",
    "rnbqkbnr/pppppppp/8/8/1P6/8/P1PPPPPP/RNBQKBNR b KQkq b3 0 1 ",
    "rnbqkbnr/pppppppp/8/8/8/1P6/P1PPPPPP/RNBQKBNR b KQkq - 0 1 ",
    "rnbqkb1r/pppp1ppp/4pn2/8/2P1P3/2N5/PP1P1PPP/R1BQKBNR b KQkq e3 0 3 ",
    "rnbqkbnr/pp1ppppp/8/2p5/2P5/8/PP1PPPPP/RNBQKBNR w KQkq c6 0 2 ",
    "rnbqkbnr/ppppp1pp/8/5p2/3P4/8/PPP1PPPP/RNBQKBNR w KQkq f6 0 2 ",
    "r1bqk2r/1p1p2pp/pB1Qpp2/4nn2/2P5/2N5/PP3PPP/R3KB1R w KQkq - 4 13 ",
    "r1bqk2r/ppp1n1bp/3p2p1/3Ppp2/8/3P1NP1/PP2PPBP/R1BQ1RK1 w kq - 1 10 ",
    "r2qk2r/p2bb1pp/2p1np2/2p1p3/Q3N3/1P1P1NP1/P3PP1P/R1B2RK1 b kq - 0 13 ",
    "r1bqk2r/pppp1ppp/8/4P3/1bP1n3/2N5/PP3PPP/R1BQKB1R w KQkq - 0 8 ",
    "r1b2rk1/p2n1pbp/2q2np1/4p3/2B1P3/2N2N2/PP2Q1PP/R1B2R1K b - - 1 14 ",
    "r1bq1rk1/pp1nbppp/2p2n2/3p2B1/3P4/2NBP3/PPQ1NPPP/R3K2R b KQ - 6 9 ",
    "r1bqk1nr/pp1p1pbp/4p1p1/8/2PP4/2p1P1P1/5PBP/R1BQK1NR b KQkq - 0 9 ",
    "r2qkb1r/pb1n1ppp/1pp1pn2/3p4/2PP4/1P1BPN2/PB3PPP/RN1Q1RK1 b kq - 3 8 ",
    "r1bqk2r/p3bppp/2p2n2/np2N3/4P3/2P5/P1Q1BPPP/RNB1K2R w KQkq - 2 12 ",
    "r2q1rk1/1p2bppp/p2pbn2/n3p3/4P3/1BN1BN2/PPP2PPP/R2QR1K1 w - - 10 12 ",
    "rnbq1rk1/ppp1p1bp/3p2p1/5p2/3Pn3/1P3NP1/PBP1PPBP/RN1Q1RK1 w - - 2 8 ",
    "r1b2rk1/ppq2ppp/5n2/n1pp4/3P4/P1PBPN2/1B3PPP/R2Q1RK1 w - - 0 12 ",
    "r2q1rk1/1b1n1ppp/p1pbpn2/1p6/3P4/P1NBPN2/1PQ2PPP/R1B2RK1 w - - 0 12 ",
    "1rb1k2r/2qpbppp/p1n1pn2/8/Np1NPP2/3BB3/PPPQ2PP/2KR3R w k - 2 12 ",
    "r2qk2r/pp1n1ppp/2pbpnb1/3p4/2PP3N/2N1P1P1/PP2BP1P/R1BQK2R w KQkq - 1 9 ",
    "r1bqk2r/pp1n1ppp/2n1p3/2bpP3/5PQ1/P1N5/1PP3PP/R1B1KBNR b KQkq - 1 8 ",
    "r4rk1/p1pqbppp/5n2/3p4/8/1P6/P1PN1PPP/R1BQ1RK1 w - - 1 12 ",
    "r1bq1rk1/5ppp/p1np1b2/1p1Np3/4P3/N1P5/PP3PPP/R2QKB1R w KQ - 1 12 ",
    "r1b2rk1/pppnq1pp/4p3/3pN3/1bPPp3/6P1/PP1BPPBP/R2Q1RK1 w - - 2 11 ",
    "r1bq1rk1/pp2nppp/2nb4/1B1p4/3N4/5N2/PPP2PPP/R1BQ1RK1 w - - 1 10 ",
    "r2q1r1k/1pp1n1pp/p1pb1p2/4p3/3PP1b1/1QP1BN2/PP1N1PPP/R4RK1 w - - 7 11 ",
    "r2q1rk1/1b1nppbp/pp3np1/3p4/3P4/1QNBPN2/PP1B1PPP/R4RK1 w - - 2 12 ",
    "r2qk2r/1bpp1ppp/p1n2n2/1pb1p3/P3P3/1B3N2/1PPP1PPP/RNBQ1RK1 w kq - 1 8 ",
    "r1bq1rk1/bppp1ppp/p7/4P3/B2nn3/3N4/PPP2PPP/RNBQ1RK1 w - - 1 10 ",
    "r1bqkb1r/3n1ppp/p3pn2/1N2P3/3p4/3B1N2/PP3PPP/R1BQK2R b KQkq - 0 11 ",
    "r1bq1rk1/1p1nbppp/2n1p3/p1ppP3/3P1P1P/2P2N2/PP2N1P1/R1BQKB1R w KQ - 1 10 ",
    "r1bqk1nr/1ppp1pbp/2n3p1/p3p3/2P5/2N3P1/PP1PPPBP/1RBQK1NR w Kkq a6 0 6 ",
    "rnb1k2r/1p2b1p1/p2ppn1p/8/3NP2B/q1N5/P1PQ2PP/1R2KB1R w Kkq - 0 13 ",
    "rn2kb1r/pp1qpp1p/3p1np1/8/2PNP3/2N5/PP3PPP/R1BQK2R b KQkq - 0 8 ",
    "r1bq1rk1/1pppbppp/p1n5/4P3/B2Nn3/8/PPP2PPP/RNBQ1RK1 w - - 1 9 ",
    "rnbqk2r/pp3ppp/4pn2/8/2BP4/P1N5/1P3PPP/R1BQK2R b KQkq - 0 9 ",
    "rnbr2k1/ppp1qppp/4pn2/3p4/2PP4/5NP1/PP1QPPBP/RN3RK1 w - - 4 9 ",
    "rn1qkbnr/pb3ppp/1p1pp3/2p5/4P3/5NP1/PPPPQPBP/RNB1K2R w KQkq - 0 6 ",
    "r1bqk2r/3n1ppp/p2b1n2/1pp1p3/3PP3/3B1N2/PP2NPPP/R1BQ1RK1 w kq - 0 12 ",
    "rn1qkbnr/pp3ppp/4p3/2ppPb2/3P4/4BN2/PPP1BPPP/RN1QK2R b KQkq - 1 6 ",
    "rnbqkbnr/1pp2ppp/p3p3/3p4/2PP4/5N2/PP2PPPP/RNBQKB1R w KQkq - 0 4 ",
    "r2qk2r/ppp1bppp/2n2n2/3p1b2/3P1B2/1QN1P3/PP3PPP/R3KBNR w KQkq - 3 8 ",
    "rn1qkb1r/p3pppp/b1p2n2/8/PppPP3/8/1PQ2PPP/RNB1KBNR w KQkq - 4 8 ",
    "rn1qk2r/ppp1ppbp/4bnp1/8/2p5/N4NP1/PP1PPPBP/R1BQ1RK1 w kq - 2 7 ",
    "rnb1kb1r/pp2pp1p/2p2p2/3q4/3P4/2P5/PP3PPP/R1BQKBNR w KQkq - 1 7 ",
    "rnbq1rk1/pp2ppbp/2pp1np1/8/3PP3/2N2N2/PPP1BPPP/R1BQ1RK1 w - - 4 7 ",
    "rnb1kbnr/pp3ppp/3qp3/8/2Bp4/5N2/PPPN1PPP/R1BQK2R w KQkq - 2 7 ",
    "r2qrbk1/1bpn1pp1/p2p1n1p/1p2p1B1/3PP3/2P2N1P/PPB2PP1/R2QRNK1 w - - 0 15 ",
    "r2qkb1r/pp1n1ppp/2p2n2/3pp3/8/3P1NPP/PPP1PPB1/R1BQK2R w KQkq e6 0 8 ",
    "r1b1k2r/ppqpbppp/2n1pn2/8/3NP3/2N1B3/PPP1BPPP/R2QK2R w KQkq - 7 8 ",
    "r3kb1r/p1pp1ppp/b1p5/3nP3/4q3/5N2/PPPQBPPP/R1B1K2R w KQkq - 11 12 ",
    "r1bqkbnr/pp2pp1p/2p3p1/2p5/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 5 ",
    "1rbq1rk1/1pp1ppbp/p1np1np1/8/2PP4/1PN2NP1/P3PPBP/R1BQ1RK1 w - - 1 9 ",
    "rnbqkb1r/ppp2ppp/3p4/8/8/2P2N2/PPP2PPP/R1BQKB1R b KQkq - 0 6 ",
    "r1bq1rk1/1pp2pbp/p1np1np1/4p3/B3P3/2PP1N2/PP1N1PPP/R1BQR1K1 b - - 5 9 ",
    "r3kb1r/pp3ppp/1qn1pn2/3p1b2/3P1B2/1QN1P3/PP3PPP/2R1KBNR w Kkq - 2 9 ",
    "1rbqk2r/2pp1ppp/p1n2n2/1pb1p3/P3P3/1B3N2/1PPP1PPP/RNBQ1RK1 w k - 1 8 ",
    "rnbq1rk1/pp3pp1/2p1pb1p/3p4/2PP4/2N1PN2/PP3PPP/2RQKB1R w K - 0 9 ",
    "r1bq1rk1/1p1n1ppp/p1pbpn2/8/2BP4/2N1PN2/PPQ2PPP/R1B2RK1 w - - 0 10 ",
    "r4rk1/1pqbbppp/p1nppn2/8/P2NPP2/2N1B3/1PP1B1PP/R2Q1R1K w - - 5 12 ",
    "rn1qkbnr/pp3ppp/2p1p3/3p4/2P3b1/5NP1/PP1PPPBP/RNBQK2R w KQkq - 0 5 ",
    "rnbqk2r/ppp1bppp/4pn2/6B1/3PN3/8/PPP2PPP/R2QKBNR w KQkq - 1 6 ",
    "r1bq1rk1/ppp1npbp/3p2p1/3Pp2n/1PP1P3/2N2N2/P3BPPP/R1BQ1RK1 w - - 1 10 ",
    "rnb2rk1/pp2ppbp/6p1/8/3PP3/8/P2B1PPP/2R1KBNR w K - 1 12 ",
    "r1bq1rk1/pppnbppp/3p1n2/4p3/2BPP3/2N2N2/PPP2PPP/R1BQ1RK1 w - - 6 7 ",
    "r1bqk2r/pp3ppp/2n1p3/3n4/1b1P4/2NB4/PP2NPPP/R1BQK2R w KQkq - 0 9 ",
    "rnbqkb1r/ppp1pp1p/6p1/3n4/7P/2N2N2/PP1PPPP1/R1BQKB1R b KQkq h3 0 5 ",
    "rnbqkb1r/ppp1pp1p/5np1/3p2B1/2PP4/2N5/PP2PPPP/R2QKBNR b KQkq - 1 4 ",
    "r1bqkb1r/pp3pp1/4pn1p/8/3p3B/2P2N2/PP3PPP/R2QKB1R w KQkq - 0 10 ",
    "r1bk1b1r/ppp2ppp/2p5/4Pn2/8/5N2/PPP2PPP/RNB2RK1 w - - 0 9 ",
    "r1bqr1k1/pppn1pbp/3p1np1/4p3/2PP4/2N1PNP1/PP3PBP/R1BQ1RK1 w - - 1 9 ",
    "r1b1qrk1/ppp2pbp/n2p1np1/4p3/2PPP3/2N1BN2/PP2BPPP/R2Q1RK1 w - - 4 9 ",
    "r1b1kb1r/1p1n1ppp/pq1p1n2/4pNB1/4PP2/2N5/PPP3PP/R2QKB1R w KQkq - 2 9 ",
    "r2q1rk1/1p1nbppp/p2p1n2/3Pp3/8/1N1QB3/PPP1BPPP/R4RK1 b - - 0 12 ",
    "r1b1k2r/pp1n1ppp/5n2/q1Pp2B1/1b6/2N1PN2/PP3PPP/R2QKB1R w KQkq - 1 9 ",
    "r3kb1r/p1ppqp1p/b1p3p1/3nP3/2P5/8/PP1NQPPP/R1B1KB1R w KQkq - 0 10 ",
    "rnbqkb1r/pp3ppp/2p1pn2/8/P1pP4/2N2N2/1P2PPPP/R1BQKB1R w KQkq - 0 6 ",
    "1rbq1rk1/2p2ppp/1bnp1n2/1N2p3/3PP3/1BP2N2/1P3PPP/R1BQ1RK1 b - - 0 12 ",
    "r2q1rk1/pp1n1ppp/2p1pn2/7b/Pb1PP3/2NB1N2/1P2QPPP/R1B2RK1 w - - 3 12 ",
    "r3k2r/pbppqpbp/1np3p1/4P3/2P2B1P/2N5/PP2QPP1/2KR1B1R b kq - 2 12 ",
    "rnbq1rk1/ppp2ppp/4pn2/8/2QP4/P4N2/1P2PPPP/R1B1KB1R b KQ - 0 8 ",
    "r1bq1rk1/ppp1p1bp/3p1np1/n2P1p2/2P5/2N2NP1/PP2PPBP/R1BQ1RK1 w - - 1 9 ",
    "r3k2r/p1pp1p1p/b1p3p1/3nP1q1/2P5/NP4P1/P3QP1P/R3KB1R b KQkq - 0 12 ",
    "r1bqk2r/pp2ppbp/2p2np1/2p5/4P3/2N2N2/PPPP1PPP/R1BQ1RK1 w kq - 0 7 ",
    "r1b2rk1/ppq2ppp/2n2n2/2p1p3/2BP4/P1P1PN2/1B3PPP/R2Q1RK1 w - - 0 12 ",
    "2kr1b1r/pb1n1p2/1q2pP2/1ppP2B1/2p5/2N3P1/PP3PBP/R2Q1RK1 b - - 4 15 ",
    "rnbqkb1r/ppp1pp1p/3p1np1/8/2PP4/5P2/PP2P1PP/RNBQKBNR w KQkq - 0 4 ",
    "rnbqk2r/ppp1ppbp/6p1/3n4/3P4/6P1/PP2PPBP/RNBQK1NR w KQkq - 0 6 ",
    "rnb1k2r/pp3ppp/4p3/q1Pn4/8/P1P2P2/4P1PP/R1BQKBNR w KQkq - 1 9 ",
    "rnbqk2r/1ppp1ppp/4pn2/p7/1bPP4/5N2/PP1BPPPP/RN1QKB1R w KQkq a6 0 5 ",
    "rnb1k1nr/ppq3pp/4p3/2ppPp2/3P2Q1/P1P5/2P2PPP/R1B1KBNR w KQkq f6 0 8 ",
    "r2qkb1r/3n1pp1/p2pbn2/1p2p2p/4P3/1NN1BP2/PPPQ2PP/2KR1B1R w kq h6 0 11 ",
    "rnb1k2r/pp3ppp/4pn2/2P3B1/1bp1P3/2N2N2/PP3PPP/3RKB1R b Kkq - 0 8 ",
    "rnbqk2r/pp1p1ppp/4p3/8/1bPNn3/2N3P1/PP2PP1P/R1BQKB1R w KQkq - 3 7 ",
    "rnbqk2r/ppp1ppbp/5np1/3p4/2PP1B2/2N5/PP2PPPP/R2QKBNR w KQkq - 2 5 ",
    "rn1qkb1r/p4ppp/1p1ppn2/8/2PN4/2N3P1/PP2PPKP/R1BQ1R2 b kq - 0 9 ",
    "r1bqkb1r/pp1npppp/2p2n2/3p4/2P5/2N1PN2/PP1P1PPP/R1BQKB1R w KQkq - 3 5 ",
    "r1bq1rk1/p4pbp/1p4p1/n1p1p3/3PP3/2PBB3/P2QNPPP/R4RK1 w - e6 0 13 ",
    "rn1qkb1r/pp2pppp/2p2n2/3p4/2PP2b1/1Q2PN2/PP3PPP/RNB1KB1R b KQkq - 2 5 ",
    "r1b1k2r/pp1p1ppp/1qn1p3/8/1bP5/2N3P1/PPNQPPBP/R3K2R b KQkq - 2 10 ",
    "r2qrbk1/1b1n1pp1/p4n1p/1p2p3/P1p1P2B/2P2N1P/1PB2PP1/R2QRNK1 w - - 0 18 ",
    "r2q1rk1/3nbppp/p2pbn2/1p2p3/4P1P1/1NN1BP2/PPPQ3P/2KR1B1R w - b6 0 12 ",
    "rnbq1rk1/p1pp1ppp/4pn2/1p6/2PP4/P1Q5/1P2PPPP/R1B1KBNR w KQ b6 0 7 ",
    "rnbqkb1r/ppp1pp1p/6p1/3n4/8/2N2N2/PP1PPPPP/R1BQKB1R w KQkq - 0 5 ",
    "rn1qk2r/pp2ppbp/6p1/2p5/3PP3/2P1BP2/P4P1P/2RQKB1R b Kkq - 0 10 ",
    "r2q1rk1/pp1n1ppp/2p1pn2/8/PbBP4/2N1PQ1P/1P3PP1/R1B2RK1 w - - 1 12 ",
    "rnbq1rk1/1p2bppp/p2ppn2/8/3NP3/2N3P1/PPP2PBP/R1BQ1RK1 w - - 4 9 ",
    "r3k2r/1p1b2pp/pBn1pp2/2p2n2/2P5/2N5/PP3PPP/2KR1B1R b kq - 3 17 ",
    "rnbqk2r/ppp1ppbp/6p1/8/Q2PP3/2P5/P4PPP/R1B1KBNR b KQkq - 2 7 ",
    "r1b1kb1r/ppqn1ppp/2n1p3/3pP3/1P1P4/P4N2/4NPPP/R1BQKB1R b KQkq - 0 10 ",
    "r1bqk2r/1pp1bppp/p1p2n2/4p3/4P3/5N2/PPPP1PPP/RNBQ1RK1 w kq - 0 7 ",
    "r1bqkb1r/pp1n2pp/2p1p3/3p1p2/2PPn3/1P1BPN2/P4PPP/RNBQ1RK1 w kq f6 0 8 ",
    "rn1q1rk1/1p1nbppp/p2pb3/4p1P1/4P3/1NN1B3/PPP1BP1P/R2QK2R w KQ - 1 11 ",
    "rnbqkb1r/1p2pppp/p1p2n2/3p4/2PP4/2N1P3/PP3PPP/R1BQKBNR w KQkq - 0 5 ",
    "r1b2rk1/5ppp/p1pb4/1p1n4/3P4/1BP3Pq/PP3P1P/RNBQR1K1 w - - 1 15 ",
    "r1bqk2r/ppp2ppp/2n1pn2/3p4/QbPP4/2N2N2/PP2PPPP/R1B1KB1R w KQkq - 4 6 ",
    "r1bq1rk1/ppp1ppbp/1nn3p1/8/3P4/2N2NP1/PP2PPBP/R1BQ1RK1 w - - 4 9 ",
    "rnbqk1nr/2p1ppbp/p2p2p1/1p6/3PPP2/2N2N2/PPP3PP/R1BQKB1R w KQkq b6 0 6 ",
    "rnbqkb1r/1p2pppp/p2p4/8/3NP1n1/2N1B3/PPP2PPP/R2QKB1R w KQkq - 2 7 ",
    "r1bq1rk1/pp2n2p/2pp1npb/3PppN1/1PP1P2P/2N1BP2/P3B1P1/R2QR1K1 b - h3 0 14 ",
    "r1b1kb1r/pp2pppp/1np2n2/3q4/2p5/N4NP1/PPQPPPBP/R1B2RK1 w kq - 6 8 ",
    "r2q1rk1/ppp1bppp/8/3p1b2/1nPPn3/2N2N2/PP2BPPP/R1BQ1RK1 w - - 5 11 ",
    "rnbqkbnr/pp2pppp/3p4/1Bp5/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 1 3 ",
    "r1b1k2r/ppp1ppbp/2n3p1/3q2B1/Q2P4/2P2N2/P3PPPP/R3KB1R w KQkq - 0 9 ",
    "rnbq1rk1/1pp1bppp/p3pn2/8/2pP4/5NP1/PPQ1PPBP/RNB2RK1 w - - 0 8 ",
    "rnbqkbnr/ppp2ppp/8/4p3/2pPP3/8/PP3PPP/RNBQKBNR w KQkq e6 0 4 ",
    "r1bqk2r/pp1n1pp1/4pn1p/2p3B1/3P4/P1Q2N2/1P2PPPP/R3KB1R w KQkq - 0 10 ",
    "r1bk1b1r/ppp2ppp/2p5/4Pn2/8/5N2/PPP2PPP/RNB2RK1 w - - 0 9 ",
    "r1b1k1nr/ppp1bppp/8/3q4/3P4/2P1N3/P4PPP/RNBQK2R b KQkq - 1 10 ",
    "rn1qkb1r/pp2pppp/2p2n2/3p4/2PP2b1/4PN1P/PP3PP1/RNBQKB1R b KQkq - 0 5 ",
    "r3kb1r/p1p1qp1p/b1pp1np1/4P3/2P2P2/1P6/P4QPP/RNB1KB1R w KQkq - 2 12 ",
    "rnbqk2r/ppp1ppbp/3p1np1/8/2PPP3/2N4P/PP3PP1/R1BQKBNR b KQkq - 0 5 ",
    "rnbq1rk1/ppp2ppp/3ppn2/8/2PPP3/P1P5/2Q2PPP/R1B1KBNR b KQ - 0 7 ",
    "rnbq1rk1/ppp1ppbp/3p1np1/8/2PP4/2N3PP/PP2PPB1/R1BQK1NR b KQ - 0 6 ",
    "rnbqk2r/ppp1ppbp/1n4p1/8/3PP3/6P1/PP2NPBP/RNBQK2R b KQkq - 2 7 ",
    "rnbq1rk1/ppp1bpp1/4p2p/3p4/2PPn2B/2N1PN2/PP3PPP/R2QKB1R w KQ - 1 8 ",
    "rnbqkb1r/1p2pppp/p2p1n2/8/P2NP3/2N5/1PP2PPP/R1BQKB1R b KQkq a3 0 6 ",
    "r1bqr1k1/2p2pp1/p2p1nnp/1pb1p3/4P3/1BPP1NNP/PP3PP1/R1BQR1K1 b - - 0 13 ",
    "r2q1rk1/2p1bppp/p1n2n2/1p1p4/3PP1b1/1B2BN2/PP3PPP/RN1QR1K1 w - - 0 12 ",
    "rnbqk2r/pp3ppp/2p1p3/8/PbBP4/2P2N2/5PPP/R1BQ1RK1 b kq - 0 9 ",
    "rnbqk2r/pp2bppp/4pn2/2p5/2p5/5NP1/PPQPPPBP/RNB2RK1 w kq - 2 7 ",
    "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 0 4 ",
    "r1k2b1r/p1pb1ppp/1pp5/4Pn2/8/2N2N1P/PPP2PP1/R1BR2K1 w - - 2 12 ",
    "r1bq1rk1/ppp1npbp/3p1np1/3Pp3/2P1P3/2N2N2/PP2BPPP/R1BQ1R1K b - - 2 9 ",
    "r1bq1rk1/pp1nbppp/2p1pn2/3p4/2PP4/5NP1/PPQBPPBP/RN3RK1 w - - 4 9 ",
    "rnbqk1nr/ppp1bppp/8/3p4/3P4/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 5 ",
    "r1bq1rk1/ppp2ppp/2nppn2/8/1bPPP3/2NB4/PPQ1NPPP/R1B1K2R b KQ - 3 7 ",
    "1rbqk2r/2p2ppp/pbnp1n2/Pp2p3/3PP3/1BP2N2/1P3PPP/RNBQ1RK1 b k - 0 10 ",
    "r2qkb1r/pp1npppp/2p2n2/5b2/P1NP4/2N5/1P2PPPP/R1BQKB1R b KQkq - 0 7 ",
    "r3r1k1/1pqbbppp/p1nppn2/8/P2NPP2/2N1B3/1PPQB1PP/R4R1K w - - 5 13 ",
    "rn1qk1nr/pp1bppbp/3p2p1/2p5/2B1PP2/2N2N2/PPPP2PP/R1BQK2R b KQkq - 5 6 ",
    "rnbqkb1r/ppp1pp1p/6p1/3n4/8/1QN2N2/PP1PPPPP/R1B1KB1R b KQkq - 1 5 ",
    "r1bq1rk1/pp3ppp/2n1p3/3n4/1b1P4/2NB4/PP2NPPP/R1BQ1RK1 w - - 2 10 ",
    "rnbqk2r/pp1p1ppp/4pn2/2p5/1bPP4/2N2P2/PP2P1PP/R1BQKBNR w KQkq c6 0 5 ",
    "r1b1kb1r/1p1n1pp1/p2p1n1p/4pN2/4PP1B/q1N5/P1PQB1PP/1R2K2R b Kkq - 1 12 ",
    "rnbqk2r/ppp1bppp/4pn2/3p4/2PP4/5NP1/PP1BPP1P/RN1QKB1R w KQkq - 3 6 ",
    "r1bq1rk1/ppp1ppbp/1nn3p1/8/3P4/2N1PNP1/PP3PBP/R1BQK2R w KQ - 1 9 ",
    "r1bqk2r/ppp1bppp/2n5/3p4/3P4/2PB1N2/P1P2PPP/R1BQ1RK1 b kq - 0 9 ",
    "r1b1qrk1/ppp2pbp/n2p2p1/4P3/2P1P1n1/2N1BN2/PP2BPPP/R2Q1RK1 w - - 1 10 ",
    "r2q1rk1/pp2bppp/2n2n2/2pp4/3P1Bb1/2NBP3/PP2NPPP/R2Q1RK1 w - - 4 10 ",
    "r1bq1rk1/pppnn1bp/3p2p1/3Ppp2/2P1P3/2NN4/PP1BBPPP/R2Q1RK1 b - - 1 11 ",
    "rnbqk2r/p2p1ppp/1p2pn2/2p5/1bPP4/P1N1P3/1P2NPPP/R1BQKB1R b KQkq - 0 6 ",
    "r2q1rk1/pp2ppbp/2n3p1/1B6/3PP1b1/4BN2/P4PPP/R2Q1RK1 w - - 3 12 ",
    "rn1qkb1r/pp3ppp/2p1pn2/3P1b2/8/5NP1/PP1PPPBP/RNBQ1RK1 b kq - 0 6 ",
    "r1bqkb1r/ppp2ppp/2n5/3np3/8/2N2NP1/PP1PPPBP/R1BQK2R b KQkq - 1 6 ",
    "rnbqkb1r/pp2pp1p/2p2np1/3p4/2PP4/2N1P3/PP3PPP/R1BQKBNR w KQkq - 0 5 ",
    "r1bq1rk1/pp1nbppp/4pn2/2p5/8/1QN2NP1/PP1PPPBP/R1B2RK1 b - - 5 9 ",
    "r1bqk2r/pppp1ppp/2n2n2/1Bb1p3/4P3/2PP1N2/PP3PPP/RNBQK2R b KQkq - 0 5 ",
    "r1bqk2r/ppp1ppbp/2n3p1/3n3P/8/2N2N2/PP1PPPP1/R1BQKB1R w KQkq - 1 7 ",
    "r1bqkb1r/pppp1ppp/5n2/4p3/2Pn4/2N2NP1/PP1PPP1P/R1BQKB1R w KQkq - 1 5 ",
    "r1bq1rk1/ppp2pbp/n2p1np1/3Pp3/2P1P1P1/2N1B3/PP2BP1P/R2QK1NR b KQ g3 0 8 ",
    "r1bq1b1r/ppp3k1/2np1n1p/8/2BPPBpP/2N5/PPP3P1/R2QK2R w KQ - 1 11 ",
    "r2qk2r/2p1bppp/p1n1b3/1p1pP3/4n3/1B2BN2/PPP2PPP/RN1Q1RK1 w kq - 3 10 ",
    "rnbq1rk1/ppp2pbp/3p1np1/8/2PNP3/2N5/PP2BPPP/R1BQ1RK1 b - - 0 8 ",
    "r2qrbk1/2pb1pp1/p2p1n1p/np2p3/3PP3/2P2NNP/PPB2PP1/R1BQR1K1 b - - 8 14 ",
    "r2q1rk1/ppp1ppbp/3p1np1/n2P4/2P3b1/2N2NP1/PP2PPBP/R1BQ1RK1 w - - 1 9 ",
    "rnbqkb1r/pp2pppp/5n2/3p4/3P4/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 5 ",
    "rnbq1rk1/ppp1bppp/5n2/3p4/3P4/P1N1P3/1P2NPPP/R1BQKB1R w KQ - 0 8 ",
    "r2qkb1r/2p2ppp/p1n1b3/1p1pP3/4n3/1B3N2/PPP2PPP/RNBQ1RK1 w kq - 1 9 ",
    "rnbqkb1r/ppp1pppp/3p1n2/8/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 1 3 ",
    "r4rk1/pp2ppbp/2n3p1/3q1b2/Q2P3B/4PN2/P3BPPP/R4RK1 b - - 4 13 ",
    "rnbqkbnr/pp1p1ppp/2p5/4p3/2PP4/6P1/PP2PP1P/RNBQKBNR b KQkq d3 0 3 ",
    "r1bq1rk1/pp1nbppp/2pp1n2/8/P1BpP3/2N2N2/1PP2PPP/R1BQR1K1 w - - 0 9 ",
    "rnbq1rk1/ppp2ppp/4pn2/3pP3/1bPP4/2N5/PPQ2PPP/R1B1KBNR b KQ - 0 6 ",
    "r1bqk2r/2p1bppp/p1np1n2/1p2p3/4P3/1B3N2/PPPP1PPP/RNBQR1K1 w kq - 0 8 ",
    "r1bq1rk1/pp3ppp/1bn5/3p4/5B2/P2BPN2/1P3PPP/R2QK2R w KQ - 3 12 ",
    "rnbqkb1r/pp3ppp/3p1n2/2pP4/P7/5N2/1P2PPPP/RNBQKB1R b KQkq a3 0 6 ",
    "rnbqkb1r/pp1ppppp/2p2n2/8/2P5/6P1/PP1PPP1P/RNBQKBNR w KQkq - 0 3 ",
    "r1bqkbnr/pppp2pp/2n5/1B2pp2/4P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 0 4 ",
    "rnbq1rk1/1p2ppbp/2p2np1/p2p4/P1PP4/2N1PN2/1P2BPPP/R1BQ1RK1 w - - 0 9 ",
    "rnbqk2r/1pp1ppbp/p2p1np1/8/2PPP3/2N2P2/PP4PP/R1BQKBNR w KQkq - 0 6 ",
    "rnbq1rk1/pp2ppbp/6p1/2p5/8/2P2NP1/P2PPPBP/R1BQ1RK1 w - c6 0 9 ",
    "r1bqk1nr/1pp2ppp/p1np4/2b1p3/2P5/2N1P1P1/PP1P1PBP/R1BQK1NR w KQkq - 0 6 ",
    "rnbqkb1r/p3pppp/1p3n2/2ppN3/8/6P1/PPPPPPBP/RNBQK2R w KQkq d6 0 5 ",
    "r2qkb1r/pp1n1pp1/2p1pn1p/4N3/3P1P1P/3Q2N1/PPP3P1/R1B1K2R w KQkq - 1 12 ",
    "rn1qr1k1/pppbbppp/3p1n2/1B1Pp3/4P3/2P2N1P/PP3PP1/RNBQR1K1 w - - 1 10 ",
    "rn1q1rk1/1bp2pbp/p2p1np1/1p2p3/P3P3/2PP1N2/1PBN1PPP/R1BQR1K1 w - - 1 12 ",
    "r2q1rk1/pp2ppbp/2n3p1/2p5/2BPP1b1/2P1BP2/P3N1PP/R2Q1RK1 b - - 0 11 ",
    "rnbq1rk1/ppppppbp/5np1/8/2P1P3/2N3P1/PP1P1PBP/R1BQK1NR b KQ e3 0 5 ",
    "rnbq1rk1/pp3pbp/3ppnp1/2pP4/2P1P3/2N2PN1/PP4PP/R1BQKB1R b KQ - 1 8 ",
    "r2qk2r/1p1nbppp/p1p1pnb1/8/2BP3N/2N1PPP1/PP1B3P/R2Q1RK1 b kq - 0 12 ",
    "r2qkb1r/1p1n1pp1/p2pbn2/4p2p/4P3/1NN1BP2/PPPQ2PP/R3KB1R w KQkq h6 0 10 ",
    "r1bqr1k1/ppp1bppp/1nn5/4p3/4N3/3PBNP1/PP2PPBP/R2Q1RK1 b - - 8 10 ",
    "r1bq1rk1/bpp1np1p/p2p1p2/4p3/4P3/1BPP1N1P/PP1N1PP1/R2QK2R w KQ - 0 11 ",
    "rnbqkb1r/3p1ppp/p3pn2/1PpP4/4P3/5P2/PP4PP/RNBQKBNR b KQkq e3 0 6 ",
    "r2q1rk1/pbpnbpp1/1p3n1p/3p4/3P3B/P1N1PN2/1PQ1BPPP/R3K2R w KQ - 2 12 ",
    "r1bqkb1r/ppppnppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4 ",
    "r2q1rk1/4bppp/p2p1n2/1pp5/2nPP1b1/5N2/PPB2PPP/RNBQR1K1 w - c6 0 14 ",
    "r1b2rk1/pp3pp1/1bn2q1p/3p4/1P3B2/P2BPN2/5PPP/R2Q1RK1 w - - 0 14 ",
    "r1bqkb1r/p2n1ppp/2p1pn2/1p6/3P4/2NQ1N2/PP2PPPP/R1B1KB1R w KQkq - 2 8 ",
    "rnbqkb1r/ppp1pp1p/3p1np1/8/3PP3/2N3P1/PPP2P1P/R1BQKBNR b KQkq - 0 4 ",
    "rnbq1rk1/pp2ppbp/2pp1np1/8/2PPP3/2N2N1P/PP3PP1/R1BQKB1R w KQ - 0 7 ",
    "rn1q1rk1/pbp1bppp/1p2pn2/3p4/2P5/1P2PNP1/PB1P1PBP/RN1Q1RK1 b - - 0 8 ",
    "r1bqk1nr/pp3ppp/2n5/2pp4/3P4/P1P1P3/5PPP/R1BQKBNR w KQkq - 0 8 ",
    "r1bqk1nr/pp2bppp/2pp4/n3p3/Q2PP3/2P2N2/P3BPPP/RNB1K2R w KQkq - 0 9 ",
    "rnbqkb1r/pppppppp/5n2/6B1/3P4/8/PPP1PPPP/RN1QKBNR b KQkq - 2 2 ",
    "rnb1k2r/1pp1qpp1/3p1n2/p2Pp2p/2P1P3/6P1/PP1N1PBP/R2QNRK1 w kq h6 0 12 ",
    "rnbqkbnr/pppppppp/8/8/6P1/8/PPPPPP1P/RNBQKBNR b KQkq g3 0 1 ",
    "rnb1kb1r/ppp2ppp/4pq2/8/2P5/2P5/P2P1PPP/R1BQKBNR w KQkq - 0 7 ",
    "rnbqkbnr/ppp2pp1/4p2p/3p4/3PP3/8/PPPN1PPP/R1BQKBNR w KQkq - 0 4 ",
    "r1bqkbnr/ppp2ppp/2n1p3/3p4/3PP3/8/PPPN1PPP/R1BQKBNR w KQkq - 2 4 ",
    "rnbqk1nr/ppp2ppp/4p3/3p4/1b1PP3/2N5/PPP1NPPP/R1BQKB1R b KQkq - 3 4 ",
    "rnb1k1nr/pp3ppp/4p3/q1ppP3/3P4/P1P5/2P2PPP/R1BQKBNR w KQkq - 1 7 ",
    "rn1qkb1r/p1pp1ppp/bp2pn2/8/2PP4/1P3NP1/P3PP1P/RNBQKB1R b KQkq - 0 5 ",
    "r1bqk2r/pp3ppp/2n1p3/2Pp4/2P1n3/P7/1PQ1PPPP/R1B1KBNR w KQkq - 1 9 ",
    "rnbqkb1r/ppp2ppp/4pn2/3p4/2PP4/5NP1/PP2PP1P/RNBQKB1R b KQkq - 0 4 ",
    "r1bqk1nr/pppp1ppp/2n5/2b5/3NP3/8/PPP2PPP/RNBQKB1R w KQkq - 1 5 ",
    "rnbqk1nr/1p1pbp1p/p3p1p1/8/4P1Q1/1N1B4/PPP2PPP/RNB1K2R w KQkq - 0 8 ",
    "r1bqkb1r/1pp2ppp/p1n1pn2/8/2pP4/5NP1/PP2PPBP/RNBQ1RK1 w kq - 2 7 ",
    "rnbqk2r/p1pp1p2/1p2p2p/6p1/1bPPn3/2N2NB1/PPQ1PPPP/R3KB1R b KQkq - 3 8 ",
    "rnbqkb1r/pp3ppp/3p1n2/2pPp3/2P5/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 5 ",
    "rnbqkb1r/ppp1pppp/5n2/3p4/8/1P4P1/P1PPPPBP/RNBQK1NR b KQkq - 0 3 ",
    "rnbqk2r/ppp1ppbp/3p1np1/8/3P4/1P3NP1/P1P1PPBP/RNBQK2R b KQkq - 0 5 ",
    "r1bqkbnr/pppp2pp/2n5/5p2/2P5/4Q1P1/PP2PP1P/RNB1KBNR b KQkq - 2 5 ",
    "r2qkb1r/2p2ppp/p1n1b3/1p1pP3/4n3/1B3N2/PPP1QPPP/RNB2RK1 b kq - 2 9 ",
    "rnbqkbnr/pp2pppp/2p5/3p4/3PP3/5P2/PPP3PP/RNBQKBNR b KQkq - 0 3 ",
    "r1bqkbnr/2pp1ppp/p7/np2p3/4P3/1B3N2/PPPP1PPP/RNBQK2R w KQkq - 2 6 ",
    "rnbqkbnr/pp1p1ppp/4p3/2p5/4P3/1P3N2/P1PP1PPP/RNBQKB1R b KQkq - 0 3 ",
    "rnbqkbnr/pp2pppp/8/3p4/3P4/8/PPP2PPP/RNBQKBNR w KQkq - 0 4 ",
    "r1bqkb1r/ppp2ppp/2n3n1/4P3/1PPp4/P4N2/4PPPP/RNBQKB1R w KQkq - 1 7 ",
    "rn1qk2r/pb1p1ppp/1p2pn2/2p5/1bPP4/1P3NP1/P2BPPBP/RN1QK2R w KQkq c6 0 8 ",
    "r2q1rk1/pp1n1ppp/2p1pn2/3p4/1bPP4/P1NBPQ1P/1P1B1PP1/R3K2R b KQ - 0 10 ",
    "rnbqkbnr/ppp1p1pp/5p2/3p2B1/3P4/8/PPP1PPPP/RN1QKBNR w KQkq - 0 3 ",
    "rnbqk2r/pp1pppbp/5np1/2p5/2PP4/5NP1/PP2PP1P/RNBQKB1R w KQkq c6 0 5 ",
    "r1bq1rk1/pp1n1ppp/4p3/3pP3/1bPNnB2/2NB4/PPQ2PPP/R3K2R b KQ - 2 10 ",
    "rnbq1rk1/pp1p1ppp/4pn2/8/1bPN4/2N3P1/PP2PPBP/R1BQK2R b KQ - 2 7 ",
    "r2qr1k1/1bppbpp1/p1n2n1p/4p3/Pp2P3/1BNP1N1P/1PP2PP1/R1BQR1K1 w - - 0 12 ",
    "rnbqkbnr/ppp2ppp/4p3/8/3PN3/8/PPP2PPP/R1BQKBNR b KQkq - 0 4 ",
    "rnbqk1nr/bp1p1ppp/p3p3/8/4P3/1N1B4/PPP2PPP/RNBQK2R w KQkq - 4 7 ",
    "2rqkb1r/1p1n1ppp/p1p1pn2/3p4/2PP2b1/1P1BPN2/P1QN1PPP/R1B1K2R w KQk - 1 9 ",
    "rnbqkb1r/ppp1pppp/8/3np3/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq - 0 5 ",
    "r2qkbnr/1ppb1ppp/p1np4/4p3/B3P3/2P2N2/PP1P1PPP/RNBQK2R w KQkq - 1 6 ",
    "r1bqkb1r/pp2pppp/2np1n2/8/3NP3/2N2P2/PPP3PP/R1BQKB1R b KQkq - 0 6 ",
    "rnbqkb1r/p2p1pp1/1p2pn1p/8/3P3B/5N2/PPP2PPP/RN1QKB1R w KQkq - 0 7 ",
    "1rbq1rnk/2p2ppp/p1np4/1pb1p3/P3P3/1BPP1N1P/1P1N1PP1/R1BQ1RK1 w - - 1 12 ",
    "rn1qkb1r/pp3ppp/3pbn2/4p3/2P1P3/1N3P2/PP4PP/RNBQKB1R b KQkq c3 0 7 ",
    "r2q1rk1/1p2bppp/pn1p1n2/4p3/4P1b1/2N1B1P1/PPPN1PBP/R2Q1RK1 w - - 10 12 ",
    "rnbqkbnr/pp1ppppp/8/8/4P3/2PB4/PP3PPP/RNBQK1NR b KQkq - 0 4 ",
    "r1b1k2r/1pqpbppp/p1n1pn2/8/3NP3/2N3P1/PPP2PBP/R1BQ1RK1 w kq - 5 9 ",
    "r3kbnr/ppp2ppp/2nq4/4p3/2B3b1/2N5/PPPPNPPP/R1BQ1RK1 w kq - 2 7 ",
    "rn2kbnr/pp3ppp/1qp1p3/3pP3/3P4/3Q1N2/PPP2PPP/RNB1K2R w KQkq - 1 7 ",
    "rnbq1rk1/1pp1bppp/p3pn2/8/2NP4/5NP1/PP2PPBP/R1BQK2R w KQ - 0 8 ",
    "r1bq1rk1/pp3pp1/2np1n1p/2p1p3/2PP4/P1PBPN2/2Q2PPP/R1B2RK1 w - - 0 11 ",
    "rnbq1rk1/ppppb1pp/4pn2/5p2/3P4/5NP1/PPP1PPBP/RNBQ1RK1 w - - 4 6 ",
    "rnbqk2r/pppp1ppp/4pn2/8/1bPP4/5N2/PP1NPPPP/R1BQKB1R b KQkq - 3 4 ",
    "r1bqkbnr/pp1ppppp/2n5/2p5/4P3/2NP4/PPP2PPP/R1BQKBNR b KQkq - 0 3 ",
    "r1bqk1nr/ppp1npbp/3p2p1/3Pp3/2P1P3/2N5/PP2NPPP/R1BQKB1R w KQkq - 1 7 ",
    "rn1qkbnr/pbpp1ppp/1p2p3/8/3PP3/3B1N2/PPP2PPP/RNBQK2R b KQkq - 3 4 ",
    "rnbqkbnr/ppp1pppp/8/3p4/3P4/2P5/PP2PPPP/RNBQKBNR b KQkq - 0 2 ",
    "rnb1kbnr/pp2pp1p/6p1/2pq4/8/N1P2N2/PP1P1PPP/R1BQKB1R b KQkq - 1 5 ",
    "r1b1k2r/pp1n1ppp/2p1pn2/q2p2B1/1bPP4/2N1PN2/PPQ2PPP/R3KB1R w KQkq - 3 8 ",
    "r1bq1rk1/pp1nppbp/3p2p1/8/3QP3/1BN5/PPP2PPP/R1B1R1K1 w - - 1 11 ",
    "rnbr2k1/ppp2pbp/5np1/4p3/2P1P3/2N2N2/PP2BPPP/R1B1K2R w KQ - 0 9 ",
    "rnbqkb1r/5ppp/p3pn2/1pp5/3P4/2N1PN2/PP2BPPP/R1BQK2R w KQkq c6 0 8 ",
    "rnbqkb1r/ppppp2p/5np1/5p2/3P4/2P3P1/PP2PPBP/RNBQK1NR b KQkq - 0 4 ",
    "r1bqkb1r/pp1p1ppp/2n1pn2/1N6/4P3/2N5/PPP2PPP/R1BQKB1R b KQkq - 3 6 ",
    "rnbqkbnr/pp3ppp/2p1p3/3p4/2PP4/5N2/PP2PPPP/RNBQKB1R w KQkq - 0 4 ",
    "rnbqk2r/pp2ppbp/2pp1np1/8/4PP2/2NP2P1/PPP3BP/R1BQK1NR b KQkq f3 0 6 ",
    "rnbqkbnr/1pp2ppp/3p4/p2Pp3/2P5/8/PP2PPPP/RNBQKBNR w KQkq a6 0 4 ",
    "rnbqkb1r/ppp2ppp/8/3p4/3PnB2/5N2/PPP2PPP/RN1QKB1R b KQkq - 1 6 ",
    "rnbqkb1r/pp2pppp/3p1n2/2p5/2B1P3/2N2N2/PPPP1PPP/R1BQK2R b KQkq - 1 4 ",
    "rnbqkbnr/pppp1ppp/8/4p3/8/1P6/P1PPPPPP/RNBQKBNR w KQkq e6 0 2 ",
    "rnbqkb1r/pp1ppppp/5n2/2p5/3P4/5N2/PPP1PPPP/RNBQKB1R w KQkq c6 0 3 ",
    "r1bqkbnr/ppp2ppp/2np4/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R w KQkq - 0 4 ",
    "rn1qkb1r/p1pp1ppp/bp2pn2/8/3P4/2P2NP1/PP2PPBP/RNBQK2R b KQkq - 2 5 ",
    "r1bq1rk1/p2nbpp1/1p2pn1p/3p4/3P4/2NBPN2/PPQ2PPP/R1B1R1K1 w - - 0 11 ",
    "rn1qkbnr/pp3ppp/2p1p1b1/8/2BP4/6N1/PPP1NPPP/R1BQK2R b KQkq - 1 7 ",
    "r1bqkb1r/pppnpppp/3p1n2/8/2PP4/2N5/PP2PPPP/R1BQKBNR w KQkq - 2 4 ",
    "r1bq1rk1/pp1pppbp/2n3p1/8/2PN2n1/2N3P1/PP2PPBP/R1BQ1RK1 w - - 3 9 ",
    "rnbqk2r/ppp1bppp/4p3/3pP3/2P1n2P/2N2N2/PP1P1PP1/R1BQKB1R b KQkq h3 0 6 ",
    "rBbqk2r/pp3p2/7p/2Pp2p1/1b2n3/2N5/PPQ1PPPP/R3KBNR b KQkq - 0 10 ",
    "rn2k2r/pp3p2/7p/q1Pp1bp1/1b2n3/2N1P1B1/PPQ1NPPP/R3KB1R w KQkq - 3 12 ",
    "rnbqk2r/pp3ppp/4pn2/2P5/2Pp4/P7/1PQ1PPPP/R1B1KBNR b KQkq - 1 8 ",
    "r1bqkbnr/pp1p1ppp/2n1p3/2p5/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 0 4 ",
    "rn1q1rk1/1p2bppp/p2pbn2/4p3/4P3/1NN1BP2/PPPQ2PP/2KR1B1R b - - 4 10 ",
    "rnbqk1nr/pp3pbp/2ppp1p1/8/2BPP3/2N2Q2/PPP2PPP/R1B1K1NR w KQkq - 0 6 ",
    "rnb1kb1r/ppp1pppp/5n2/q7/3P4/2N5/PPPB1PPP/R2QKBNR b KQkq - 2 5 ",
    "r1bq1rk1/4bppp/ppn1pn2/6B1/P1BP4/2N2N2/1P3PPP/R2QR1K1 w - - 0 12 ",
    "r1b1k2r/pp2bppp/2n1pn2/3q4/3p4/2P1BN2/PP2BPPP/RN1Q1RK1 w kq - 0 9 ",
    "r1bqkb1r/pp1p1ppp/2n1pn2/2p5/2P5/1P3N2/PB1PPPPP/RN1QKB1R w KQkq - 0 5 ",
    "rnbqkb1r/1p3ppp/p2ppn2/6B1/3NP3/2N5/PPP2PPP/R2QKB1R w KQkq - 0 7 ",
    "r1bqk2r/ppp1b1pp/1nn2p2/4p3/8/1P3NP1/PB1PPPBP/RN1Q1RK1 w kq - 0 9 ",
    "r1b2rk1/1pqp1ppp/p3pn2/6Q1/3bP3/2N5/PPP1BPPP/R1B2R1K w - - 5 12 ",
    "r1r3k1/pp2ppbp/3pbnp1/q7/3BP1PP/2N2P2/PPPQ4/1K1R1B1R w - - 1 14 ",
    "rnbqk2r/ppp2p2/4p1pp/3pP3/3Pn1Q1/2P5/P1PB1PPP/R3KBNR w KQkq - 0 9 ",
    "rn1qk1nr/pbppppbp/1p4p1/8/8/5NP1/PPPPPPBP/RNBQ1RK1 w kq - 2 5 ",
    "2rqkb1r/pp1bpppp/2np1n2/6B1/3NP3/2N5/PPPQ1PPP/R3KB1R w KQk - 7 8 ",
    "r1b2rk1/1pqpbppp/p1N1pn2/8/4P3/2N3P1/PPP2PBP/R1BQR1K1 b - - 0 10 ",
    "rnbqk2r/pp2bppp/4pn2/2P5/2Pp4/2N2N2/PPQ1PPPP/R1B1KB1R w KQkq - 0 7 ",
    "rnbqkb1r/ppp1pp1p/1n1p2p1/4P3/2BP4/5N2/PPP2PPP/RNBQK2R w KQkq - 2 6 ",
    "r1b1k2r/1pqp1ppp/p1n1pn2/8/1b1NP3/2N1B3/PPPQ1PPP/2KR1B1R w kq - 4 9 ",
    "rnbqk2r/1p3ppp/p3pn2/4N3/1bpB4/6P1/PP2PPBP/RN1QK2R w KQkq - 1 9 ",
    "2r2rk1/pp3ppp/2nqpn2/3p4/3P4/2NQPN2/PP3PPP/2R2RK1 w - - 4 13 ",
    "rnbqkbnr/ppppp1pp/8/5p2/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq e3 0 2 ",
    "rnbq1rk1/1p2ppbp/p2p1np1/2pP4/2P2B2/2N2N1P/PP2PPP1/R2QKB1R w KQ - 0 8 ",
    "r1bqkb1r/3n1ppp/p2p1n2/1p2p1B1/4P3/2N3P1/PPP1NP1P/R2QKB1R w KQkq - 2 9 ",
    "r1bq1rk1/pppp1ppp/1bn5/1B2P3/4n3/2P2N2/PP3PPP/RNBQ1RK1 w - - 0 8 ",
    "2rqr1k1/pp1bppb1/3p1np1/4n2p/3NP2P/1BN1BP2/PPPQ2P1/1K1R3R w - h6 0 14 ",
    "3q1rk1/pp1bppbp/3p1np1/8/2rNP1P1/2N1BP2/PPPQ4/2KR3R w - - 1 16 ",
    "rn2kb1r/1bp1pppp/p2q1n2/1p6/8/1BN2N2/PPPP1PPP/R1BQ1RK1 w kq - 2 8 ",
    "rnb2rk1/p3q1pp/1ppbpn2/3p1p2/2PP4/1P3NP1/PB1NPPBP/R2Q1RK1 w - - 0 10 ",
    "r1b1kb1r/pp2pppp/1nnq4/8/3p4/1BP2N2/PP3PPP/RNBQK2R w KQkq - 0 9 ",
    "r1bq1rk1/p3ppbp/2p3p1/3n4/8/2N1BP2/PPPQ2PP/2KR1B1R w - - 0 12 ",
    "1rbqkb1r/5ppp/p2p1n2/1p1Pp3/4P3/N3B3/PP3PPP/R2QKB1R w KQk - 1 12 ",
    "rn1q1rk1/1p1nbppp/3pb3/p3p1P1/4P2P/1NN1B3/PPP1BP2/R2QK2R w KQ - 0 12 ",
    "r1bqkb1r/p4ppp/2pppn2/8/4PB2/2N5/PPP2PPP/R2QKB1R w KQkq - 0 8 ",
    "r1bqk1nr/ppp2ppp/2n5/3p4/1b1P4/2NB4/PPP2PPP/R1BQK1NR w KQkq - 2 6 ",
    "rnb1kbnr/pp2pp1p/6p1/2pq4/3P4/2P5/PP3PPP/RNBQKBNR w KQkq - 0 5 ",
    "rnbqk1nr/ppp2ppp/1b2p3/3p4/1P2P3/P1N2N2/2PP1PPP/R1BQKB1R w KQkq - 1 6 ",
    "rnbqr1k1/ppp2pb1/3p1npp/4p1B1/2P5/2NP1NP1/PP2PPBP/R2Q1RK1 w - - 0 9 ",
    "r2qkb1r/5p1p/p1np4/1p1Ppp2/8/N2B4/PPP2PPP/R2Q1RK1 b kq - 0 13 ",
    "r1bqkb1r/pppp1ppp/2n5/3Pp3/4n3/2P2N2/PP3PPP/RNBQKB1R b KQkq - 0 5 ",
    "rn1qkb1r/pp2pppp/2p2n2/5b2/2QP4/5NP1/PP2PP1P/RNB1KB1R b KQkq - 0 6 ",
    "rn1qkb1r/ppp2p1p/4p3/3p1bp1/3PnB2/3BP3/PPPN1PPP/R2QK1NR w KQkq g6 0 7 ",
    "r2q1rk1/1bpn1pbp/p4np1/1p2p3/2P1P3/2N1BNPP/PP3PB1/R2Q1RK1 w - - 2 12 ",
    "r3kbnr/ppp1qppp/2n5/3p3b/3P4/3B1N1P/PPP1QPP1/RNB1K2R w KQkq - 5 8 ",
    "r1bqk2r/pppp1ppp/2n2n2/4p3/1bP5/2NP1N2/PP1BPPPP/R2QKB1R b KQkq - 2 5 ",
    "rnbqkb1r/pp3ppp/3p1n2/1B2p3/3NP3/5P2/PPP3PP/RNBQK2R b KQkq - 1 6 ",
    "rn1qkb1r/pp3ppp/2p1pn2/5b2/2BPN3/5N2/PPPB1PPP/R2QK2R w KQkq - 2 9 ",
    "rn1q1rk1/3pppbp/b4np1/2pP4/8/2N3P1/PP2PPBP/R1BQK1NR w KQ - 3 9 ",
    "rnbqkb1r/pp2pppp/5n2/2pp4/2P5/5NP1/PP1PPPBP/RNBQK2R b KQkq c3 0 4 ",
    "rnbqkb1r/pp3ppp/4p3/2pn4/2B5/2N1PN2/PP1P1PPP/R1BQK2R b KQkq - 1 6 ",
    "r2q1rk1/4ppbp/bn1p1np1/2pP4/8/2N2NP1/PP2PPBP/1RBQ1RK1 w - - 8 12 ",
    "r1bq1rk1/ppn1ppbp/3p1np1/2pP4/2P1P3/2N2N2/PP2BPPP/R1BQ1RK1 w - - 1 9 ",
    "rnbqkbnr/pp2pppp/8/3p4/2PP4/8/PP3PPP/RNBQKBNR b KQkq c3 0 4 ",
    "rn1q1rk1/pb1pbppp/1p2pn2/2p5/2P5/1P2PN2/PB1PBPPP/RN1Q1RK1 w - - 4 8 ",
    "r1b2rk1/1pqn1pbp/p2ppnp1/8/2P1PP2/3B1N2/PP4PP/RNB1QR1K w - - 1 12 ",
    "r1bqkb1r/1p1p1pp1/p1n1pn2/7p/3NP1P1/2N1B3/PPP2P1P/R2QKB1R w KQkq h6 0 8 ",
    "r1b2rk1/pp1nqppp/2n1p3/2bpP3/3N1P2/2N1B3/PPPQ2PP/2KR1B1R w - - 5 11 ",
    "rn3rk1/pb1pqpp1/1p2p2p/3n4/3P4/3BPN2/PP1N1PPP/R2Q1RK1 w - - 0 12 ",
    "r1bqk1nr/ppp2pb1/2np2p1/4p2p/2P5/2N1P1P1/PP1PNPBP/R1BQK2R w KQkq h6 0 7 ",
    "rnbqkb1r/ppp1pppp/5n2/1B1P4/8/8/PPPP1PPP/RNBQK1NR b KQkq - 2 3 ",
    "rnbqk2r/ppp1ppbp/5np1/6B1/2pPP3/2N2N2/PP3PPP/R2QKB1R b KQkq e3 0 6 ",
    "r1bq1rk1/1p3ppp/2nppn2/p7/2PP4/PQ3NP1/4PPBP/RN3RK1 b - - 0 11 ",
    "r2q1rk1/pp2bppp/2n1p3/8/2BP2b1/2P1BN2/P4PPP/R2Q1RK1 w - - 1 12 ",
    "rnbqkb1r/pp3ppp/2p2n2/3pp3/8/3P2P1/PPPNPPBP/R1BQK1NR w KQkq e6 0 5 ",
    "rnbqkb1r/pp1ppppp/5n2/2pP4/8/2N5/PPP1PPPP/R1BQKBNR b KQkq - 2 3 ",
    "r2q1rk1/1p1nbppp/p1p1p3/P2nP2b/3PN3/3B1N2/1P2QPPP/R1B2RK1 w - - 0 15 ",
    "r1bqr1k1/pppn1pbp/3p1np1/8/2PNP3/2N3P1/PP3PBP/R1BQR1K1 b - - 2 10 ",
    "rnbq1rk1/pp4bp/3ppnp1/2p5/2P1PP2/2N2N2/PP4PP/R1BQKB1R w KQ - 0 9 ",
    "rn1q1rk1/pp3pbp/3pbnp1/2p5/2P1PP2/2N2N2/PP4PP/R1BQKB1R w KQ - 0 9 ",
    "rnbq1rk1/pp1p1ppp/4pn2/8/1PP5/2p1P3/1P2NPPP/R1BQKB1R w KQ - 0 8 ",
    "rnbq1rk1/1p2ppbp/p2p1np1/2pP4/P1P1PP2/2N2N2/1P4PP/R1BQKB1R b KQ a3 0 8 ",
    "rnbq1rk1/pp1p1ppp/4pn2/2p3B1/1bPP4/2N1PN2/PP3PPP/R2QKB1R b KQ - 0 6 ",
    "r1bqkbnr/pppp2pp/2n5/1B2pp2/4P3/2N2N2/PPPP1PPP/R1BQK2R b KQkq - 1 4 ",
    "r2qr1k1/pbpn1pp1/1p3n1p/3p4/1b1P3B/2NBPN2/PP3PPP/2RQ1RK1 w - - 6 12 ",
    "rn1qkbnr/ppp1pppp/8/3P4/3P4/8/PP2PPPP/RbBQKBNR w KQkq - 0 4 ",
    "rnbqkbnr/pp1ppp1p/2p3p1/8/3PP2P/8/PPP2PP1/RNBQKBNR b KQkq h3 0 3 ",
    "r2q1rk1/pb1pbppp/1pn1pn2/2p5/8/1P1P1NP1/PBPNPPBP/R2Q1RK1 w - - 3 9 ",
    "rnbq1rk1/pp1p1ppp/4pn2/2P3B1/1bP5/2N5/PPQ1PPPP/R3KBNR b KQ - 2 6",
    ///// NEW
    "rn1qk2r/1b2bppp/p3pn2/1pp5/3PP3/1BN2N2/PP3PPP/R1BQ1RK1 w kq - 1 11",
    "r2q1rk1/pb1n1ppp/1p1bpn2/2pp4/2PP4/1PNBPN2/PB3PPP/R2Q1RK1 w - c6 0 10",
    "r3k2r/2p1qppp/p1n1b3/1pbpP3/4n3/1B2BN2/PPP1QPPP/RN3RK1 w kq - 5 11",
    "r2q1rk1/pp2bppp/2npbn2/2p5/3P4/2P2N1P/PPB2PP1/RNBQ1RK1 w - - 1 11",
    "r1bqk2r/1p2bppp/p1n1pn2/8/3P4/1BN2N2/PP3PPP/R1BQ1RK1 w kq - 3 11",
    "r2q1rk1/1bp1bppp/p1n2n2/1p2p3/4p3/1BPP1N2/PP1N1PPP/R1BQR1K1 w - - 0 11",
    "r3k2r/pp1nbpp1/1qp1p2n/3pPb2/3P3p/1QP1BN1P/PP1NBPP1/R3K2R w KQkq - 3 11",
    "r3kb1r/pp2pp1p/2n1b1p1/q3P3/3p2nQ/2N2N2/PPPB1PPP/2KR1B1R w kq - 1 11",
    "2r1kb1r/1p3ppp/p1npbq2/4p3/2B1P3/4BN2/PPP2PPP/R2QK2R w KQk - 0 11",
    "r1b2rk1/1pq1bppp/p1nppn2/8/P2NPP2/2N1B3/1PP1B1PP/R2Q1RK1 w - - 1 11",

    "rnb2rk1/pp3ppp/4pn2/q7/2B1P3/2b5/PPNB1PPP/R2QK2R w KQ - 0 11",
    "r1b1kb1r/pp1n3p/2p2pp1/4p3/4PB2/2P5/PPKN1PPP/R4B1R w kq e6 0 11",
    "r1bq1rk1/pp1n1ppp/3b1n2/3p4/3pP3/2NB1N2/PPQ2PPP/R1B2RK1 w - - 0 11",
    "r1bqr1k1/pp1n1p1p/2p2np1/2b1p3/4P2N/2N3P1/PPP1QPBP/R1B2RK1 w - - 0 11",
    "r1bqk2r/1p2bppp/p1n1pn2/4N3/3P4/3B4/PP3PPP/RNBQ1RK1 w kq - 3 10",
    "r1bqk2r/1p2ppb1/p1np3p/6p1/2BNP1n1/2N3B1/PPP2PPP/R2QK2R w KQkq - 4 11",
    "r1b1k1r1/ppp2p1p/2p2pn1/2b5/4P3/2N5/PPPB1PPP/R3KB1R w KQq - 2 11",
    "r1bqr1k1/ppp2pbp/3p1np1/2n5/2P5/1PN2NP1/PB1QPPBP/R3K2R w KQ - 7 11",
    "r3kbnr/pR3ppp/2n1p3/3pPb2/2pP4/2N1BN2/P1P1BPPP/4K2R w Kkq - 1 11",
    "r2qk2r/pp2bppp/2n1pnb1/2pp4/4P1P1/2PP1N1P/PPBNQP2/R1B1K2R w KQkq - 1 11",

    "r3k2r/pppb1pb1/2nppq1p/8/3PPP2/2N5/PPPQ3P/2KR1BNR w kq - 1 11",
    "r2q1rk1/1p1nbppp/p2pbn2/4p3/4P3/1NN1BP2/PPPQ2PP/2KR1B1R w - - 8 11",
    "r2q1rk1/ppp1bppp/1n2b3/4p3/1P1n4/P1NP1NP1/4PPBP/R1BQ1RK1 w - - 1 11",
    "r1bq1rk1/pp1n1pp1/4pb1p/2p5/2BP4/2N1PN2/PP1Q1PPP/R4RK1 w - c6 0 11",
    "rn1qkb1r/pb3p2/2p1p2p/1p2P1pn/2pP4/2N2NB1/PP2BPPP/R2QK2R w KQkq - 1 11",
    "r1bqk2r/1p3ppp/p2p1n2/4n3/1b1NP3/2NBB3/PPP3PP/R2Q1RK1 w kq - 0 11",
    "r2q1rk1/1p2bppp/p1npbn2/4p1B1/2B1P3/2N2N2/PPP2PPP/R2Q1RK1 w - - 8 11",
    "r1bqkb1r/pp4pp/2n1pp2/3n2B1/3N4/2P2N2/PP3PPP/R2QKB1R w KQkq - 0 11",
    "r1b1kb1r/1pq2pp1/p2p1n1p/4pN2/4PP2/2N5/PPP1Q1PP/R3KB1R w KQkq - 0 11",
    "r1bqk2r/3nbppp/2p1p3/p2nP3/PpBP4/5N2/NP1B1PPP/R2Q1RK1 w kq - 2 12"

};
void SetStartingPosition(position_t *pos,int position) {
    setPosition(pos, FenStartString[position]);
}
#endif
#ifdef OPTIMIZE

void SetNewGame() {
    origScore = 0;
    transClear(0);
    //TODO consider clearing pawn and eval hash
}
int PlayOptimizeGame(position_t *pos, int startPos, int time) {
    pos_store_t undo;
    int zeros = 0;
    int wScore;
    int bScore;
    int value;
    int moves = 0;
    SetStartingPosition(pos,startPos);

    while (true) {
        char s[255];
        SetNewGame();
        origScore = 0;
        if (time<4) sprintf(s,"go wtime 10000 winc 0 btime 10000 binc 0");
        else sprintf(s,"go wtime 80000 winc 200 btime 80000 binc 200");

        uciGo(pos,s);
        if (SearchInfo(0).last_value != -10000) value = SearchInfo(0).last_value;
        else value = SearchInfo(0).last_last_value;
        if (pos->side == WHITE && value != -10000) wScore = value;
        if (pos->side == BLACK && value != -10000) bScore = value;
        if (wScore > 400 && bScore < -400) {
            return 1;
        }
        if (bScore > 400 && wScore < -400) {
            return 2;
        }
        else if (moves > 500) return 0;
        else if (SearchInfo(0).last_value ==0) {

            zeros++;
            if (zeros>5) {
                return 0;
            }
        }
        else if (value != -10000) zeros = 0;
        if (!SearchInfo(0).bestmove) {
            if (kingIsInCheck(pos)) {
                if (pos->side == WHITE) return 2;
                else return 1;
            }
            return 0; //stalemate
        } else {
            origScore = value; // just to be safe
        }
        Print(3,"%d %s ",moves,move2Str(SearchInfo(0).bestmove));
        if (moves%10==9 && pos->side == BLACK) Print(3,"\n");
        makeMove(pos, &undo, SearchInfo(0).bestmove);
        if (pos->posStore.fifty==0) {
            pos->stack[0] = pos->hash;
            pos->sp = 0;
        }
        if (anyRep(pos)) {
            return 0;
        }
        moves++;
    }
    Print(3,"info string why here??\n");
    return -1;
}
void optimize(position_t *pos, int threads) {
    Guci_options.threads = threads;

    for (int gameOn=0; gameOn < 21; gameOn++) {
        int position = rand()%NUM_START_POS;
        Print(3,"************ STARTING GAME %d %s\n",gameOn+1,FenStartString[position]);
        PlayOptimizeGame(pos,position,gameOn/5);
    }
}
#endif
#ifdef SELF_TUNE2
int totalGames = 0;
Personality AllPersonalities[MAX_PERSONALITIES];
int numPersonalities=0;

void InitSimilarity(int p) {
    for (int i = 0; i < numPersonalities; i++) {
        for (int k=0; k < NUM_GENOMES; k++) {
            float sim = AllPersonalities[p].similarity(AllPersonalities[i],k);
            //if (i!=p) {
            AllPersonalities[p].updateRatingWeight(sim,AllPersonalities[i],k,true);
            //}
            //	Print(3,"%s *** %d ",AllPersonalities[i].PrettyString(),k);
            //	Print(3,"%f *** %s\n",sim,AllPersonalities[p].PrettyString());
        }
    }
}
void UpdateSimilarity(int p, bool increment) { //increment determines whether to remove or add similarty
    for (int i = 0; i < numPersonalities; i++) {
        for (int k=0; k < NUM_GENOMES; k++) {
            float sim = AllPersonalities[p].similarity(AllPersonalities[i],k);
            AllPersonalities[i].updateRatingWeight(sim,AllPersonalities[p],k,increment);
            if (i!=p) { //don't double count
                AllPersonalities[p].updateRatingWeight(sim,AllPersonalities[i],k,increment);
            }
        }
    }
}
bool ToChange(const int games,const int mult) {
    int ch = 100;

    //more people means generate less people
    if (numPersonalities >= MAX_PERSONALITIES) ch *=20;
    else if (numPersonalities > MAX_PERSONALITIES/2) ch *=16;
    else if (numPersonalities > MAX_PERSONALITIES/3) ch *=14;
    else if (numPersonalities > MAX_PERSONALITIES/4) ch *=12;
    else if (numPersonalities > MAX_PERSONALITIES/5) ch *=10;
    else if (numPersonalities > MAX_PERSONALITIES/6) ch *=8;
    else if (numPersonalities > MAX_PERSONALITIES/7) ch *=6;
    else if (numPersonalities > MAX_PERSONALITIES/8) ch *=4;
    else if (numPersonalities > MAX_PERSONALITIES/9) ch *=3;
    else if (numPersonalities > MAX_PERSONALITIES/10) ch *=2;

    // more games played means greater chance
    if (games > 40000) ch /= 10;
    else if (games > 20000) ch /= 9;
    else if (games > 10000) ch /= 8;
    else if (games > 6400) ch /= 7;
    else if (games > 3200) ch /= 6;
    else if (games > 1600) ch /= 5;
    else if (games > 800) ch /= 4;
    else if (games > 400) ch /= 3;
    else if (games > 200) ch /= 2;
    else if (games < 100) return false;
    ch /= mult;
    return (rand()%ch == 0);
}
// bubble 
void SortPlayers() {
    int done;
    do {
        int i;
        done = true;
        for (i=1; i < numPersonalities;i++) {
            if (AllPersonalities[i].rating > AllPersonalities[i-1].rating) {
                AllPersonalities[i].Swap(&AllPersonalities[i-1]);
                done = false;
            }
        }
    } while (!done);
}
double GetWinP(int p) {
    if (AllPersonalities[p].games <= 0) return 0.5; // 0 / 0 is considered winning 1/2 your games I guess
    return ((double) AllPersonalities[p].points / (double) AllPersonalities[p].games);
}
int GetBestGenome(int i) {
    float bestScore = -10000.0;
    int bestP = 0;
    for (int j = 0; j < numPersonalities; j++) {
        if (AllPersonalities[j].GetSimilarityRating(i) > bestScore) {
            bestP = j;
            bestScore = AllPersonalities[j].GetSimilarityRating(i);
        }
    }	
    return bestP;
}
int GetBestSimilar () {
    float bestScore = -10000.0;
    int bestP = 0;
    for (int i = 0; i < numPersonalities; i++) {
        if (AllPersonalities[i].GetSimilarityRating() > bestScore) {
            bestP = i;
            bestScore = AllPersonalities[i].GetSimilarityRating();
        }
    }
    return bestP;
}
void ShowActive() {
    int i;
    SortPlayers();
    Personality DefaultP;
    int bestP = GetBestSimilar();

    Print(3,"\n");
    for (i = 0; i < MIN(numPersonalities,MAX_SHOW); i++) {
        if (DefaultP.Equal(AllPersonalities[i])) Print(3,"*");
        else if (bestP==i) Print(3,"@");
        else Print(3," ");
        if (i < 9) Print(3," ");
        Print(3,"%d %.1f[%.1f] (%.2f(",i+1,AllPersonalities[i].rating,AllPersonalities[i].GetSimilarityRating(),GetWinP(i));
        Print(3,"%d): %s\n",AllPersonalities[i].games/2,AllPersonalities[i].PrettyString());
    }
    if (bestP >= i) {
        Print(3,"@");
        if (bestP < 9) Print(3," "); // will be a little off if position > 100
        Print(3,"%d %.1f[%.1f] (%.2f(",bestP+1,AllPersonalities[bestP].rating,AllPersonalities[bestP].GetSimilarityRating(),GetWinP(bestP));
        Print(3,"%d): %s\n",AllPersonalities[bestP].games/2,AllPersonalities[bestP].PrettyString());
    }
    for (;i < numPersonalities;i++) {
        if (DefaultP.Equal(AllPersonalities[i])) { 
            Print(3,"*");
            if (i < 9) Print(3," ");
            Print(3,"%d %.1f[%.1f] (%.2f(",i+1,AllPersonalities[i].rating,AllPersonalities[i].GetSimilarityRating(),GetWinP(i));
            Print(3,"%d): %s\n",AllPersonalities[i].games/2,AllPersonalities[i].PrettyString());
            break;
        }
    }
    Print(3,"--------------------------------------------------------------\n");
    if (AllPersonalities[bestP].numToMutate>1) {
        for (int i= 0; i < NUM_GENOMES; i++) {
            if (AllPersonalities[bestP].toMutate[i]) {
                bestP = GetBestGenome(i);
                Print(3,"%d",i+1);
                if (bestP < 9) Print(3," "); // will be a little off if position > 100
                Print(3,"%d %.1f[%.1f] (%.2f(",bestP+1,AllPersonalities[bestP].rating,AllPersonalities[bestP].GetSimilarityRating(),GetWinP(bestP));
                Print(3,"%d): %s\n",AllPersonalities[bestP].games/2,AllPersonalities[bestP].PrettyString());
            }
        }
    }
    Print(3,"TOTAL: %d GAMES: %d AVERAGE %.1f\n",numPersonalities,totalGames,((float)totalGames*2.0)/(float)numPersonalities);

}
void SetNewGame() { //this should be changed when we do one thread per personality
    origScore = 0;
    //	transClear();
    for (int i = 0; i < Guci_options.threads; ++i) {
        pawnTableClear(&Threads[i].pt);
        evalTableClear(&Threads[i].et);
    }
}
void tuneGo(position_t *pos, int player, int64 nodes) {

    ASSERT(pos != NULL);
    ASSERT(options != NULL);
    //	uciGo(pos,"nodes 5000");
    //	return;
    /* initialization */
    SearchInfo(player).depth_is_limited = false;
    SearchInfo(player).depth_limit = MAXPLY;
    SearchInfo(player).moves_is_limited = false;
    SearchInfo(player).time_is_limited = false;
    SearchInfo(player).time_limit_max = 0;
    SearchInfo(player).time_limit_abs = 0;
    SearchInfo(player).node_is_limited = true;
    SearchInfo(player).node_limit = nodes;
    SearchInfo(player).start_time = SearchInfo(player).last_time = getTime();
    SearchInfo(player).alloc_time = 0;
    SearchInfo(player).best_value = -INF;
    SearchInfo(player).last_value = -INF;
    SearchInfo(player).last_last_value = -INF;
    SearchInfo(player).change = 0;
    SearchInfo(player).research = 0;
    SearchInfo(player).bestmove = 0;
    SearchInfo(player).pondermove = 0;
    SearchInfo(player).mate_found = 0;

    memset(Threads[player].history, 0, sizeof(Threads[player].history));
    memset(Threads[player].evalvalue, 0, sizeof(Threads[player].evalvalue));
    memset(Threads[player].evalgains, 0, sizeof(Threads[player].evalgains));
    Threads[player].nodes_since_poll = 0;
    Threads[player].nodes_between_polls = 8192;
    Threads[player].nodes = 0;
    memset(Threads[player].killer1, 0, sizeof(Threads[player].killer1));
    memset(Threads[player].killer2, 0, sizeof(Threads[player].killer2));

    memset(SearchInfo(player).moves, 0, sizeof(SearchInfo(player).moves));
    SearchInfo(player).thinking_status = THINKING;
    getBestMove(pos, player);
    if (!SearchInfo(player).bestmove) {
        if (RETURN_MOVE)
            Print(3, "info string No legal move found. Start a new game.\n\n");
        return;
    } else {
        if (RETURN_MOVE) {
            Print(3, "bestmove %s", move2Str(SearchInfo(player).bestmove));
            if (SearchInfo(player).pondermove) Print(3, " ponder %s", move2Str(SearchInfo(player).pondermove));
            Print(3, "\n\n");
        }
        origScore = SearchInfo(player).last_value; // just to be safe
    }
}
double ProbWin(double rating1, double rating2) {
    double win;
    double dif = rating1 - rating2;
    if (dif < 0) dif = -dif;
    //	Print(3,"\n rating dif %f ",dif);
    if (dif == 0 ) win = 0.5; // should centralize these scores sometime
    else if (dif < 7.0) win = 0.51;
    else if (dif < 14.0) win = 0.52;
    else if (dif < 21.0) win = 0.53;
    else if (dif < 29) win = 0.54;
    else if (dif < 36) win = 0.55;
    else if (dif < 43) win = 0.56;
    else if (dif < 50) win = 0.57;
    else if (dif < 57) win = 0.58;
    else if (dif < 65) win = 0.59;
    else if (dif < 72) win = 0.60;
    else if (dif < 80) win = 0.61;
    else if (dif < 87) win = 0.62;
    else if (dif < 95) win = 0.63;
    else if (dif < 102) win = 0.64;
    else if (dif < 110) win = 0.65;
    else if (dif < 117) win = 0.66;
    else if (dif < 125) win = 0.67;
    else if (dif < 133) win = 0.68;
    else if (dif < 141) win = 0.69;
    else if (dif < 149) win = 0.70;
    else if (dif < 158) win = 0.71;
    else if (dif < 166) win = 0.72;
    else if (dif < 175) win = 0.73;
    else if (dif < 184) win = 0.74;
    else if (dif < 193) win = 0.75;
    else if (dif < 202) win = 0.76;
    else if (dif < 211) win = 0.77;
    else if (dif < 220) win = 0.78;
    else if (dif < 230) win = 0.79;
    else if (dif < 240) win = 0.80;
    else if (dif < 251) win = 0.81;
    else if (dif < 262) win = 0.82;
    else if (dif < 273) win = 0.83;
    else if (dif < 284) win = 0.84;
    else if (dif < 296) win = 0.85;
    else if (dif < 309) win = 0.86;
    else if (dif < 322) win = 0.87;
    else if (dif < 335) win = 0.88;
    else if (dif < 351) win = 0.89;
    else if (dif < 366) win = 0.90;
    else if (dif < 383) win = 0.91;
    else if (dif < 401) win = 0.92;
    else if (dif < 422) win = 0.93;
    else if (dif < 444) win = 0.94;
    else if (dif < 470) win = 0.95;
    else if (dif < 501) win = 0.96;
    else if (dif < 538) win = 0.97;
    else if (dif < 589) win = 0.98;
    else  win = 0.99;

    if (rating2 > rating1) return (1.0-win);
    return win;
}
void Rate(int player, double expResult, double result, int opp) {
    double k;
    int g = AllPersonalities[player].games;
    int oppG = AllPersonalities[opp].games;

    // the less games an opponent has the less reliable the new information
    if (oppG <= MIN_GAMES) k = 2;
    else if (oppG <= MIN_GAMES*2) k = 4;
    else if (oppG <= MIN_GAMES*4) k = 6;
    else if (oppG <= MIN_GAMES*8) k = 8;
    else if (oppG <= MIN_GAMES*16) k = 10;
    else if (oppG <= MIN_GAMES*32) k = 12;
    else if (oppG <= MIN_GAMES*64) k = 14;
    else k = 16;
    // the more games you have the more accurate your current rating;
    k -= AllPersonalities[player].gameWeight();

    if (k < 1) k = 1;
    {
        double change = (k) * (result - expResult);
        AllPersonalities[player].rating += change;
        //		Print(3,"\n%d(%f) changed %f\n",player+1,expResult,change);
    }
}
void Provisional(int player) {
    int wins = AllPersonalities[player].points;
    int losses = AllPersonalities[player].games - AllPersonalities[player].points;
    if (AllPersonalities[player].games <= 0) return;
    AllPersonalities[player].rating = (((160*losses) + (240*wins)) / AllPersonalities[player].games)*10;
}
void RateDraw(int p1, int p2) {
    double winP = ProbWin(AllPersonalities[p1].rating,AllPersonalities[p2].rating);
    AllPersonalities[p1].games +=2;
    AllPersonalities[p2].games +=2;
    AllPersonalities[p1].points +=1;
    AllPersonalities[p2].points +=1;
    if (AllPersonalities[p1].games <= MIN_GAMES) Provisional(p1);
    else Rate(p1,winP,0.5,p2);;
    if (AllPersonalities[p2].games <= MIN_GAMES) Provisional(p2);
    else Rate(p2,1.0-winP,0.5,p1);
}
void RateWin(int winner, int loser) {
    double winP = ProbWin( AllPersonalities[winner].rating,AllPersonalities[loser].rating);
    AllPersonalities[winner].games +=2;
    AllPersonalities[loser].games +=2;
    AllPersonalities[winner].points +=2;
    if (AllPersonalities[winner].games <= MIN_GAMES) Provisional(winner);
    else Rate(winner,winP,1.0,loser);
    if (AllPersonalities[loser].games <= MIN_GAMES) Provisional(loser);
    else Rate(loser,1.0-winP,0.0,winner);

}

int PlayTuneGame(int startPos, int player1, int player2) {
    pos_store_t undo;
    int zeros = 0;
    int wScore = 0;
    int bScore = 0;
    int value;
    int moves = 0;
    int64 nodeTweak = rand()%(TUNE_NODES/10+1 - TUNE_NODES/20);
    position_t pos;
    int activePlayer;

    SetStartingPosition(&pos,startPos);
    //	Print(3,"%s\n",FenStartString[startPos]);
    activePlayer = (pos.side==WHITE ? player1 : player2);
    transClear(player1); transClear(player2);
    while (true) {
        origScore = 0;
        pawnTableClear(&Threads[activePlayer].pt);
        evalTableClear(&Threads[activePlayer].et);
        tuneGo(&pos,activePlayer,TUNE_NODES+nodeTweak);
        if (SearchInfo(activePlayer).last_value != -10000) value = SearchInfo(activePlayer).last_value;
        else value = SearchInfo(activePlayer).last_last_value;
        if (pos.side == WHITE && value != -10000) wScore = value;
        if (pos.side == BLACK && value != -10000) bScore = value;
        if (wScore > 400 && bScore < -400) {
            return 1;
        }
        if (bScore > 400 && wScore < -400) {
            return 2;
        }
        else if (moves > 500) return 0;
        else if (SearchInfo(activePlayer).last_value ==0) {

            zeros++;
            if (zeros>5) {
                return 0;
            }
        }
        else if (value != -10000) zeros = 0;
        if (!SearchInfo(activePlayer).bestmove) {
            if (kingIsInCheck(&pos)) {
                if (pos.side == WHITE) return 2;
                else return 1;
            }
            return 0; //stalemate
        } else {
            origScore = value; // just to be safe
        }
        //		Print(3,"%d %s ",moves,move2Str(SearchInfo(activePlayer).bestmove));
        //		if (moves%10==9 && pos.side == BLACK) Print(3,"\n");
        //		Print(3,"%d",activePlayer/2);
        makeMove(&pos, &undo, SearchInfo(activePlayer).bestmove);
        activePlayer = (pos.side==WHITE ? player1 : player2);
        if (pos.posStore.fifty==0) {
            pos.stack[0] = pos.hash;
            pos.sp = 0;
        }
        if (anyRep(&pos)) {
            return 0;
        }
        moves++;
    }
    Print(3,"info string why here??\n");
    return -1;
}
int FindPlayer(Personality p) {
    for (int i = 0; i < numPersonalities; i++) {
        if (p.Equal(AllPersonalities[i])) return i;
    }
    return -1;
}
void PlayRatedGame(int startPos, int player1, int player2) {
    int result = PlayTuneGame(startPos,player1, player2);
    MutexLock(SMPLock);
    //first find self
    int playerIndex1 = FindPlayer(personality(player1));
    int playerIndex2 = FindPlayer(personality(player2));
    if (playerIndex1>=0 && playerIndex2>= 0) {//this is true unless during battle one of them was removed for a new addition (unlikely but possible)
        UpdateSimilarity(playerIndex1,false);
        UpdateSimilarity(playerIndex2,false);
        if (result==1) RateWin(playerIndex1,playerIndex2);
        else if (result==2) RateWin(playerIndex2,playerIndex1);
        else RateDraw(playerIndex1,playerIndex2);
        totalGames++;
        UpdateSimilarity(playerIndex1,true);
        UpdateSimilarity(playerIndex2,true);
    }
    MutexUnlock(SMPLock);
}
bool AddPersonality(int p) {
    for (int i = 0; i < numPersonalities; i++) {
        if (personality(p).Equal(AllPersonalities[i])) {
            personality(p).Copy(AllPersonalities[i]); //make sure you have uptodate game and rating info and such
            return false;
        }
    }
    personality(p).SetDefaultRating(); //if it has not already been done, make sure we have a pure new rating
    if (numPersonalities == MAX_PERSONALITIES) {
        SortPlayers(); //make sure we are replacing the worst rating
        AllPersonalities[numPersonalities-1].Copy(personality(p));
        InitSimilarity(numPersonalities-1);
    }
    else {
        AllPersonalities[numPersonalities].Copy(personality(p));
        InitSimilarity(numPersonalities);
        numPersonalities++;
    }
    return true;
}

void InitTune() {
    DrawValue[WHITE] = 0;
    DrawValue[BLACK] = 0;
    MutexInit(SMPLock,NULL);
    srand(getTime());
    AllPersonalities[numPersonalities++].SetDefaults(); // lets seed the default
    //	Print(3,"%d %d %d %d %d %d\n",rand(),rand(),rand(),rand(),rand(),rand());
    //	Print(3,"%d %d %d %d %d %d\n",Random(1,10),Random(1,10),Random(1,10),Random(1,10),Random(1,10),Random(1,10));
}
bool InUse(int p) {
    for (int i=0; i < MaxNumOfThreads; i++) {
        if (personality(i).Equal(AllPersonalities[p])) return true;
    }
    return false;
}
int PickOpponent() {
    int opponent;
    int score = INT_MAX;
    if (rand()%SUFFICIENT_DIVERSITY==0) { //give games to default personality
        Personality DefaultP;
        Print(3,"*");
        for (int i = 0; i < numPersonalities; i++) {
            if (DefaultP.Equal(AllPersonalities[i])) {
                if (InUse(i)) break;
                return i;				
            }
        }
    }
    if (rand()%10==0 && numPersonalities > SUFFICIENT_DIVERSITY) { //pick one of the opponents with one of the best genomes
        int g = rand()%AllPersonalities[0].numToMutate;
        for (int i = 0; i < NUM_GENOMES;i++) {
            if (AllPersonalities[0].toMutate[i] ) {
                g--;
                if (g<0) {
                    opponent = GetBestGenome(i);
                    if (!InUse(opponent)) return opponent;
                }
            }
        }
    }
    if (rand()%5==0 && numPersonalities > SUFFICIENT_DIVERSITY) { //pick the opponent with the least games
        for (int i = 0; i < numPersonalities; i++) {
            if (InUse(i) && AllPersonalities[i].games < score ) {
                score = AllPersonalities[i].games;
                opponent = i;
            }
        }
    }
    else if (rand()%2==0 && numPersonalities > SUFFICIENT_DIVERSITY) {
        do {
            opponent = rand()%SUFFICIENT_DIVERSITY;
        } while (InUse(opponent));
    }
    else if (rand()%2==0 && numPersonalities > SUFFICIENT_DIVERSITY*2) {
        do {
            opponent = rand()%SUFFICIENT_DIVERSITY*2;
        } while (InUse(opponent));
    }
    else do {
        opponent = rand()%numPersonalities;
    } while (InUse(opponent));
    return opponent;
}
void NewTuneGame(const int player1) {
    const int player2 = player1+1;
    int position = rand()%NUM_START_POS;
    MutexLock(SMPLock);
    //OK lets see if we have a large enough pool of players
    if (numPersonalities < SUFFICIENT_DIVERSITY) {
        do {
            personality(player1).Randomize();
            Print(3,"creating random(%d) %s\n",player1,personality(player1).PrettyString());
        } while (!AddPersonality(player1));
        do {
            personality(player2).Randomize();
            Print(3,"creating random(%d) %s\n",player2,personality(player2).PrettyString());
        } while (!AddPersonality(player2));
    }
    else if (personality(player1).games < MIN_GAMES) { //if we have not had enough games, lets pick some random opponent and duke it out
        int opponent = PickOpponent();
    }
    else { //pick a player to test heavily weighting the top players
        int newPlayer;
        if (personality(player1).games%10!=0) { // only change after you have played 10 games
            bool change = false;
            if (rand()%3==0) { //pick the best prospect given similar ratings
                int newPlayer = GetBestSimilar();
                if (!InUse(player1)) {
                    change = true;
                    personality(player1).Copy(AllPersonalities[newPlayer]);
                }
                if (!change) { //OK, lets get something similar to this then, at least in one dimension
                    int bestP=-1;
                    float bestSc = -10000;
                    for (newPlayer = 0; newPlayer < numPersonalities; newPlayer++) {
                        float sim = 0;
                        for (int k = 0; k < NUM_GENOMES; k++) {
                            sim += AllPersonalities[newPlayer].similarity(AllPersonalities[newPlayer],k);
                        }
                        if (sim > bestSc && !InUse(newPlayer)) {
                            bestSc = sim;
                            bestP = newPlayer;
                        }
                    }
                    if (bestP >= 0) {
                        newPlayer = bestP;
                        personality(player1).Copy(AllPersonalities[newPlayer]);
                        change = true;
                    }
                }
            }
            if (!change) {
                for (newPlayer=0; newPlayer < numPersonalities; newPlayer++) {
                    if (rand()%(newPlayer==0?2:3) && !InUse(newPlayer)) break;
                }
                if (newPlayer==numPersonalities) {
                    do {
                        newPlayer = rand()%numPersonalities; 
                    } while (InUse(newPlayer));
                }
                personality(player1).Copy(AllPersonalities[newPlayer]);
            }
            change = false;
            if (ToChange(personality(player1).games,personality(player1).numToMutate) && personality(player1).numToMutate > 1) { //lets frankenstein this stucker
                for (int i = 0; i < NUM_GENOMES; i++) {
                    if (personality(player1).toMutate[i]) {
                        int bestP = GetBestGenome(i);
                        personality(player1).CopyGenome(AllPersonalities[bestP],i);
                        change = true;
                    }
                }
            }
            //shall i combine?
            if (change == false && ToChange(personality(player1).games,1)) {
                if (newPlayer==0) personality(player1).Combine(AllPersonalities[1]);
                else personality(player1).Combine(AllPersonalities[0]);
                change = true;
            }
            //now decide whether to mutate or not
            if (change == false && ToChange(personality(player1).games,1)) {
                personality(player1).Mutate();
                change = true;
            }

            if (change) AddPersonality(player1);
        }
        //now to find an opponent
        int opponent = PickOpponent();
        personality(player2).Copy(AllPersonalities[opponent]);
    }
    Print(3,"starting series between %s and ",personality(player1).PrettyString());
    Print(3,"%s\n",personality(player2).PrettyString());
    MutexUnlock(SMPLock);

    //now play a rated game
    PlayRatedGame(position,player1, player2);
    PlayRatedGame(position,player2, player1);

    MutexLock(SMPLock);
    //OK, find and copy back results

    for (int i=0; i < numPersonalities; i++) {
        if (AllPersonalities[i].Equal(personality(player1))) {
            personality(player1).games = AllPersonalities[i].games;
            personality(player1).rating =AllPersonalities[i].rating;
            personality(player1).points = AllPersonalities[i].points;
            break;
        }
    }
    for (int i=0; i < numPersonalities; i++) {
        if (AllPersonalities[i].Equal(personality(player2))) {
            personality(player2).games = AllPersonalities[i].games;
            personality(player2).rating =AllPersonalities[i].rating;
            personality(player2).points = AllPersonalities[i].points;
            break;
        }
    }

    ShowActive();
    MutexUnlock(SMPLock);
}

#endif
#ifdef SELF_TUNE


// consider adjusting score by your neighbors
void FilePrintHeader(int gameOn, int white, int black,int result) {
    FILE *fp;

    fp=fopen(SELF_TUNE_F, "a+");
    if (fp==NULL) {
        Print(3,"trouble opening training\n");
        return;
    }
    fprintf (fp," [Event \"Hannibal Training\"]\n");
    fprintf (fp," [White \"%s\"]\n",PlayerString(white));
    fprintf (fp," [Black \"%s\"]\n",PlayerString(black));
    fprintf (fp," [Round \"%d\"]\n",gameOn+1);
    if (result==1) {
        fprintf (fp," [Result \"1-0\"]\n");
        // at least one move so things do not crash
        fprintf(fp, "1. d2d4 {fakemove} \n");
        fprintf (fp," 1-0\n");
    }
    else if (result==2) {
        fprintf (fp," [Result \"0-1\"]\n");
        // at least one move so things do not crash
        fprintf(fp, "1. d2d4 {fakemove} \n");
        fprintf (fp," 0-1\n");
    }
    else {
        fprintf (fp," [Result \"1/2-1/2\"]\n");
        // at least one move so things do not crash
        fprintf(fp, "1. d2d4 {fakemove} \n");
        fprintf (fp," \"1/2-1/2\"\n");
    }
    fclose(fp);
}


void ShowBest(int num) {
    int i;

    PrintPlayerDiff(0);
    for(i=0; i < num && i < numPlayers;i++) {
        if (isChampion(i)) Print(3,"*");
        Print(3,"%d %.1f(%.2f(",i+1,GetScore(i),GetWinP(i));
        Print(3,"%d",playerList[i].points/2);
        if (playerList[i].points%2) Print(3,".5");
        Print(3,"/%d)): %s\n",playerList[i].games/2,PlayerString(i));
    }
}
int GetChallenger() {
    int i;
    if (numPlayers < RANDOM_SEEDS) {
        CreateRandomPlayer();
        return numPlayers-1;
    }
    {
        // if a top player has not played enough games, lets get them some
        for (i=1;i<10 && i<numChampions;i++) {
            if (playerList[0].games < (MATCH_SIZE)*6 || playerList[i].games < (MATCH_SIZE)*6) return i;
        }
    }
    // pick challenger
    {
        if (rand()%10==0 && numChampions > 0) {
            i = GetChampion();
            if (i > 0) {
                Print(3,"SELECTING CHAMPION %d(%d)\n",i+1,playerList[i].games);
                return i;
            }
        }
        for (i=1; i<numPlayers-1 && rand()%4;i++);
        {
            int q;
            if (i > 10) q = MUT*2;
            else q = MUT;
            if (rand()%q==0) return Mutate(i);
        }
        ASSERT (i < numPlayers);
        return i;
    }
}


void SetStartingPosition(position_t *pos,int position) {
    setPosition(pos, FenStartString[position]);
}
int PlayGame(position_t *pos, int player1, int player2, int startPos) {
    pos_store_t undo;
    int zeros = 0;
    int wScore;
    int bScore;
    int value;
    int moves = 0;
    int nodeTweak = rand()%(TUNE_NODES/10+1 - TUNE_NODES/20);

    SetStartingPosition(pos,startPos);
    InitPlayer(player1,0);
    InitPlayer(player2,1);
    Print(3,"%d:%s(%.1f)\t vs. ",player1+1,PlayerString(player1),GetScore(player1));
    Print(3,"%d:%s(%.1f)\t",player2+1,PlayerString(player2),GetScore(player2));
    while (true) {
        if (pos->side==WHITE) {
            SetPlayer(player1,0);
        }
        else {
            SetPlayer(player2,1);
        }
        origScore = 0;
        transClear();
#ifdef EVAL_TABLE
        evalTableClear();
#endif
        if (USE_PHASH) pawnTableClear();
        {
            uint64 pieces = pos->occupied & ~(pos->pawns | pos->kings);
            char s[255];

            if (NODE_BASED) {
                sprintf(s,"nodes %d",TUNE_NODES+nodeTweak);
            }
            else {
                int numPieces = bitCnt(pieces);
                if (numPieces==0)
                    sprintf(s,"depth %d",TUNE_PDEPTH);
                else if (numPieces <= 4)
                    sprintf(s,"depth %d",TUNE_EDEPTH);
                else sprintf(s,"depth %d",TUNE_DEPTH);
            }
            uciGo(pos,s);
        }
        if (SearchInfo.last_value != -10000) value = SearchInfo.last_value;
        else value = SearchInfo.last_last_value;
        if (pos->side == WHITE && value != -10000) wScore = value;
        if (pos->side == BLACK && value != -10000) bScore = value;
        if (wScore > 400 && bScore < -400) {
            return 1;
        }
        if (bScore > 400 && wScore < -400) {
            return 2;
        }
        else if (moves > 500) return 0;
        else if (SearchInfo.last_value ==0) {

            zeros++;
            if (zeros>5) {
                return 0;
            }
        }
        else if (value != -10000) zeros = 0;
        if (!SearchInfo.bestmove) {
            if (kingIsInCheck(pos)) {
                if (pos->side == WHITE) return 2;
                else return 1;
            }
            return 0; //stalemate
        } else {
            origScore = value; // just to be safe
        }
        makeMove(pos, &undo, SearchInfo.bestmove);
        if (pos->posStore.fifty==0) {
            pos->stack[0] = pos->hash;
            pos->sp = 0;
        }
        //		if (pos->posStore.fifty >= 100) return 0;

        if (anyRep(pos)) {
            return 0;
        }
        moves++;
    }
    Print(3,"info string why here??\n");
    return -1;
}

void Rate(int player, double expResult, double result, int opp) {
    double k;
    int g = playerList[player].games;
    int oppG = playerList[opp].games;


    if (g < 50) {
        k = 25;
    }
    else if (g < 100) {
        k = 20;
    }
    else if (g < 150) {
        k = 17.5;
    }
    else if (g < 200) {
        k = 15;
    }
    else if (g < 300) {
        k = 12.5;
    }
    else if (g < 400) {
        k = 10;
    }
    else if (g < 600) {
        k = 9;
    }
    else if (g < 800) {
        k =  8;
    }
    else if (g < 1000) {
        k = 7;
    }
    else if (g < 1500) {
        k = 6;
    }
    else if (g < 2000) {
        k = 6.5;
    }
    else if (g < 2500) {
        k = 5;
    }
    else if (g < 3000) {
        k = 4.5;
    }
    else if (g < 4000) {
        k = 4;
    }
    else if (g < 5000) {
        k = 3.5;
    }
    else if (g < 7500) {
        k = 3;
    }
    else if (g < 10000) {
        k = 2.5;
    }
    else if (g < 15000) {
        k = 2;
    }
    else if (g < 20000) {
        k = 1.5;
    }
    else k = 1;
    if (oppG < 20 && g>400) k *= 0.2;
    else if (opp < 40 && g > 300) k *= 0.3;
    else if (oppG < 60 && g > 200) k *= 0.4;
    else if (oppG < 100 && g > 50) k *= 0.5;
    else if (oppG < 200) k *= 0.75;
    else if (oppG > 2000) k *= 1.5;
    else if (oppG > 1000) k *= 1.25;
    {
        double change = (k) * (result - expResult);
        playerList[player].rating += change;
        //		Print(3,"\n%d(%f) changed %f\n",player+1,expResult,change);
    }
}
void RateDraw(int p1, int p2) {
    double winP = ProbWin(playerList[p1].rating,playerList[p2].rating);
    Rate(p1,winP,0.5,p2);
    Rate(p2,1.0-winP,0.5,p1);
    playerList[p1].games +=2;
    playerList[p2].games +=2;
    playerList[p1].points +=1;
    playerList[p2].points +=1;
}
void RateWin(int winner, int loser) {
    double winP = ProbWin(playerList[winner].rating,playerList[loser].rating);
    //	Print(3,"\n%d has a %f chance to beat %d\n",winner,winP,loser);
    Rate(winner,winP,1.0,loser);
    Rate(loser,1.0-winP,0.0,winner);
    playerList[winner].games +=2;
    playerList[loser].games +=2;
    playerList[winner].points +=2;
}
void  PlayRatedGame(position_t *pos, int player1, int player2,int gameOn,int position) {
    int result = PlayGame(pos,player1,player2,position);
    if (result==0) {
        RateDraw(player1,player2);
        Print(3," DRAW\n");
    }
    else if (result==1) {
        RateWin(player1,player2);
        Print(3,"%d (WHITE) wins\n",player1+1);
    }
    else if (result == 2) {
        RateWin(player2,player1);
        Print(3," %d (BLACK) wins\n",player2+1);
    }
    else Print(3,"odd result %d \n",result);
    FilePrintHeader(gameOn,player1,player2,result);
}
int FirstMatch(int player,int gameOn,position_t *pos) {
    int game=1;
    int matchOn=1;
    double startingRating;
    do  {
        int player2;
        int position = rand()%NUM_START_POS;

        do {
            if (game==1 && player!=0) player2 = 0;
            else player2 = rand()%numPlayers;
        } while (player2 == player);
        if (game==1&&matchOn!=2) startingRating = playerList[player].rating;
        PlayRatedGame(pos,player,player2,gameOn,position);
        game++;
        PlayRatedGame(pos,player2,player,gameOn,position);
        game++;
        if (game > FIRST_MATCH_SIZE) {
            game = 1;
            matchOn++;
            Print(3,"%d--------------- %f -> %f-----------------\n",player+1,startingRating,playerList[player].rating);
        }
    } while (matchOn<=2 || game!=1 || startingRating < playerList[player].rating);

    return gameOn+matchOn*MATCH_SIZE;
}
void SortAndTest(position_t *pos) {
    player_t oldBest;
    int opponent;


    CopyPlayer(&oldBest,&playerList[0]);
    SortPlayers();
    while (!SamePlayer(&oldBest,&playerList[0])) {
        int matches = 0;
        CopyPlayer(&oldBest,&playerList[0]);

        ShowBest(10);
        for (opponent=1;opponent<numPlayers;opponent++) {
            int div = 1;
            int ps = numPlayers-opponent;
            while ((matches+ps/(div+1))>100 && div < 10) div++;
            if (playerList[0].games < VETERAN || (rand()%div==0)) {
                int position = rand()%NUM_START_POS;
                PlayRatedGame(pos,0,opponent,0,position);
                sinceNewPlayer++;
                PlayRatedGame(pos,opponent,0,0,position);
                sinceNewPlayer++;
                matches++;
                if (playerList[0].games%10==0 || opponent==numPlayers-1) {
                    SortPlayers();
                    if (!SamePlayer(&oldBest,&playerList[0])) {
                        break;
                    }
                }
            }
        }
        if (sinceNewPlayer > MAX_BEFORE_NEW) break;
    }
    ShowBest(10);
}
void ChampionWar(position_t *pos) {
    int player1, player2, position;
    double start1, start2;
    for (player1=0;player1 < numPlayers-1;player1++) {
        for (player2=player1+1;player2 < numPlayers;player2++) {
            start1 = GetScore(player1);
            start2 = GetScore(player2);
            for (position=0;position < NUM_START_POS;position++) {
                PlayRatedGame(pos,player1,player2,0,position);
                PlayRatedGame(pos,player2,player1,0,position);
                if (position%10==9 || position==NUM_START_POS-1) {
                    double newScore1 = GetScore(player1);
                    double newScore2 = GetScore(player2);
                    Print(3,"%d-- %d: %f -> %f -------------- %d: %f -> %f ----------\n",position+1,player1+1,start1,newScore1,player2+1,start2,newScore2);
                    ShowBest(numPlayers);
                }
            }
        }
    }
}
void selfTune(position_t *pos) {
    // initialize
    int gameOn = 1;
    int player1,player2,firstMatch;
    srand((unsigned int) getTime());
    InitPlayers();
    ShowBest(numPlayers);
    // a little round robin fun for seeds
    if (numPlayers > 1) {
        if (CHAMPION_WAR) {
            ChampionWar(pos);
        }
        else {
            int matches = 0;
            for (player1=0;player1 < numPlayers-1;player1++) {
                for (player2=player1+1;player2 < numPlayers;player2++) {
                    if (isChampion(player2) || isChampion(player1) || Random(1,4+matches/100) <= 4) {
                        int position = rand()%NUM_START_POS;
                        PlayRatedGame(pos,player1,player2,gameOn,position);
                        gameOn++;
                        PlayRatedGame(pos,player2,player1,gameOn,position);
                        gameOn++;
                        matches++;
                    }
                }
            }
        }
        SortPlayers();
        ShowBest(numPlayers);

    }
    while (true) {
        int matchOn = 1;
        int matchSize;
        double startScore1,startScore2;
        // king of the hill is the one with the best winning %
        player1 = 0;
        // make room for mutations
        while (numPlayers > MAX_PLAYERS-1) {// destroy player with worse record (even if it is original)
            numPlayers--;
            Print(3,"removing %s(%.3f(%d/%d))\n",PlayerString(numPlayers),GetScore(numPlayers),playerList[numPlayers].points,playerList[numPlayers].games);
        }
        player2 = GetChallenger();
        if ((sinceNewPlayer > MAX_BEFORE_NEW || rand()%(MUT_1)==0) && numPlayers > RANDOM_SEEDS && player2 < numPlayers-1) player1 = Mutate(player1);
        if (player1 == player2) player1 = 0;

        if (playerList[player1].games < MATCH_SIZE*4) {

            firstMatch = player1;
        }
        else if (playerList[player2].games < MATCH_SIZE*4) {
            firstMatch = player2;
        }
        else {
            firstMatch = -1;
        }
        matchSize = MATCH_SIZE;
        if (numPlayers < 3 || matchSize < 4) firstMatch = -1;
        if (firstMatch>=0) {
            gameOn = FirstMatch(firstMatch,gameOn,pos);
            sinceNewPlayer = 0;
        }
        else {
            // if a champion or out of favor mutation is out of favor, give it some extra games to prove itself
            if (player2 >= 10 || player1 >= 10) {
                if (player1 >= 20 || player2 >= 20) {
                    matchSize *= 2;
                }
                else {
                    matchSize += matchSize / 2 + matchSize%2;
                }
            }
            startScore1 = playerList[player1].rating;
            startScore2 = playerList[player2].rating;
            while (matchOn <= matchSize) {
                int position = rand()%NUM_START_POS;

                PlayRatedGame(pos,player1,player2,gameOn,position);
                gameOn++;
                sinceNewPlayer++;
                matchOn++;
                PlayRatedGame(pos,player2,player1,gameOn,position);
                gameOn++;
                sinceNewPlayer++;
                matchOn++;
            }

            Print(3,"%d--- %f -> %f---------%d--- %f -> %f---------\n",player1+1,startScore1,playerList[player1].rating,
                player2+1,startScore2,playerList[player2].rating);
        }
        //		SortPlayers();
        //		ShowBest(10);

        SortAndTest(pos);
    }

}
#endif

#ifdef SELF_TUNE2
#define MIN_SIM 0.9
#define SIM_WEIGHT 0.2 
#define TUNE_NODES 50000
#define MAX_PERSONALITIES 10000
#define MAX_SHOW 20
#define DEFAULT_RATING 2000
#define SUFFICIENT_DIVERSITY 50 //should be at > twice the number of active games
#define MIN_GAMES 100
int Random (int min, int max) {
    if (min >=max) return min;
    return rand()%(max-min+1)+min;
}
typedef struct {
    int low;
    int high;
} range_t;
class Personality {
private:
    double similarityWeight[NUM_GENOMES];
    double ratingWeight[NUM_GENOMES];
public:
    int numToMutate; //TODO make numToMatate and the ranges static
    int stuck_end; range_t stuck_end_range;
    int stuck_bishop; range_t stuck_bishop_range;
    int stuck_rook; range_t stuck_rook_range;
    int stuck_queen; range_t stuck_queen_range;
    int tempo_open; range_t tempo_open_range;
    int tempo_end; range_t tempo_end_range;
    int TrappedMoves[7*4+1]; range_t trapped1_range; range_t trapped2_range; range_t trapped3_range; range_t trapped4_range;


    int games, points;
    double rating;

    bool toMutate[NUM_GENOMES];

    int GetTrapped1() { return TrappedMoves[0]-TrappedMoves[1];}
    int GetTrapped2() { return TrappedMoves[1]-TrappedMoves[2];}
    int GetTrapped3() { return TrappedMoves[2]-TrappedMoves[3];}
    int GetTrapped4() { return TrappedMoves[3];}
    void SetDefaultRating() {
        games = 0;
        points = 0;
        rating = DEFAULT_RATING;
        for (int i = 0; i < NUM_GENOMES; i++) {
            similarityWeight[i] = 0;
            ratingWeight[i] = 0;
        }
    }
    void SetDefaults() {
        SetDefaultRating();
        numToMutate = NUM_GENOMES; //stuck, trapped

        stuck_end = STUCK_END; stuck_end_range.low = 0; stuck_end_range.high = 10;
        stuck_bishop = STUCK_BISHOP; stuck_bishop_range.low = 0; stuck_bishop_range.high = 50;
        stuck_rook = STUCK_ROOK; stuck_rook_range.low = 0; stuck_rook_range.high = 50;
        stuck_queen = STUCK_QUEEN; stuck_queen_range.low = 0; stuck_queen_range.high = 50;

        tempo_open = TEMPO_OPEN; tempo_open_range.low = 0; tempo_open_range.high = 40;
        tempo_end = TEMPO_END; tempo_end_range.low = 0; tempo_end_range.high = 40;

        SetTrapped(TRAPPED1,TRAPPED2,TRAPPED3,TRAPPED4);
        trapped1_range.low = 0; trapped1_range.high = 200;
        trapped2_range.low = 0; trapped2_range.high = 200;
        trapped3_range.low = 0; trapped3_range.high = 50;
        trapped4_range.low = 0; trapped4_range.high = 20;

        SetMutate(true,true,true); //mutate stuck but not trapped
    }
    double GetSimilarityRating(int i) {
        double sr = 0;
        if (toMutate[i]) sr += (similarityWeight[i] == 0) ? 0 : (ratingWeight[i] / similarityWeight[i]);

        if (sr==0) return 0;
        return sr;
    }
    double GetSimilarityRating() {
        double sr = 0;
        int numCount = 0;
        for (int i = 0; i < NUM_GENOMES; i++) {
            if (toMutate[i])
            {
                if (similarityWeight[i] > 0) {
                    sr += (ratingWeight[i] / similarityWeight[i]);
                    numCount++;
                }
            }
        }
        if (numCount==0) return 0;
        return (sr / (float) numCount);
    }

    char* PrettyString() {
        static char s[255], s1[255]="", s2[255]="", s3[255]="";
        if (tempo_open_range.low != tempo_open_range.high) {
            double sr = GetSimilarityRating(0);
            sprintf(s1,"%d %d [%.2f]",tempo_open,tempo_end,sr);
        }
        if (stuck_end_range.low != stuck_end_range.high) {
            double sr = GetSimilarityRating(1);
            sprintf(s2,"%d %d %d %d [%.2f]",stuck_end,stuck_bishop,stuck_rook,stuck_queen,sr);
        }
        if (trapped1_range.low != trapped1_range.high) {
            double sr = GetSimilarityRating(2);
            sprintf(s3,"%d %d %d %d [%.2f]",GetTrapped1(),  GetTrapped2(),  GetTrapped3(),  GetTrapped4(),sr);
        }
        sprintf(s,"%s%s%s",s1,s2,s3);

        return s;
    }
    void updateRatingWeight(double sim, Personality p,int k, bool increment) {
        double games = (double)p.games/(double)MIN_GAMES;
        double weight = sim * games;
        if (increment) {
            similarityWeight[k] += weight;
            ratingWeight[k] += weight * p.rating;
        }
        else {
            similarityWeight[k] -= weight;
            ratingWeight[k] -=  weight * p.rating;
        }
    }
    int rangeDif(range_t r) { return (r.high - r.low);}
    double gameWeight() {
        if (games < MIN_GAMES) return 0;
        else if  (games < MIN_GAMES*2) return 1;
        else if  (games < MIN_GAMES*4) return 2;
        else if  (games < MIN_GAMES*8) return 3;
        else if  (games < MIN_GAMES*16) return 4;
        else if  (games < MIN_GAMES*32) return 5;
        else if  (games < MIN_GAMES*64) return 6;
        return 7;
    }
    double similarity(Personality p, int k) {
        if (Equal(p)) return (1.0); //if you have no games, you don't count as similar to yourself :)
        double dif = 0;
        double s;
        if (k==0) { //tempo
            float range = (float) rangeDif(tempo_open_range);
            if (range==0) return 0;
            dif += fabs((float)tempo_open - (float)p.tempo_open) / range;
            range = (float) rangeDif(tempo_end_range);
            dif += fabs((float)tempo_end - (float)p.tempo_end) / range;
            s = 1.0 - (dif / 2.0);			
        }
        else if (k==1) { //stuck
            float range = (float) rangeDif(stuck_end_range);
            if (range==0) return 0;
            dif += fabs((float)stuck_end - (float)p.stuck_end) / range;
            range = (float) rangeDif(stuck_bishop_range);
            dif += fabs((float)stuck_bishop - (float)p.stuck_bishop) / range;
            range = (float) rangeDif(stuck_rook_range);
            dif += fabs((float)stuck_rook - (float)p.stuck_rook) / range;
            range = (float) rangeDif(stuck_queen_range);
            dif += fabs((float)stuck_queen - (float)p.stuck_queen) / range;
            s = 1.0 - (dif / 4.0);			
        }
        else if (k==2) { //trapped
            float range = (float) rangeDif(trapped1_range);
            if (range==0) return 0;
            dif += abs(GetTrapped1() - p.GetTrapped1()) / range;
            range = (float) rangeDif(trapped2_range);
            dif += fabs((float)GetTrapped2() - (float)p.GetTrapped2()) / range;
            range = (float) rangeDif(trapped3_range);
            dif += fabs((float)GetTrapped3() - (float)p.GetTrapped3()) / range;
            range = (float) rangeDif(trapped4_range);
            dif += fabs((float)GetTrapped4() - (float)p.GetTrapped4()) / range;
            s = 1.0 - (dif / 4.0);			
        }
        if (s < MIN_SIM) return 0;
        else return (s * SIM_WEIGHT);
    }

    /*
    char* FileString() {
    static char s[255];
    sprintf(s,"%.2f %d %d %d %d %d %d %d %d %d %d",rating, points, games, stuck_end,stuck_bishop,stuck_rook,stuck_queen, GetTrapped1(),  GetTrapped2(),  GetTrapped3(),  GetTrapped4());
    return s;
    }*/
    bool Equal(Personality p) {
        return (stuck_end == p.stuck_end && stuck_bishop == p.stuck_bishop && stuck_rook == p.stuck_rook &&
            stuck_queen == p.stuck_queen && GetTrapped1() == p.GetTrapped1() && GetTrapped2() == p.GetTrapped2() &&
            GetTrapped3() == p.GetTrapped3() && GetTrapped4() == p.GetTrapped4() &&
            tempo_open == p.tempo_open && tempo_end == p.tempo_end);
    }
    void SetTrapped(int trapped1, int trapped2, int trapped3, int trapped4) {
        TrappedMoves[0] = trapped4+trapped3+trapped2+trapped1;
        TrappedMoves[1] = trapped4+trapped3+trapped2;
        TrappedMoves[2] = trapped4+trapped3;
        TrappedMoves[3] = trapped4;
        for (int i = 4; i < 7*4+1;i++) TrappedMoves[i] = 0;
    }
    void SetMutate(bool tempoMutate, bool stuckMutate, bool trappedMutate) {
        numToMutate = 0;
        toMutate[0] = tempoMutate;
        toMutate[1] = stuckMutate;
        toMutate[2] = trappedMutate;
        if (stuckMutate == false) {
            stuck_end_range.low = stuck_end_range.high = stuck_end;
            stuck_bishop_range.low = stuck_bishop_range.high = stuck_bishop;
            stuck_rook_range.low = stuck_rook_range.high = stuck_rook; 
            stuck_queen_range.low = stuck_queen_range.high = stuck_queen;
        }
        else numToMutate++;
        if (trappedMutate == false) {
            trapped1_range.low = trapped1_range.high = GetTrapped1();
            trapped2_range.low = trapped2_range.high = GetTrapped2();
            trapped3_range.low = trapped3_range.high = GetTrapped3();
            trapped4_range.low = trapped4_range.high = GetTrapped4();
        }
        else numToMutate++;
        if (tempoMutate == false) {
            tempo_open_range.low = tempo_open_range.high = tempo_open;
            tempo_end_range.low = tempo_end_range.high = tempo_end;
        }
        else numToMutate++;
    }
    void Randomize() {
        SetDefaultRating();
        stuck_end = Random(stuck_end_range.low,stuck_end_range.high);
        stuck_bishop = Random(stuck_bishop_range.low,stuck_bishop_range.high);
        stuck_rook = Random(stuck_rook_range.low,stuck_rook_range.high);
        stuck_queen = Random(stuck_queen_range.low,stuck_queen_range.high);
        int trapped1 = Random(trapped1_range.low,trapped1_range.high);
        int trapped2 = Random(trapped2_range.low,trapped2_range.high);
        int trapped3 = Random(trapped3_range.low,trapped3_range.high);
        int trapped4 = Random(trapped4_range.low,trapped4_range.high);
        SetTrapped(trapped1, trapped2, trapped3, trapped4);

        tempo_open = Random(tempo_open_range.low, tempo_open_range.high);
        tempo_end = Random(tempo_end_range.low, tempo_end_range.high);
    }
    int change(int dif) {
        if (dif==0) return 0;
        else return 1 + Random(0,(dif-1)/8);
    }
    bool MutateVariable(int *v, range_t r) { //returns whether succesfully mutated
        int startValue = *v;
        if (*v < r.low || *v > r.high || r.low == r.high) return false;
        if (Random(0,1) == 0 && *v > r.low) {// make the variable lower
            int dif = *v - r.low;
            *v -= change(dif);
        }
        else { // make the variable higher
            int dif = r.high - *v;
            *v += change(dif);
        }
        return (*v != startValue);
    }
    bool CanMutate(range_t r) {
        return (r.low < r.high);
    }
    void Mutate() {
        if (numToMutate < 1) return;
        bool changed = false;
        do {
            int mutate = Random(1,numToMutate);
            int option = Random(0,10000);
            if (CanMutate(tempo_open_range)) {
                mutate--;
                if (mutate==0) {
                    if (option%2==0) changed = MutateVariable(&tempo_open,tempo_open_range);
                    else changed = MutateVariable(&tempo_end,tempo_end_range);
                }
            }
            if (CanMutate(stuck_end_range)) {
                mutate--;
                if (mutate==0) {
                    if (option%4==0) changed = MutateVariable(&stuck_end,stuck_end_range);
                    else if (option%4==1) changed = MutateVariable(&stuck_bishop,stuck_bishop_range);
                    else if (option%4==2) changed = MutateVariable(&stuck_rook,stuck_rook_range);
                    else changed = MutateVariable(&stuck_queen,stuck_queen_range);
                }
            }
            if (CanMutate(trapped1_range)) {
                mutate--;
                if (mutate==0) {
                    int trapped1 = GetTrapped1();
                    int trapped2 = GetTrapped2();
                    int trapped3 = GetTrapped3();
                    int trapped4 = GetTrapped4();

                    if (option%4==0) changed = MutateVariable(&trapped1,trapped1_range);
                    else if (option%4==1) changed = MutateVariable(&trapped2,trapped2_range);
                    else if (option%4==2) changed = MutateVariable(&trapped3,trapped3_range);
                    else changed = MutateVariable(&trapped4,trapped4_range);
                    if (changed) SetTrapped(trapped1, trapped2, trapped3, trapped4);
                }
            }
        } while (!changed);
        games = 0; points = 0;
        SetDefaultRating();
    }

    void Combine(Personality p) {
        tempo_open =(tempo_open + p.tempo_open + rand()%2)/2;
        tempo_end =(tempo_end + p.tempo_end + rand()%2)/2;

        stuck_end = (stuck_end + p.stuck_end + rand()%2)/2;
        stuck_bishop = (stuck_bishop + p.stuck_bishop + rand()%2)/2;
        stuck_rook = (stuck_rook + p.stuck_rook + rand()%2)/2;
        stuck_queen = (stuck_queen + p.stuck_queen + rand()%2)/2;

        TrappedMoves[0] = (TrappedMoves[0] + p.TrappedMoves[0] + rand()%2)/2;
        TrappedMoves[1] = (TrappedMoves[1] + p.TrappedMoves[1] + rand()%2)/2;
        TrappedMoves[2] = (TrappedMoves[2] + p.TrappedMoves[2] + rand()%2)/2;
        TrappedMoves[3] = (TrappedMoves[3] + p.TrappedMoves[3] + rand()%2)/2;
        SetDefaultRating();
    }

    void CopyTempo(Personality p) {
        tempo_open = p.tempo_open;
        tempo_end = p.tempo_end;
    }
    void CopyStuck(Personality p) {
        stuck_end = p.stuck_end;
        stuck_bishop = p.stuck_bishop;
        stuck_rook = p.stuck_rook;
        stuck_queen = p.stuck_queen;
    }
    void CopyTrapped(Personality p) {
        TrappedMoves[0] = p.TrappedMoves[0];
        TrappedMoves[1] = p.TrappedMoves[1];
        TrappedMoves[2] = p.TrappedMoves[2];
        TrappedMoves[3] = p.TrappedMoves[3];
    }
    void CopyGenome(Personality p,int g) {
        if (g==0) CopyTempo(p);
        else if (g==1) CopyStuck(p);
        else if (g==2) CopyTrapped(p);
    }
    void Copy(Personality p) {

        games = p.games;
        rating = p.rating;
        for (int i = 0; i < NUM_GENOMES; i++) {
            CopyGenome(p,i);
            similarityWeight[i] = p.similarityWeight[i];
            ratingWeight[i] = p.ratingWeight[i];
        }
        points = p.points;
    }

    void Swap(Personality *p) {
        Personality temp;
        temp.Copy(*p);
        p->Copy(*this);
        Copy(temp);
    }
    /*
    Personality(char *str) {
    int trapped1,trapped2,trapped3,trapped4;
    SetDefaults();
    sscanf(str,"%d %d %d %d %d %d %d %d %d %d %d",&games, &rating, &points, &stuck_end,&stuck_bishop,&stuck_rook,&stuck_queen,&trapped1,&trapped2,&trapped3,&trapped4);
    }

    Personality(int stuckEnd, int stuckBishop, int stuckRook, int stuckQueen, int trapped1, int trapped2, int trapped3, int trapped4) {
    SetDefaults();
    stuck_end = stuckEnd;
    stuck_bishop = stuckBishop;
    stuck_rook = stuckRook;
    stuck_queen = stuckQueen;
    SetTrapped(trapped1, trapped2, trapped3, trapped4);
    }*/
    Personality() { 
        SetDefaults();
    }
};

Personality personality[MaxNumOfThreads];
#define personality(thread) personality[thread]
#endif