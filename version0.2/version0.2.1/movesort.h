#ifndef MOVESORT_TYPE_H_
#define MOVESORT_TYPE_H_

#include "base.h"
#include "position.h"

// 着法分类
const int SORT_TYPE_OTHER = 0;
const int SORT_TYPE_KILLER_2 = 1;
const int SORT_TYPE_KILLER_1 = 2;
const int SORT_TYPE_NICE_CAP = 3;
const int SORT_TYPE_HASHTABLE = 4;
const int MAX_SORT_TYPE = SORT_TYPE_HASHTABLE;

// 清空两个表
void ClearHistoryKillerTable ( void );

// 更新历史表、着法表
void InsertHistoryTable ( const int mv, const int depth );
void InsertKillerTable ( const int mv );

// 着法排序结构体
struct MoveSortStruct {
	int move[128];	// 着法表
	int nMoveIndex;	// 索引下标
	int nMoveNum;	// 着法表长度

	int InitAlphaBetaMove ( void ); // 生成着法

	int NextMove ( void ) { // 提供下一个着法
		if ( nMoveIndex < nMoveNum ) {
			nMoveIndex ++;
			return move [ nMoveIndex-1 ];
		}
		else {
			return 0;
		}
	}
};


#endif /* MOVESORT_TYPE_H_ */
