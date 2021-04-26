#include <windows.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <tchar.h>
#include <wrl.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <wininet.h>

#include "picojson.h"
#include "resource.h"
#include "WebView2.h"
#include "version.h"

#if _DEBUG
#define FCC_DEVTOOL
#define FCC_ACCEPTALL
#endif

using namespace Microsoft::WRL;

static bool report(HRESULT hr, LPCWSTR message) {
	if (SUCCEEDED(hr)) {
		return false;
	}
	std::wstring m(message);
	m += L": ";
	{
		LPWSTR t = NULL;
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			hr,
			LANG_USER_DEFAULT,
			(LPWSTR)&t,
			0,
			NULL);
		m += t;
		LocalFree(t);
	}
	m += L"\n";
	OutputDebugStringW(m.c_str());
	return true;
}

static HRESULT to_u16(LPCSTR src, const int srclen, std::wstring& dest) {
	if (srclen == 0) {
		dest.resize(0);
		return S_OK;
	}
	const int destlen = MultiByteToWideChar(CP_UTF8, 0, src, srclen, nullptr, 0);
	if (destlen == 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	dest.resize(destlen);
	if (MultiByteToWideChar(CP_UTF8, 0, src, srclen, &dest[0], destlen) == 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (srclen == -1) {
		dest.resize(destlen - 1);
	}
	return S_OK;
}

static HRESULT to_u8(LPCWSTR src, const int srclen, std::string& dest) {
	if (srclen == 0) {
		dest.resize(0);
		return S_OK;
	}
	const int destlen = WideCharToMultiByte(CP_UTF8, 0, src, srclen, nullptr, 0, nullptr, nullptr);
	if (destlen == 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	dest.resize(destlen);
	if (WideCharToMultiByte(CP_UTF8, 0, src, srclen, &dest[0], destlen, nullptr, nullptr) == 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (srclen == -1) {
		dest.resize(destlen - 1);
	}
	return S_OK;
}

static HRESULT CALLBACK show_save_dialog(HWND hWnd, LPCWSTR filename, std::wstring& dest) {
	ComPtr<IFileSaveDialog> d;
	HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&d));
	if (FAILED(hr))
	{
		return hr;
	}
	const COMDLG_FILTERSPEC filter[] =
	{
		{ L"Wave ファイル(*.wav)", L"*.wav" }
	};
	hr = d->SetFileTypes(ARRAYSIZE(filter), filter);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = d->SetFileTypeIndex(1);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = d->SetDefaultExtension(L"wav");
	if (FAILED(hr))
	{
		return hr;
	}
	hr = d->SetFileName(filename);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = d->Show(hWnd);
	if (FAILED(hr))
	{
		return hr;
	}
	ComPtr<IShellItem> r;
	hr = d->GetResult(&r);
	if (FAILED(hr))
	{
		return hr;
	}
	PWSTR filepath = NULL;
	hr = r->GetDisplayName(SIGDN_FILESYSPATH, &filepath);
	if (FAILED(hr))
	{
		return hr;
	}
	dest = filepath;
	CoTaskMemFree(filepath);
	return S_OK;
}

static HRESULT download(LPCWSTR user_agent, LPCWSTR url, LPCWSTR filepath) {
	const HANDLE file = CreateFileW(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	HINTERNET inet = InternetOpen(user_agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!inet) {
		CloseHandle(file);
		return HRESULT_FROM_WIN32(GetLastError());
	}
	HINTERNET h = InternetOpenUrl(inet, url, NULL, 0, 0, 0);
	if (!h) {
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		InternetCloseHandle(inet);
		CloseHandle(file);
		return hr;
	}
	char buf[4096] = {};
	DWORD len = 0;
	do {
		if (!InternetReadFile(h, buf, 4096, &len)) {
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			InternetCloseHandle(h);
			InternetCloseHandle(inet);
			CloseHandle(file);
			return hr;
		}
		if (len > 0) {
			DWORD written = 0;
			if (!WriteFile(file, buf, len, &written, nullptr)) {
				HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
				InternetCloseHandle(h);
				InternetCloseHandle(inet);
				CloseHandle(file);
				return hr;
			}
		}
	} while (len > 0);
	InternetCloseHandle(h);
	InternetCloseHandle(inet);
	CloseHandle(file);
	return S_OK;
}

static HRESULT write_text(LPCWSTR filepath, LPCWSTR text) {
	std::string u8;
	HRESULT hr = to_u8(text, -1, u8);
	if (FAILED(hr)) {
		return hr;
	}
	const HANDLE file = CreateFileW(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	std::string::size_type pos = 0;
	while (pos < u8.size())
	{
		DWORD written = 0;
		if (!WriteFile(file, &u8[pos], u8.size() - pos, &written, nullptr)) {
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			CloseHandle(file);
			return hr;
		}
		pos += written;
	}
	CloseHandle(file);
	return S_OK;
}

static void sanitize(LPCWSTR src, std::wstring& dest) {
	dest.resize(0);
	while (*src != L'\0') {
		switch (*src) {
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
		case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
		case 0x22: case 0x2a: case 0x2f: case 0x3a: case 0x3c: case 0x3e: case 0x3f: case 0x7c:
		case 0x7f:
			dest += L"_";
			break;
		default:
			dest += *src;
			break;
		}
		++src;
	}
}

static void build_default_filename(LPCWSTR character, LPCWSTR text, std::wstring& ret) {
	ret.resize(0);
	{
		SYSTEMTIME st = {};
		GetLocalTime(&st);
		wchar_t datetime[128] = {};
		swprintf_s(datetime, 128, L"%04d%02d%02d_%02d%02d%02d_", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		ret += datetime;
	}
	{
		std::wstring s;
		sanitize(character, s);
		ret += s;
	}
	ret += L"_";
	{
		std::wstring s;
		sanitize(text, s);
		if (s.size() > 10) {
			s.resize(9);
			ret += s;
			ret += L"…";
		}
		else {
			ret += s;
		}
	}
	ret += L".wav";
}

class DarkMode {
	bool supported_;
	HMODULE uxtheme_;
	HBRUSH dark_brush_;
	OSVERSIONINFOEXW ver_;
	typedef BOOL(WINAPI* AllowDarkModeForAppFunc)(BOOL);
	AllowDarkModeForAppFunc AllowDarkModeForApp_;
public:
	DarkMode() : supported_(false), uxtheme_(nullptr), dark_brush_(nullptr), ver_({}), AllowDarkModeForApp_(nullptr) {
		supported_ = init_version() && is_dark_mode_supported_os();
		if (!supported_) {
			return;
		}
		uxtheme_ = LoadLibraryEx(_T("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!uxtheme_) {
			return;
		}
		AllowDarkModeForApp_ = (AllowDarkModeForAppFunc)GetProcAddress(uxtheme_, MAKEINTRESOURCEA(135));
		if (AllowDarkModeForApp_) {
			AllowDarkModeForApp_(TRUE); // System menu
		}
	}
	virtual ~DarkMode() {
		if (uxtheme_) {
			FreeLibrary(uxtheme_);
		}
		if (dark_brush_) {
			DeleteObject(dark_brush_);
		}
	}
	void set(HWND h) {
		if (!supported_) {
			return;
		}
		BOOL b = is_dark_mode_enabled() ? TRUE : FALSE;
		DwmSetWindowAttribute(h, ver_.dwBuildNumber >= 19041 ? 20 : 19, &b, sizeof(BOOL));
		if (b) {
			if (!dark_brush_) {
				dark_brush_ = CreateSolidBrush(RGB(0x34, 0x34, 0x34));
			}
			SetClassLong(h, GCL_HBRBACKGROUND, (LONG)dark_brush_);
		}
		else {
			SetClassLong(h, GCL_HBRBACKGROUND, (LONG)(COLOR_WINDOW + 1));
		}
		InvalidateRect(h, NULL, TRUE);
	}
	void cleanup(HWND h) {
		SetClassLong(h, GCL_HBRBACKGROUND, (LONG)(COLOR_WINDOW + 1));
	}
private:
	bool init_version() {
		const HMODULE h = LoadLibraryEx(_T("ntdll.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!h) {
			return false;
		}
		typedef INT(WINAPI* RtlGetVersionFunc) (OSVERSIONINFOEXW*);
		RtlGetVersionFunc RtlGetVersion_ = (RtlGetVersionFunc)GetProcAddress(h, "RtlGetVersion");
		if (!RtlGetVersion_) {
			FreeLibrary(h);
			return false;
		}
		RtlGetVersion_(&ver_);
		return true;
	}
	bool is_dark_mode_enabled() const {
		HKEY h;
		if (RegOpenKey(HKEY_CURRENT_USER, _T(R"REG(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)REG"), &h) != ERROR_SUCCESS) {
			return false;
		}
		DWORD typ = REG_DWORD, v = 0, sz = sizeof(DWORD);
		const auto r = RegQueryValueEx(h, _T("AppsUseLightTheme"), 0, &typ, (LPBYTE)&v, &sz);
		RegCloseKey(h);
		if (r != ERROR_SUCCESS || typ != REG_DWORD) {
			return false;
		}
		return v == 0;
	}
	bool is_dark_mode_supported_os() const {
		if (ver_.dwMajorVersion > 10 || (ver_.dwMajorVersion == 10 && ver_.dwBuildNumber >= 17763)) {
			return true;
		}
		return false;
	}
};

class API {
	typedef std::function<void(const bool, const picojson::object)> resolver;
	typedef std::function<void()> task;
	HWND window_;
	EventRegistrationToken token_;
	LONG waiting;
	CRITICAL_SECTION cs;
	std::vector<task> tasks;
public:
	API() : window_(nullptr), cs({}) {
		InitializeCriticalSection(&cs);
	}
	virtual ~API() {
		DeleteCriticalSection(&cs);
	}

	void set_window(HWND hWnd) {
		window_ = hWnd;
	}

	void pump() {
		LONG n = InterlockedAdd(&waiting, 0);
		if (n <= 0) {
			return;
		}
		EnterCriticalSection(&cs);
		for (task t : tasks) {
			t();
		}
		tasks.clear();
		InterlockedExchange(&waiting, 0);
		LeaveCriticalSection(&cs);
	}

	HRESULT install(ComPtr<ICoreWebView2> webview) {
		HRESULT hr = webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
			[this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
			return handle(webview, args);
		}).Get(), &token_);
		if (FAILED(hr)) {
			return hr;
		}
		hr = webview->AddScriptToExecuteOnDocumentCreated(LR"JS(CoeFontStudioFrontend = (()=>{
"use strict";
const cbs = {};
let id = 0;
window.chrome.webview.addEventListener("message", e => {
  if (cbs[e.data.id]) {
	const cb = cbs[e.data.id];
    delete cbs[e.data.id];
    if (e.data.err) {
      cb[1](e.data.err);
    } else {
      cb[0](e.data.params);
    }
  }
});
const call = (method, params) => {
  return new Promise((resolve, reject) => {
    cbs[++id] = [resolve, reject];
    if (!params) {
      params = {};
    }
    window.chrome.webview.postMessage({id, method, params});
  });
};

window.cbs = cbs;
document.addEventListener('click', e => {
  if (!e || !e.target) {
    return;
  }
  const a = e.target.closest('a.download-button');
  if (!a) {
    return;
  }
  if (!a.matches('.yomi-card-dl-btn')) {
    return;
  }
  e.stopPropagation();
  e.preventDefault();
  const userAgent = navigator.userAgent;
  const url = a.href;
  const text = document.querySelector('.maineditor .focusin .textarea textarea').value;
  const character = document.querySelector('.maineditor .focusin .speaker .v-select__selection').textContent;
  CoeFontStudioFrontend.download({userAgent, url, text, character}).catch(r => {
    alert(r.message);
  });
}, true);

return new Proxy({}, {
  get: function(obj, prop) {
    if (!(prop in obj)) {
      obj[prop] = call.bind(undefined, prop);
    }
    return obj[prop];
  }
});
})()
)JS", nullptr);
		if (FAILED(hr)) {
			return hr;
		}
		return S_OK;
	}
private:
	void add_task(task t) {
		EnterCriticalSection(&cs);
		tasks.push_back(t);
		InterlockedAdd(&waiting, 1);
		LeaveCriticalSection(&cs);
		PostMessage(window_, WM_APP + 0x2525, 0, 0);
	}
	HRESULT handle(ComPtr<ICoreWebView2> webview, ICoreWebView2WebMessageReceivedEventArgs* args) {
		picojson::value v;
		{
			PWSTR json = nullptr;
			{
				const HRESULT hr = args->get_WebMessageAsJson(&json);
				if (FAILED(hr)) {
					return E_FAIL;
				}
			}
			std::string u8;
			{
				const HRESULT hr = to_u8(json, -1, u8);
				CoTaskMemFree(json);
				if (FAILED(hr)) {
					return hr;
				}
			}
			const std::string err = picojson::parse(v, u8);
			if (!err.empty()) {
				return E_FAIL;
			}
		}
		if (!v.is<picojson::object>()) {
			return E_FAIL;
		}
		const picojson::object& obj = v.get<picojson::object>();
		const auto idit = obj.find(u8"id"), methodit = obj.find(u8"method"), paramsit = obj.find(u8"params");
		if (
			idit == obj.end() || methodit == obj.end() || paramsit == obj.end() ||
			!paramsit->second.is<picojson::object>()
			) {
			return E_FAIL;
		}
		std::string id = idit->second.to_str();
		picojson::object ret;
		dispatch(
			methodit->second.to_str(),
			paramsit->second.get<picojson::object>(),
			[id, webview, this](const bool ok, picojson::object ret) -> void {
			picojson::object robj;
			robj[u8"id"].set<std::string>(id);
			robj[ok ? u8"params" : u8"err"].set<picojson::object>(ret);
			std::wstring ws;
			{
				std::string u8(picojson::value(robj).serialize());
				const HRESULT hr = to_u16(u8.c_str(), (const int)u8.size(), ws);
				if (FAILED(hr)) {
					report(hr, L"to_u16 failed");
					return;
				}
			}
			add_task([webview, ws]() -> void {
				report(webview->PostWebMessageAsJson(ws.c_str()), L"PostWebMessageAsJson failed");
			});
		}
		);
		return S_OK;
	}

	void dispatch(const std::string& method, picojson::object params, resolver fn) const {
		if (method == u8"version") {
			return api_version(params, fn);
		}
		else if (method == u8"download") {
			return api_download(params, fn);
		}
		return error_invalid_call(fn);
	}

	void api_version(const picojson::object params, resolver fn) const {
		picojson::object result;
		result[u8"version"].set<std::string>(version);
		return fn(true, result);
	}
	void api_download(const picojson::object params, resolver fn) const {
		std::wstring user_agent;
		if (!get_string(params, u8"userAgent", user_agent)) {
			return error_invalid_args(fn);
		}
		std::wstring url;
		if (!get_string(params, u8"url", url)) {
			return error_invalid_args(fn);
		}
		std::wstring character;
		if (!get_string(params, u8"character", character)) {
			return error_invalid_args(fn);
		}
		std::wstring text;
		if (!get_string(params, u8"text", text)) {
			return error_invalid_args(fn);
		}

		std::wstring default_filename;
		std::wstring filename;
		{
			build_default_filename(character.c_str(), text.c_str(), default_filename);
			HRESULT hr = show_save_dialog(window_, default_filename.c_str(), filename);
			if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
				return error_abort(fn);
			}
			if (FAILED(hr)) {
				return error_internal(fn);
			}
		}
		std::thread t1(api_download_worker, user_agent, url, text, filename, fn);
		t1.detach();
	}
	static void api_download_worker(
		const std::wstring user_agent,
		const std::wstring url,
		const std::wstring text,
		const std::wstring filename,
		resolver fn
	) {
		HRESULT hr = download(user_agent.c_str(), url.c_str(), filename.c_str());
		if (FAILED(hr)) {
			return error_internal(fn);
		}

		std::wstring textname = filename;
		textname.resize(textname.rfind(L'.') + 1);
		textname += L"txt";
		hr = write_text(textname.c_str(), text.c_str());
		if (FAILED(hr)) {
			return error_internal(fn);
		}
		picojson::object result;
		return fn(true, result);
	}

	static void error(const char* code, const char* message, resolver fn) {
		picojson::object r;
		r[u8"code"].set<std::string>(code);
		r[u8"message"].set<std::string>(message);
		return fn(false, r);
	}
	static void error_abort(resolver fn) {
		return error(u8"abort", u8"処理が中断されました", fn);
	}
	static void error_internal(resolver fn) {
		return error(u8"internal error", u8"内部エラーです", fn);
	}
	static void error_invalid_call(resolver fn) {
		return error(u8"invalid function call", u8"正しくない呼び出しです", fn);
	}
	static void error_invalid_args(resolver fn) {
		return error(u8"invalid arguments", u8"引数が正しくありません", fn);
	}
	static void error_open_file(resolver fn) {
		return error(u8"cannot open file", u8"ファイルが開けません", fn);
	}
	static void error_close_file(resolver fn) {
		return error(u8"cannot close file", u8"ファイルが閉じられません", fn);
	}
	static void error_read_from_file(resolver fn) {
		return error(u8"failed to read from file", u8"ファイルからの読み込みに失敗しました", fn);
	}
	static void error_write_to_file(resolver fn) {
		return error(u8"failed to write to file", u8"ファイルへの書き込みに失敗しました", fn);
	}
	static bool get_string(const picojson::object obj, const char* name, std::wstring& s) {
		const auto it = obj.find(name);
		if (it == obj.end() || !it->second.is<std::string>()) {
			return false;
		}
		if (report(
			to_u16(it->second.get<std::string>().c_str(), -1, s),
			L"failed to convert to UTF-16"
		)) {
			return false;
		}
		return true;
	}
};

static HRESULT CALLBACK task_dialog_callback(_In_ HWND hWnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_ LONG_PTR lpRefData) {
	if (msg == TDN_HYPERLINK_CLICKED) {
		const auto r = ShellExecuteW(hWnd, L"open", (LPCWSTR)lParam, NULL, NULL, SW_SHOWNORMAL);
		if (r <= (HINSTANCE)32) {
			report(
				HRESULT_FROM_WIN32((int)r),
				L"ShellExecute failed"
			);
		}
	}
	return S_OK;
}

static HRESULT webview2_runtime_check(LPCWSTR title, LPCWSTR main_instruction, LPCWSTR content) {
	for (;;) {
		LPWSTR ver;
		const HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &ver);
		if (FAILED(hr) || ver == nullptr) {
			TASKDIALOGCONFIG config = {};
			config.cbSize = sizeof(TASKDIALOGCONFIG);
			config.dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
			config.nDefaultButton = IDCLOSE;
			config.hwndParent = nullptr;
			config.hInstance = nullptr;
			config.dwFlags = TDF_ENABLE_HYPERLINKS;
			config.pszWindowTitle = title;
			config.pszMainIcon = TD_ERROR_ICON;
			config.pszMainInstruction = main_instruction;
			config.pszContent = content;
			config.pfCallback = &task_dialog_callback;
			int nRes;
			if (report(
				TaskDialogIndirect(&config, &nRes, NULL, NULL),
				L"TaskDialogIndirect failed"
			)) {
				return E_FAIL;
			}
			if (nRes == IDRETRY) {
				continue;
			}
			return S_FALSE;
		}
		return S_OK;
	}
}

static const TCHAR szWindowClass[] = _T("cfs_frontend");
static const TCHAR szTitle[] = _T("非公式 CoeFont Studio フロントエンド");

static HINSTANCE hInst;
static API api;
static DarkMode dark_mode;
static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);

static ComPtr<ICoreWebView2Controller> webview_controller;
static ComPtr<ICoreWebView2> webview;

static HWND create_window(int show) {
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = window_proc;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex))
	{
		report(
			HRESULT_FROM_WIN32(GetLastError()),
			L"RegisterClassEx failed"
		);
		return 0;
	}

	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1200, 900,
		NULL,
		NULL,
		hInst,
		NULL
	);
	if (!hWnd)
	{
		report(
			HRESULT_FROM_WIN32(GetLastError()),
			L"CreateWindow failed"
		);
		return 0;
	}
	ShowWindow(hWnd, show);
	return hWnd;
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	hInst = hInstance;
	if (webview2_runtime_check(
		szTitle,
		L"WebView2 ランタイムがありません",
		LR"MSG(プログラムの実行には WebView2 ランタイムが必要です。

<a href="https://go.microsoft.com/fwlink/p/?LinkId=2124703">https://go.microsoft.com/fwlink/p/?LinkId=2124703</a>

このリンクからランタイムをインストールしてください。
)MSG"
)) {
		return 1;
	}

	HWND hWnd = create_window(nCmdShow);
	if (!hWnd) {
		MessageBox(NULL, _T("ウィンドウの作成に失敗しました。"), szTitle, MB_ICONERROR);
		return 1;
	}

	if (report(
		CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
			Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
				[hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

		if (report(
			env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
				[hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
			if (controller == nullptr) {
				return E_FAIL;
			}
			webview_controller = controller;
			if (report(
				webview_controller->get_CoreWebView2(&webview),
				L"ICoreWebView2Controller::get_CoreWebView2 failed"
			)) {
				return E_FAIL;
			}

			if (report(
				webview->CallDevToolsProtocolMethod(
					L"Emulation.setDefaultBackgroundColorOverride",
					LR"JSON({"color":{"r":0,"g":0,"b":0,"a":0}})JSON",
					nullptr
				),
				L"ICoreWebView2::CallDevToolsProtocolMethod failed"
			)) {
				return E_FAIL;
			}
			{
				ComPtr<ICoreWebView2Settings> Settings;
				if (report(
					webview->get_Settings(&Settings),
					L"ICoreWebView2::get_Settings failed"
				)) {
					return E_FAIL;
				}
				report(
					Settings->put_IsScriptEnabled(TRUE),
					L"ICoreWebView2Settings::put_IsScriptEnabled failed"
				);
				report(
					Settings->put_AreDefaultScriptDialogsEnabled(TRUE),
					L"ICoreWebView2Settings::put_AreDefaultScriptDialogsEnabled failed"
				);
				report(
					Settings->put_IsWebMessageEnabled(TRUE),
					L"ICoreWebView2Settings::put_IsWebMessageEnabled failed"
				);
				report(
					Settings->put_IsZoomControlEnabled(FALSE),
					L"ICoreWebView2Settings::put_IsZoomControlEnabled failed"
				);
				report(
					Settings->put_IsBuiltInErrorPageEnabled(FALSE),
					L"ICoreWebView2Settings::put_IsBuiltInErrorPageEnabled failed"
				);
				report(
					Settings->put_AreDefaultContextMenusEnabled(TRUE),
					L"ICoreWebView2Settings::put_AreDefaultContextMenusEnabled failed"
				);
				report(
#ifdef FCC_DEVTOOL
					Settings->put_AreDevToolsEnabled(TRUE),
#else
					Settings->put_AreDevToolsEnabled(FALSE),
#endif
					L"ICoreWebView2Settings::put_AreDevToolsEnabled failed"
				);
			}

			{
				RECT bounds;
				if (GetClientRect(hWnd, &bounds)) {
					report(
						webview_controller->put_Bounds(bounds),
						L"ICoreWebView2Controller::put_Bounds failed"
					);
				}
				else {
					report(
						HRESULT_FROM_WIN32(GetLastError()),
						L"GetClientRect failed"
					);
				}
			}

			if (report(
				webview->Navigate(L"https://coefont.studio/editor"),
				L"ICoreWebView2::Navigate failed"
			)) {
				return E_FAIL;
			}

			EventRegistrationToken nwrToken;
			if (report(
				webview->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
					[hWnd](ICoreWebView2* webview, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
				PWSTR uri = nullptr;
				{
					const HRESULT hr = args->get_Uri(&uri);
					if (FAILED(hr)) {
						OutputDebugString(_T("ICoreWebView2NewWindowRequestedEventArgs::get_Uri failed"));
						return E_FAIL;
					}
				}
				if (ShellExecuteW(hWnd, L"open", uri, NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE)32) {
					OutputDebugString(_T("[WARN] ShellExecuteW failed"));
				}
				CoTaskMemFree(uri);
				args->put_Handled(true);
				return S_OK;
			}).Get(), &nwrToken), L"ICoreWebView2::add_NewWindowRequested failed"
			)) {
				return E_FAIL;
			}

			if (report(api.install(webview), L"internal API install failed")) {
				return E_FAIL;
			}

			return S_OK;
		}).Get()), L"ICoreWebView2Environment::CreateCoreWebView2Controller failed")) {
			return E_FAIL;
		}
		return S_OK;
	}).Get()), L"CreateCoreWebView2EnvironmentWithOptions failed")) {
		return E_FAIL;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

static LRESULT CALLBACK window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		dark_mode.set(hWnd);
		api.set_window(hWnd);
		break;
	case WM_DPICHANGED:
	{
		RECT* const r = (RECT*)lParam;
		SetWindowPos(hWnd, NULL, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
		break;
	}
	case WM_SETTINGCHANGE:
		dark_mode.set(hWnd);
		break;
	case WM_SIZE:
		if (webview_controller != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			webview_controller->put_Bounds(bounds);
		};
		return 0;
	case WM_DESTROY:
		dark_mode.cleanup(hWnd);
		PostQuitMessage(0);
		return 0;
	case WM_APP + 0x2525:
		api.pump();
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
