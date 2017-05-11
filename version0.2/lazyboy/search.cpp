#include "search.h"
#include "base.h"
#include "hash.h"
#include "position.h"
#include "rollback.h"
#include "movesort.h"
#include "evaluate.h"
#include "debug.h"
#include "time.h"

PositionStruct pos; // 当前搜索局面
RollBackListStruct roll; // 回滚着法表
SearchStruct Search; // 搜索结构体

// 无害裁剪
int HarmlessPruning ( void ) {
	// 1. 和局局面
	if ( pos.IsDraw() ) {
		return 0; // eleeye上表示，为了安全起见，不用pos.DrawValue()
	}

	// 2. 路径重复
	int vRep = roll.RepStatus ();
	if ( vRep != REP_NONE ) {
		return roll.RepValue ( vRep );
	}

	return - MATE_VALUE;
}

// Alpha-Beta 搜索
int AlphaBetaSearch ( int depth, int alpha, int beta ) {
	int val;
	int bestval = - MATE_VALUE;
	int bestmv = 0;
	int hash_type = HASH_TYPE_ALPHA;
	MoveSortStruct mvsort;

	if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
		return bestval;
	}

	Search.nNode ++;

	// 1. 无害裁剪
	val = HarmlessPruning ();
	if ( val > - MATE_VALUE ) {
		return val;
	}

	// 2. 打分
	if ( depth <= 0 ) {
		return pos.Evaluate ( alpha, beta );
	}

	// 3. 置换裁剪
	val = QueryValueInHashTable ( depth, alpha, beta );
	if ( val != - MATE_VALUE ) {
		if ( pos.nDistance == 0 ) {
			Search.bmv = bestmv;
		}
		return val;
	}

	// 4. 生成着法
	mvsort.InitAlphaBetaMove ();

	// 5. 递归搜索
	int mv;
	while ( (mv = mvsort.NextMove()) != 0 ) {
		pos.MakeMove ( mv );
		int val = - AlphaBetaSearch ( depth - 1, -beta, -alpha );
		pos.UndoMakeMove ();

		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			return bestval;
		}

		if ( val > bestval ) {
			bestval = val;
			bestmv = mv;
			if ( bestval >= beta ) {
				Search.nBeta ++;
				InsertKillerTable ( bestmv );
				hash_type = HASH_TYPE_BETA;
				break;
			}
			if ( bestval > alpha ) {
				alpha = bestval;
				hash_type = HASH_TYPE_PV;
			}
		}
	}

	// 6. 最后
	if ( pos.nDistance == 0 && bestmv != 0 ) {
		Search.bmv = bestmv;
	}
	InsertHashTable ( depth, bestval, bestmv, hash_type );
	InsertHistoryTable ( bestmv, depth );
	return bestval;
}

// 主搜索函数
int MainSearch ( void ) {
	// 1. 特殊情况
	MoveSortStruct mvsort;
	int nMoveNum = mvsort.InitAlphaBetaMove ();
	if ( nMoveNum == 0 ) { // 无着法
		printf ( "bestmove a0a1 resign\n" );
		fflush ( stdout );
		return 0;
	}
	else if ( nMoveNum == 1 ) { // 唯一着法
		printf ( "bestmove %s\n", MoveIntToStr(mvsort.move[0]).c_str() );
		fflush ( stdout );
		return mvsort.move[0];
	}
	else if ( pos.IsDraw() ) { // 和局
		printf ( "bestmove a0a1 draw\n" );
		fflush ( stdout );
		// 注意不要return
	}

	// 2. 大搜索
	// 初始化时间器
	InitBeginTime ( SEARCH_TOTAL_TIME );
	SetTimeLimit (10);
	// 迭代加深
	printf("depth   time    nNode  rBeta   value  bestmv\n");
	fflush ( stdout );
	int last_bvl = - MATE_VALUE, last_bmv = 0;
	for ( int depth = 1; /*depth <= ?*/; depth ++ ) {
		InitBeginTime ( THIS_SEARCH_TIME );
		Search.bmv = 0;
		Search.nNode = Search.nBeta = 0;
		Search.bvl = AlphaBetaSearch ( depth, - MATE_VALUE, MATE_VALUE );

		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			Search.bvl = last_bvl;
			Search.bmv = last_bmv;
			break;
		}
		else {
			last_bvl = Search.bvl;
			last_bmv = Search.bmv;
		}

		// 重要信息输出
		double tc = TimeCost(THIS_SEARCH_TIME);
		if ( tc < 10.0 ) {
			printf( "%5d  %.2fs  %7d    %2.0f%%  %6d    %s\n",
								depth, TimeCost(THIS_SEARCH_TIME), Search.nNode,
								100.0*Search.nBeta/Search.nNode, Search.bvl, MoveIntToStr(Search.bmv).c_str() );
		}
		else {
			printf( "%5d  %.1fs  %7d    %2.0f%%  %6d    %s\n",
								depth, TimeCost(THIS_SEARCH_TIME), Search.nNode,
								100.0*Search.nBeta/Search.nNode, Search.bvl, MoveIntToStr(Search.bmv).c_str() );
		}
		fflush ( stdout );

		if ( Search.bvl <= - BAN_VALUE || Search.bvl >= BAN_VALUE) {
			break;
		}
	}
	printf( "totaltime: %.2fs\n", TimeCost(SEARCH_TOTAL_TIME) );
	fflush ( stdout );

	// 3. 输出最优着法
	if ( Search.bmv == 0 || Search.bvl <= - BAN_VALUE ) {
		printf ( "bestmove a0a1 resign\n" ); // 认输
		fflush ( stdout );
		return 0;
	}
	else {
		printf ( "bestmove %s\n", MoveIntToStr(Search.bmv).c_str() );
		fflush ( stdout );
		return Search.bmv;
	}
}