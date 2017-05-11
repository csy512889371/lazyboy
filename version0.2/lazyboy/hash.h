#ifndef HASH_H_
#define HASH_H_

#include "base.h"

const int HASH_TYPE_ALPHA = 0;
const int HASH_TYPE_BETA = 1;
const int HASH_TYPE_PV = 2;

void InitZobrist ( void );
void InitHashTable ( const int x );
void DelHashTable ( void );
void ClearHashTable ( void );
void InsertHashTable ( const int depth, const int val, const int mv, const int type );
int QueryValueInHashTable ( const int depth, const int alpha, const int beta );
int QueryMoveInHashTable ( void );

#endif /* HASH_H_ */