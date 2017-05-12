#ifndef SEARCH_H_
#define SEARCH_H_

#include "base.h"
#include "position.h"
#include "rollback.h"

const int SEARCH_MAX_DEPTH = 1000;

extern PositionStruct pos; // 当前搜索局面
extern RollBackListStruct roll; // 回滚着法表

const int nBest = 10;

struct SearchStruct {
	int bmv[nBest]; // 最佳着法
	int bvl[nBest]; // 最优得分
	int nNode; // 总节点数
	int nBeta; // beta点个数
	int depth;
};
extern SearchStruct Search;

int SearchMain ( void );

#endif /* SEARCH_H_ */
