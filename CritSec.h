

#ifndef _CRITSEC_H_
#define _CRITSEC_H_

#include <ucos.h>

class CritSec
{
    OS_CRIT 		m_CritSec;

public:
    CritSec()               { OSCritInit( &m_CritSec ); }
    ~CritSec()              {}
    void Lock( int t = 0 )  { OSCritEnter( &m_CritSec, t ); }
    void Unlock()           { OSCritLeave( &m_CritSec ); }

}; /* CritSec */


class AutoLock
{
    CritSec* m_pCritSec;

public:
    AutoLock( CritSec* pCS, int t = 0 ) : m_pCritSec( pCS )
                    { m_pCritSec->Lock( t ); }
    ~AutoLock()     { m_pCritSec->Unlock(); }

}; /* AutoLock */

#endif /* _CRITSEC_H_ */

