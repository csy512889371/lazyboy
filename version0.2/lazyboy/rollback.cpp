#include "rollback.h"
#include "evaluate.h"
#include "search.h"
#include "debug.h"

// 初始化回滚结构体
void RollBackListStruct::Init ( void ) {
	nRollNum = 0;
	for ( int i = 0; i < RBL_MAXN; i ++ ) {
		move[i] = 0;
		dstPiece[i] = 0;
		check[i] = 0;
		checked[i] = 0;
		zobrist[i] = std::make_pair ( 0, 0 );
	}
}

// 判断重复类型
int RollBackListStruct::RepStatus ( void ) const {
	// 1. 逐个往回寻找
	int ThisSideConCheck = ( nRollNum == 0 ) ? 0 : checked[ nRollNum - 1 ];
	int OppSideConCheck = pos.checked;
	int TurnThisSide = 1;
	for ( int i = nRollNum - 1; i >= 0; i -- ) {
		if ( TurnThisSide ) {
			TurnThisSide = 0;
			ThisSideConCheck &= checked[i];
		}
		else {
			TurnThisSide = 1;
			OppSideConCheck &= checked[i];
		}
		if ( zobrist[i] == pos.zobrist ) {
			return ThisSideConCheck == OppSideConCheck ? REP_DRAW :
					( ThisSideConCheck > OppSideConCheck ? REP_LOSE : REP_WIN );
		}
	}

	return REP_NONE;
}

// 返回重复打分值
int RollBackListStruct::RepValue ( const int vRep ) const {
	assert( vRep != REP_NONE );
	switch ( vRep ) {
		case REP_WIN:
			return BAN_VALUE;
		case REP_LOSE:
			return -BAN_VALUE;
		default: // REP_DRAW
			return 0;
	}
	return 0;
}
