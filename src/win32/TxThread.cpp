#include "../globals.h"
#include "TxThread.h"

TxThread::TxThread() {
	m_hThread = NULL;
	m_hStop = NULL;
	m_trdID = NULL;
}

TxThread::~TxThread() {
	Stop();
	if (m_hThread) CloseHandle(m_hThread);
	if (m_hStop) CloseHandle(m_hStop);
}

void TxThread::Run() {
	Stop();
	m_hStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hThread = CreateThread(NULL, 0, sThreadProc, (LPVOID)this, 0, &m_trdID);
}

void TxThread::Stop() {
	if (m_hThread) {
		if (m_hStop) {
			SetEvent(m_hStop);
		}
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	if (m_hStop)   CloseHandle(m_hStop);
	m_hStop = NULL;
}

DWORD WINAPI TxThread::sThreadProc(LPVOID lpParameter) {
	TxThread* pThis = (TxThread*)lpParameter;
	return pThis->ThreadProc();
}

BOOL TxThread::WaitForStop(DWORD ms) {
	if (WaitForSingleObject(m_hStop, ms) != WAIT_TIMEOUT) {
		return TRUE;
	}
	return FALSE;
}

void TxThread::postMessage(UINT Msg, WPARAM wParam, LPARAM lParam) {
	if (m_hThread) {
		PostThreadMessage(m_trdID, Msg, wParam, lParam);
	}
}