//
//
//

#pragma once

#include <mmsystem.h>

class CMultiMediaTimer
{
	void(*m_handler)(void*);
	void* m_parameter;
	unsigned long m_msec;
	MMRESULT m_timerID;
public:
	bool IsRunning() const;
	void Period(unsigned long msec);
	void Handler(void(*handler)(void*), void*);
	void Start(unsigned long msec, void(*handler)(void*), void*);
	void Start();
	void Stop();
	CMultiMediaTimer();
	virtual ~CMultiMediaTimer();
private:
	void TimerHandler();
	static void CALLBACK TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
};

