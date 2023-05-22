#define UNICODE
#include "http_loader.h"
#pragma comment(lib, "winhttp.lib")

http_loader::http_loader() {
	m_hSession = NULL;
	m_maxConnectionsPerServer = 5;
	InitializeCriticalSectionAndSpinCount(&m_sync, 1000);
}

http_loader::~http_loader() {
	stop();
	if (m_hSession) {
		WinHttpCloseHandle(m_hSession);
	}
	DeleteCriticalSection(&m_sync);
}

BOOL http_loader::open(LPCWSTR pwszUserAgent, DWORD dwAccessType, LPCWSTR pwszProxyName, LPCWSTR pwszProxyBypass) {
	m_hSession = WinHttpOpen(pwszUserAgent, dwAccessType, pwszProxyName, pwszProxyBypass, WINHTTP_FLAG_ASYNC);
	if (m_hSession) {
		WinHttpSetOption(m_hSession, WINHTTP_OPTION_MAX_CONNS_PER_SERVER, &m_maxConnectionsPerServer, sizeof(m_maxConnectionsPerServer));

		if (WinHttpSetStatusCallback(m_hSession, (WINHTTP_STATUS_CALLBACK)http_loader::http_callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0) != WINHTTP_INVALID_STATUS_CALLBACK) {
			return TRUE;
		}
	}
	if (m_hSession) {
		WinHttpCloseHandle(m_hSession);
	}
	return FALSE;
}

void http_loader::close() {
	if (m_hSession) {
		WinHttpCloseHandle(m_hSession);
		m_hSession = NULL;
	}
}

VOID CALLBACK http_loader::http_callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
	CoInitialize(NULL);

	DWORD dwError = ERROR_SUCCESS;
	http_request* request = (http_request*)dwContext;

	if (request) {
		switch (dwInternetStatus) {
		case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
			dwError = request->onSendRequestComplete();
			break;
		case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
			dwError = request->onHeadersAvailable();
			break;
		case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
			dwError = request->onReadComplete(dwStatusInformationLength);
			break;
		case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING: dwError = request->onHandleClosing(); break;
		case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
			dwError = request->onRequestError(((WINHTTP_ASYNC_RESULT*)lpvStatusInformation)->dwError);
			break;
		}
		if (dwError != ERROR_SUCCESS) {
			request->cancel();
		}
	}

	CoUninitialize();
}

BOOL http_loader::download_file(LPCWSTR url, http_request* request) {
	if (request) {
		request->set_parent(this);
		if (request->create(url, m_hSession)) {
			lock();
			m_requests.push_back(request);
			unlock();
			return TRUE;
		}
	}
	return FALSE;
}

void http_loader::remove_request(http_request* request) {
	bool is_ok = false;
	lock();
	for (http_request::vector::iterator i = m_requests.begin(); i != m_requests.end(); i++) {
		if ((*i) == request) {
			m_requests.erase(i);
			is_ok = true;
			break;
		}
	}
	unlock();
	if (is_ok) {
		request->release();
	}
}

void http_loader::stop() {
	lock();

	for (http_request::vector::iterator i = m_requests.begin(); i != m_requests.end(); i++) {
		(*i)->cancel();
	}

	unlock();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

http_request::http_request() {
	m_status = 0;
	m_error = 0;
	m_downloaded_length = 0;
	m_content_length = 0;
	m_refCount = 1;
	m_hConnection = NULL;
	m_hRequest = NULL;
	m_http = NULL;
	InitializeCriticalSection(&m_sync);
}

http_request::~http_request() {
	cancel();
	DeleteCriticalSection(&m_sync);
}

BOOL http_request::create(LPCWSTR url, HINTERNET hSession) {
	m_url = url;
	m_error = ERROR_SUCCESS;

	URL_COMPONENTS urlComp;

	ZeroMemory(&urlComp, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);

	urlComp.dwSchemeLength = -1;
	urlComp.dwHostNameLength = -1;
	urlComp.dwUrlPathLength = -1;
	urlComp.dwExtraInfoLength = -1;

	if (!WinHttpCrackUrl(url, lstrlen(url), 0, &urlComp)) {
		return FALSE;
	}

	std::wstring host;
	std::wstring path;
	std::wstring extra;

	host.insert(0, urlComp.lpszHostName, urlComp.dwHostNameLength);
	path.insert(0, urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	if (urlComp.dwExtraInfoLength) {
		extra.insert(0, urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
	}

	DWORD flags = 0;
	if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
		flags = WINHTTP_FLAG_SECURE;
	}

	m_hConnection = WinHttpConnect(hSession, host.c_str(), urlComp.nPort, 0);

	PCWSTR pwszAcceptTypes[] = { L"*/*", NULL };

	path += extra;

	m_hRequest = WinHttpOpenRequest(m_hConnection, L"GET", path.c_str(), NULL, NULL, pwszAcceptTypes, flags);

	lock();
	if (!m_hRequest) {
		WinHttpCloseHandle(m_hConnection);
		m_hConnection = NULL;
		unlock();
		return FALSE;
	}

	DWORD options = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
	WinHttpSetOption(m_hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &options, sizeof(options));

	if (!WinHttpSendRequest(m_hRequest, NULL, 0, NULL, 0, 0, (DWORD_PTR)this)) {
		WinHttpCloseHandle(m_hRequest);
		m_hRequest = NULL;
		WinHttpCloseHandle(m_hConnection);
		m_hConnection = NULL;
		unlock();
		return FALSE;
	}
	unlock();

	return TRUE;
}

void http_request::cancel() {
	lock();
	if (m_hRequest) {
		WinHttpCloseHandle(m_hRequest);
		m_hRequest = NULL;
	}
	if (m_hConnection) {
		WinHttpCloseHandle(m_hConnection);
		m_hConnection = NULL;
	}
	unlock();
}

DWORD http_request::onSendRequestComplete() {
	lock();
	DWORD dwError = ERROR_SUCCESS;

	if (!WinHttpReceiveResponse(m_hRequest, NULL)) {
		dwError = GetLastError();
	}

	unlock();

	return dwError;
}

DWORD http_request::onHeadersAvailable() {
	lock();

	DWORD dwError = ERROR_SUCCESS;
	m_status = 0;
	DWORD StatusCodeLength = sizeof(m_status);

	OnHeadersReady(m_hRequest);

	if (!WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE, NULL, &m_status, &StatusCodeLength, NULL)) {
		dwError = GetLastError();
	}
	else {
		WCHAR buf[255];
		DWORD len = sizeof(buf);
		if (WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CONTENT_LENGTH, NULL, buf, &len, NULL)) {
			m_content_length = _wtoi64(buf);
		}
		else {
			m_content_length = 0;
		}
		m_downloaded_length = 0;

		dwError = readData();
	}

	unlock();

	return dwError;
}

DWORD http_request::onHandleClosing() {
	WCHAR errMsg[255];
	errMsg[0] = 0;

	if (m_error) {
		FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM, GetModuleHandle(L"winhttp.dll"), m_error, 0, errMsg, 255, NULL);
	}

	OnFinish(m_error, errMsg);
	m_http->remove_request(this);

	return ERROR_SUCCESS;
}

DWORD http_request::onRequestError(DWORD dwError) {
	m_error = dwError;
	return m_error;
}

DWORD http_request::readData() {
	DWORD dwError = ERROR_SUCCESS;

	if (!WinHttpReadData(m_hRequest, m_buffer, sizeof(m_buffer), NULL)) {
		dwError = GetLastError();
	}

	return dwError;
}

DWORD http_request::onReadComplete(DWORD len) {
	DWORD dwError = ERROR_SUCCESS;

	if (len != 0) {
		lock();
		m_downloaded_length += len;
		OnData(m_buffer, len, m_downloaded_length, m_content_length);
		dwError = readData();
		unlock();
	}
	else {
		cancel();
	}

	return dwError;
}

void http_request::add_ref() {
	InterlockedIncrement(&m_refCount);
}

void http_request::release() {
	LONG lRefCount;
	lRefCount = InterlockedDecrement(&m_refCount);
	if (lRefCount == 0) {
		delete this;
	}
}

void http_request::set_parent(http_loader* parent) {
	m_http = parent;
}

void http_request::OnHeadersReady(HINTERNET hRequest) {
}