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

// 零窗口搜索
int SearchCut ( int depth, int beta ) {
	int val, mv;
	int bstval = - MATE_VALUE;
	int bstmv = 0;
	MoveSortStruct mvsort;

	if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
		return bstval;
	}
	Search.nNode ++;
	Search.maxDistance = MAX ( Search.maxDistance, pos.nDistance );

	// 1. 无害裁剪
	val = HarmlessPruning ();
	if ( val > - MATE_VALUE ) {
		return val;
	}

	// 2. 打分
	if ( depth <= 0 ) {
		return pos.Evaluate ();
	}

	// 3. 置换裁剪
	val = QueryBestValueInHashTable ( depth, beta - 1, beta );
	if ( val != - MATE_VALUE ) {
		if ( pos.nDistance == 0 ) {
			Search.bmv = bstmv;
		}
		return val;
	}

	// 4. 达到极限深度
	if ( pos.nDistance >= SEARCH_MAX_DEPTH ) {
		return pos.Evaluate ();
	}

	// 5. 空着裁剪 ????

	// 6. 生成着法
	int nMoveNum = mvsort.InitCutMove ();

	// 7. 按照着法搜索
	while ( (mv = mvsort.NextMove()) != 0 ) {
		pos.MakeMove ( mv );

		// 8. 尝试选择性延伸
		int newDepth = ( pos.checked || nMoveNum == 1 ) ? depth : depth - 1;

		// 9. 递归搜索
		val = - SearchCut ( newDepth, 1 - beta );
		pos.UndoMakeMove ();

		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			return bstval;
		}

		// 10. 边界
		if ( val > bstval ) {
			bstval = val;
			bstmv = mv;
			if ( bstval >= beta ) {
				Search.nBeta ++;
				InsertMoveToHashTable ( depth, bstmv, bstval, HASH_TYPE_BETA );
				InsertHistoryTable ( bstmv, depth );
				return bstval;
			}
		}
	}

	// 11. 最后
	InsertMoveToHashTable ( depth, bstmv, bstval, HASH_TYPE_ALPHA );
	return bstval;
}

// Alpha-Beta 搜索
int SearchAlphaBeta ( int depth, int alpha, int beta ) {
	int val, mv;
	int bstval = - MATE_VALUE;
	int bstmv = 0;
	int hash_type = HASH_TYPE_ALPHA;
	MoveSortStruct mvsort;

	if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
		return bstval;
	}
	Search.nNode ++;
	Search.maxDistance = MAX ( Search.maxDistance, pos.nDistance );

	// 1. 无害裁剪
	val = HarmlessPruning ();
	if ( val > - MATE_VALUE ) {
		return val;
	}

	// 2. 打分
	if ( depth <= 0 ) {
		return pos.Evaluate ();
	}

	// 3. 置换裁剪
	val = QueryBestValueInHashTable ( depth, alpha, beta );
	if ( val != - MATE_VALUE ) {
		if ( pos.nDistance == 0 ) {
			Search.bmv = bstmv;
		}
		return val;
	}

	// 4. 达到极限深度
	if ( pos.nDistance >= SEARCH_MAX_DEPTH ) {
		return pos.Evaluate ();
	}

	// 5. 内部迭代加深启发
	mv = QueryBestMoveInHashTable ();
	if ( depth > 2 && mv == 0 ) {
		val = SearchAlphaBeta ( depth / 2, alpha, beta );
		if ( val <= alpha ) {
			val = SearchAlphaBeta ( depth / 2, - MATE_VALUE, beta );
		}
		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			return bstval;
		}
	}

	// 6. 生成着法
	int nMoveNum = mvsort.InitAlphaBetaMove ();

	// 7. 按照着法搜索
	while ( (mv = mvsort.NextMove()) != 0 ) {
		pos.MakeMove ( mv );

		// 8. 尝试选择性延伸
		int newDepth = ( pos.checked || nMoveNum == 1 ) ? depth : depth - 1;

		// 9. 零窗口搜索、递归搜索
		if ( bstval == - MATE_VALUE ) {
			val = - SearchAlphaBeta ( newDepth, -beta, -alpha );
		}
		else {
			val = - SearchCut ( newDepth, -alpha );
			if ( val > alpha && val < beta ) {
				val = - SearchAlphaBeta ( newDepth, -beta, -alpha );
			}
		}
		pos.UndoMakeMove ();

		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			return bstval;
		}

		// 10. 边界
		if ( val > bstval ) {
			bstval = val;
			bstmv = mv;
			if ( bstval >= beta ) {
				Search.nBeta ++;
				hash_type = HASH_TYPE_BETA;
				break;
			}
			if ( bstval > alpha ) {
				alpha = bstval;
				hash_type = HASH_TYPE_PV;
			}
		}
	}

	// 11. 最后
	InsertMoveToHashTable ( depth, bstmv, bstval, hash_type );
	InsertHistoryTable ( bstmv, depth );
	if ( pos.nDistance == 0 && bstmv != 0 ) {
		Search.bmv = bstmv;
	}
	return bstval;
}

// 主搜索函数
int SearchMain ( void ) {
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

	// 2. 初始化
	ClearHistoryTable ();
	InitBeginTime ( SEARCH_TOTAL_TIME );

	// 3. 迭代加深搜索
	printf("depth   time    nNode  rBeta   value  bestmv\n");
	fflush ( stdout );
	int last_bvl = - MATE_VALUE, last_bmv = 0;
	for ( int depth = 1; /*depth <= ?*/; depth ++ ) {
		InitBeginTime ( THIS_SEARCH_TIME );
		Search.bmv = 0;
		Search.nNode = Search.nBeta = 0;
		Search.maxDistance = 0;
		Search.bvl = SearchAlphaBeta ( depth, - MATE_VALUE, MATE_VALUE );

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
	printf ( "maxDistance: %d\n", Search.maxDistance );
	printf ( "totaltime: %.2fs\n", TimeCost(SEARCH_TOTAL_TIME) );
	fflush ( stdout );

	// 4. 输出最优着法
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
