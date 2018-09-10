/**************************************************/
/*  Name: Hannibal						          */
/*  Copyright: 2009-2014                          */
/*  Author: Sam Hamilton, Edsel Apostol           */
/*  Contact: snhamilton@rocketmail.com            */
/*  Contact: ed_apostol@yahoo.com                 */
/*  Description: A chess playing program.         */
/**************************************************/

#include <chrono>
#include <windows.h>
#include "typedefs.h"
#include "data.h"
#include "constants.h"
#include "macros.h"
#include "attacks.h"
#include "position.h"
#include "utils.h"
#include "eval.h"

/* a utility to print output into different files:
bit 0: stdout
bit 1: logfile
bit 2: errfile
bit 3: dumpfile
*/

int fr_square(int f, int r) {
	return ((r << 3) | f);
}
void PrintBitBoard(uint64 n) {
	PrintOutput() << "info string ";
	while (n) {
		int sq = popFirstBit(&n);
		PrintOutput() << (char)(SQFILE(sq) + 'a') << (char)('1' + SQRANK(sq)) << ' ';
	}
	PrintOutput() << "\n";
}
/* a utility to convert large int to hex string*/
char *bit2Str(uint64 n) {
	static char s[19];
	int i;
	s[0] = '0';
	s[1] = 'x';
	for (i = 0; i < 16; i++) {
		if ((n & 15) < 10) s[17 - i] = '0' + (n & 15);
		else s[17 - i] = 'A' + (n & 15) - 10;
		n >>= 4;
	}
	s[18] = 0;
	return s;
}

/* a utility to convert int move to string */
std::string move2Str(basic_move_t m) {
	const std::string promstr = "0pnbrqk";
	std::string str;

	if (m == 0) str = "0000";
	else {
		str = (SQFILE(moveFrom(m)) + 'a');
		str += ('1' + SQRANK(moveFrom(m)));
		str += (SQFILE(moveTo(m)) + 'a');
		str += ('1' + SQRANK(moveTo(m)));
		if (movePromote(m) != EMPTY) str += promstr[movePromote(m)];
	}
	return str;
}

/* a utility to convert int square to string */
char *sq2Str(int sq) {
	static char str[3];

	/* ASSERT(moveIsOk(m)); */

	sprintf_s(str, "%c%c%c",
		SQFILE(sq) + 'a',
		'1' + SQRANK(sq),
		'\0'
	);
	return str;
}

/* a utility to get a certain piece from a position given a square */
int getPiece(const position_t& pos, uint32 sq) {
	return pos.pieces[sq];
}

/* a utility to get a certain color from a position given a square */
int getColor(const position_t& pos, uint32 sq) {
	uint64 mask = BitMask[sq];

	ASSERT(squareIsOk(sq));

	if (mask & pos.color[WHITE]) return WHITE;
	else if (mask & pos.color[BLACK]) return BLACK;
	else {
		ASSERT(mask & ~pos.occupied);
		return WHITE;
	}
}
int DiffColor(const position_t& pos, uint32 sq, int color) {
	ASSERT(squareIsOk(sq));

	return ((BitMask[sq] & pos.color[color]) == 0);
}
/* returns time in milli-seconds */
uint64 getTime(void) {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/* parse the move from string and returns a move from the
move list of generated moves if the move string matched
one of them */
uint32 parseMove(movelist_t& mvlist, std::string s) {
	for (auto& a : s) a = tolower(a);
	for (mvlist.pos = 0; mvlist.pos < mvlist.size; mvlist.pos++) {
		if (move2Str(mvlist.list[mvlist.pos].m) == s)
			return mvlist.list[mvlist.pos].m;
	}
	return 0;
}

bool anyRep(const position_t& pos) {//this is used for book repetition detection, but should not be used in search
	if (pos.posStore.fifty >= 100) return true;
	pos_store_t* psp;
	if (!pos.posStore.previous || !pos.posStore.previous->previous) return false;
	psp = pos.posStore.previous->previous;
	for (int plyForRep = 4, pliesToCheck = MIN(pos.posStore.pliesFromNull, pos.posStore.fifty); plyForRep <= pliesToCheck; plyForRep += 2) {
		if (!psp->previous || !psp->previous->previous) return false;
		psp = psp->previous->previous;
		if (psp->hash == pos.posStore.hash) return true;
	}
	return false;
}

bool anyRepNoMove(const position_t& pos, const int m) {
	if (moveCapture(m) || isCastle(m) || pos.posStore.fifty < 3 || pos.posStore.pliesFromNull < 3 || movePiece(m) == PAWN) return false;
	if (pos.posStore.fifty >= 99) return true;

	uint64 compareTo = pos.posStore.hash ^ ZobColor ^ ZobPiece[pos.side][movePiece(m)][moveFrom(m)] ^ ZobPiece[pos.side][movePiece(m)][moveTo(m)];
	int plyForRep = 3, pliesToCheck = MIN(pos.posStore.pliesFromNull, pos.posStore.fifty);

	for (pos_store_t* psp = pos.posStore.previous; psp && plyForRep <= pliesToCheck; plyForRep += 2) {
		if (!psp->previous) return false;
		else if ((psp = psp->previous->previous) && psp->hash == compareTo) return true;
	}
	return false;
}

typedef int(*fun1_t) (LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
typedef int(*fun2_t) (USHORT, PGROUP_AFFINITY);
typedef int(*fun3_t) (HANDLE, CONST GROUP_AFFINITY*, PGROUP_AFFINITY);

int bestGroup(int index) {
	int groupSize = 0, groups[2048];
	int nodes = 0, cores = 0, threads = 0;
	DWORD returnLength = 0, byteOffset = 0;

	HMODULE k32 = GetModuleHandle("Kernel32.dll");
	fun1_t fun1 = (fun1_t)GetProcAddress(k32, "GetLogicalProcessorInformationEx");
	if (!fun1) return -1;

	if (fun1(RelationAll, NULL, &returnLength)) return -1;

	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, *ptr;
	ptr = buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(returnLength);
	if (!fun1(RelationAll, buffer, &returnLength)) {
		free(buffer);
		return -1;
	}

	while (ptr->Size > 0 && byteOffset + ptr->Size <= returnLength) {
		if (ptr->Relationship == RelationNumaNode) nodes++;
		else if (ptr->Relationship == RelationProcessorCore) {
			cores++;
			threads += (ptr->Processor.Flags == LTP_PC_SMT) ? 2 : 1;
		}
		byteOffset += ptr->Size;
		ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(((char*)ptr) + ptr->Size);
	}
	free(buffer);

	for (int n = 0; n < nodes; n++)
		for (int i = 0; i < cores / nodes; i++)
			groups[groupSize++] = n;
	for (int t = 0; t < threads - cores; t++)
		groups[groupSize++] = t % nodes;
	return index < groupSize ? groups[index] : -1;
}

void bindThisThread(int index) {
	int group;
	if ((group = bestGroup(index)) == -1) return;

	HMODULE k32 = GetModuleHandle("Kernel32.dll");
	fun2_t fun2 = (fun2_t)GetProcAddress(k32, "GetNumaNodeProcessorMaskEx");
	fun3_t fun3 = (fun3_t)GetProcAddress(k32, "SetThreadGroupAffinity");
	if (!fun2 || !fun3) return;

	GROUP_AFFINITY affinity;
	if (fun2(group, &affinity)) fun3(GetCurrentThread(), &affinity, NULL);
}