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
	int vl, mv;
	int bvl = - MATE_VALUE;
	int bmv = 0;
	MoveSortStruct mvsort;

	if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
		return bvl;
	}
	Search.nNode ++;

	// 无害裁剪
	vl = HarmlessPruning ();
	if ( vl > - MATE_VALUE ) {
		return vl;
	}

	// 达到极限深度
	if ( depth <= 0 || pos.nDistance >= SEARCH_MAX_DEPTH ) {
		// 选择性延伸
		if ( pos.checked || pos.nDistance == 0 ) {
			return SearchCut ( 1, beta );
		}
		return pos.Evaluate ();
	}

	// 置换裁剪
	vl = QueryBestValueInHashTable ( depth, beta - 1, beta );
	if ( vl != - MATE_VALUE ) {
		return vl;
	}

	// 生成着法
	int nMoveNum = mvsort.InitCutMove ();

	// 按照着法搜索
	while ( (mv = mvsort.NextMove()) != 0 ) {
		pos.MakeMove ( mv );

		// 尝试选择性延伸
		int newDepth = ( pos.checked || nMoveNum == 1 ) ? depth : depth - 1;

		// 递归搜索
		vl = - SearchCut ( newDepth, 1 - beta );
		pos.UndoMakeMove ();

		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			return bvl;
		}

		// 边界
		if ( vl > bvl ) {
			bvl = vl;
			bmv = mv;
			if ( vl >= beta ) {
				Search.nBeta ++;
				InsertMoveToHashTable ( depth, bmv, bvl, HASH_TYPE_BETA );
				InsertHistoryTable ( bmv, depth );
				return vl;
			}
		}
	}

	// 最后
	InsertMoveToHashTable ( depth, bmv, bvl, HASH_TYPE_ALPHA );
	return bvl;
}

// Alpha-Beta 搜索
int SearchAlphaBeta ( int depth, int alpha, int beta ) {
	int vl, mv, mvHash, vlHash;
	int bvl[nBest], bmv[nBest];
	for ( int i = 0; i < nBest; i ++ ) {
		bvl[i] = - MATE_VALUE;
		bmv[i] = 0;
	}
	int hash_type = HASH_TYPE_ALPHA;
	MoveSortStruct mvsort;

	if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
		return bvl[0];
	}
	Search.nNode ++;

	// 无害裁剪
	vl = HarmlessPruning ();
	if ( vl > - MATE_VALUE ) {
		return vl;
	}

	// 达到极限深度
	if ( depth <= 0 || pos.nDistance >= SEARCH_MAX_DEPTH ) {
		// 选择性延伸
		if ( pos.checked || pos.nDistance == 0 ) {
			return SearchAlphaBeta ( 1, alpha, beta );
		}
		return pos.Evaluate ();
	}

	// 置换裁剪
	vlHash = QueryBestValueInHashTable ( depth, alpha, beta );
	if ( vlHash != - MATE_VALUE ) {
		return vl;
	}

	// 内部迭代加深启发
	if ( depth > 2 ) {
		mvHash = QueryBestMoveInHashTable ();
		if ( mvHash == 0 ) {
			SearchAlphaBeta ( depth / 2, alpha, beta );
			if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
				return bvl[0];
			}
		}
	}

	// 生成着法
	int nMoveNum = mvsort.InitAlphaBetaMove ();

	// 大搜索
	while ( (mv = mvsort.NextMove()) != 0 ) {
		pos.MakeMove ( mv );
		int newDepth = ( pos.checked || nMoveNum == 1 ) ? depth : depth - 1;
		int cutDepth = ( newDepth > 4 ? newDepth - 2 : newDepth );
		vl = - SearchCut ( cutDepth, - alpha );
		if ( vl > alpha && vl < beta ) {
			vl = - SearchAlphaBeta ( newDepth, -beta, -alpha );
		}
		pos.UndoMakeMove ();

		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			return bvl[0];
		}

		if ( pos.nDistance == 0 ) {
			for ( int i = 0; i < nBest; i ++ ) {
				if ( vl > bvl[i] ) {
					for ( int j = nBest - 1; j > i; j -- ) {
						bvl[j] = bvl[j-1];
						bmv[j] = bmv[j-1];
					}
					bvl[i] = vl;
					bmv[i] = mv;
					break;
				}
			}
		}
		else if ( vl > bvl[0] ) {
			bvl[0] = vl;
			bmv[0] = mv;
		}

		// 边界
		if ( vl >= beta ) {
			Search.nBeta ++;
			hash_type = HASH_TYPE_BETA;
			break;
		}
		if ( vl > alpha ) {
			alpha = vl;
			hash_type = HASH_TYPE_PV;
		}
	}

	// 最后
	InsertMoveToHashTable ( depth, bmv[0], bvl[0], hash_type );
	InsertHistoryTable ( bmv[0], depth );
	if ( pos.nDistance == 0 ) {
		for ( int i = 0; i < nBest; i ++ ) {
			Search.bvl[i] = bvl[i];
			Search.bmv[i] = bmv[i];
		}
	}
	return bvl[0];
}

// 主搜索函数
int SearchMain ( void ) {
	// 特殊情况
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

	// 初始化
	ClearHistoryTable ();
	InitBeginTime ( SEARCH_TOTAL_TIME );

	// 迭代加深搜索
	printf("depth   time    nNode  rBeta");
	for ( int i = 0; i < nBest; i ++ ) {
		printf("   bvl[%d]  bmv[%d]", i, i);
	}
	printf("\n");
	fflush ( stdout );
	int lastbvl[nBest], lastbmv[nBest];
	for ( int i = 0; i < nBest; i ++ ) {
		lastbvl[i] = - MATE_VALUE;
		lastbmv[i] = 0;
	}
	for ( int depth = 1; /*depth <= ?*/; depth ++ ) {
		InitBeginTime ( THIS_SEARCH_TIME );
		for ( int i = 0; i < nBest; i ++ ) {
			Search.bvl[i] = - MATE_VALUE;
			Search.bmv[i] = 0;
		}
		Search.nNode = Search.nBeta = 0;
		Search.depth = depth;

		SearchAlphaBeta ( depth, - MATE_VALUE, MATE_VALUE );

		if ( TimeOut(SEARCH_TOTAL_TIME) ) { // 超时
			for ( int i = 0; i < nBest; i ++ ) {
				Search.bvl[i] = lastbvl[i];
				Search.bmv[i] = lastbmv[i];
			}
			break;
		}
		else {
			for ( int i = 0; i < nBest; i ++ ) {
				lastbvl[i] = Search.bvl[i];
				lastbmv[i] = Search.bmv[i];
			}
		}

		// 重要信息输出
		printf( "%5d  %.2fs  %7d    %2.0f%%", depth, TimeCost(THIS_SEARCH_TIME), Search.nNode, 100.0*Search.nBeta/Search.nNode);
		for ( int i = 0; i < nBest; i ++ ) {
			printf("   %6d    %s", Search.bvl[i], MoveIntToStr(Search.bmv[i]).c_str());
		}
		printf("\n");
		fflush ( stdout );

		if ( Search.bvl[0] <= - BAN_VALUE || Search.bvl[0] >= BAN_VALUE) {
			break;
		}
	}
	printf ( "totaltime: %.2fs\n", TimeCost(SEARCH_TOTAL_TIME) );
	fflush ( stdout );

	// 输出最优着法
	if ( Search.bmv[0] == 0 || Search.bvl[0] <= - BAN_VALUE ) {
		printf ( "bestmove a0a1 resign\n" ); // 认输
		fflush ( stdout );
		return 0;
	}
	else {
		printf ( "bestmove %s\n", MoveIntToStr(Search.bmv[0]).c_str() );
		fflush ( stdout );
		return Search.bmv[0];
	}
}
