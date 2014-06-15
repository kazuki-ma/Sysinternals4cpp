#include <string>
#include <vector>
#include <algorithm>
#include <cstdarg>
#include <Windows.h>

const unsigned int FILE_DEVICE_PROCMON_LOG = 0x00009535;
const unsigned int IOCTL_EXTERNAL_LOG_DEBUGOUT = CTL_CODE(FILE_DEVICE_PROCMON_LOG, 0x81, METHOD_BUFFERED, FILE_WRITE_ACCESS);
const size_t DEBUGGER_OUT_MAX_LENGTH = 2048;

class ProcessMonitorLogger {
public:
	ProcessMonitorLogger()
		: hDevice(NULL), firstError(false)
	{
		this->openDevice();
	}

	virtual ~ProcessMonitorLogger() {
		this->closeDevice();
	}

	inline int log(const char* const format, va_list args) {
		std::vector<char> message(DEBUGGER_OUT_MAX_LENGTH + 1);

		_snprintf_s(&message[0], DEBUGGER_OUT_MAX_LENGTH, _TRUNCATE, format, args);

		return this->log(&message[0]);
	}

	inline int log(const wchar_t* const format, va_list args) {
		std::vector<wchar_t> message(DEBUGGER_OUT_MAX_LENGTH + 1);

		// secure version of wsprintf
		_snwprintf_s(&message[0], DEBUGGER_OUT_MAX_LENGTH, _TRUNCATE, format, args);

		return this->log(&message[0]);
	}

	inline int log(const std::string message) const {
		return this->log(message.c_str());
	}

	inline int log(const char* const message) const {
		std::vector<wchar_t> messageW(DEBUGGER_OUT_MAX_LENGTH + 1);
		size_t numOfCharConverted = 0;
		mbstowcs_s(&numOfCharConverted, &messageW[0], DEBUGGER_OUT_MAX_LENGTH, message, _TRUNCATE);
		return this->log(&messageW[0]);
	}

	inline int log(const std::wstring message) const {
		return this->log(message.c_str());
	}

	inline int log(const wchar_t* const message) const {
		DWORD nb = 0;
		const int messagelen = static_cast<int>(std::min<size_t>(
			DEBUGGER_OUT_MAX_LENGTH,
			wcslen(message) * sizeof(wchar_t)
			));

		BOOL ok = DeviceIoControl(hDevice, IOCTL_EXTERNAL_LOG_DEBUGOUT,
			const_cast<wchar_t*>(message), messagelen, NULL, 0, &nb, NULL);

		return ok;
	}

	boolean isOpened() const {
		return (hDevice != NULL);
	}

private:
	HANDLE hDevice;
	mutable	boolean firstError;

	int openDevice(boolean reopen = false) {
		if (hDevice && reopen) {
			this->closeDevice();
		}

		if (hDevice) {
			// device already opened;
			return 0;
		}

		hDevice = ::CreateFileA("\\\\.\\Global\\ProcmonDebugLogger",
			GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		return 0;
	}

	int closeDevice() {
		if (::CloseHandle(hDevice)) {
			hDevice = NULL;
			return true;
		}
		return false;
	}
};