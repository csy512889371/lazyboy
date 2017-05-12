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
void InsertMoveToHashTable ( const int depth, const int bstmv, const int bstval, const int type );
int QueryBestValueInHashTable ( const int depth, const int alpha, const int beta );
int QueryBestMoveInHashTable ( void );

#endif /* HASH_H_ */
