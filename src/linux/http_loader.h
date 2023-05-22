#pragma once

class http_loader {
	CURL* m_curl;
	std::wstring m_url;
public:
	http_loader();
	~http_loader();

	Glib::RefPtr<Gio::InputStream> load_file(const litehtml::string& url);
	const char* get_url() const;

private:
	static size_t curlWriteFunction(void* ptr, size_t size, size_t nmemb, void* stream);
};

inline const char* http_loader::get_url() const {
	return m_url.c_str();
}
