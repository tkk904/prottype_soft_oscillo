//
//
//

#ifndef PRIMADO2_MONITOR_H_
#define PRIMADO2_MONITOR_H_

#include "DrillMonitor.h"

#include "TSND121.h"
#include "Primado2.h"

class CPrimado2Monitor : public IDrillMonitor
{
	CPrimado2 m_primado2;
	CTSND121 m_tsnd121;
public:
	virtual int  GetData(double* ptr, int count);
	virtual int  GetError();
	virtual bool Start(const char* spindle, const char* sensor);
	virtual void Stop();
	virtual bool Initialize();
	virtual void Finalize();
	//
	static CPrimado2Monitor& Instance();
private:
	CPrimado2Monitor();
	virtual ~CPrimado2Monitor();
};

#endif // !PRIMADO2_MONITOR_H_
