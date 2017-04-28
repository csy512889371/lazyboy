#ifndef HASH_H_
#define HASH_H_

#include "base.h"

void InitZobrist ( void );
void InitHashTable ( const int x );
void DelHashTable ( void );
void ClearHashTable ( void );
void InsertHashTable ( const int depth, const int val, const int mv );
int QueryValueInHashTable ( const int depth );
int QueryMoveInHashTable ( void );

#endif /* HASH_H_ */
