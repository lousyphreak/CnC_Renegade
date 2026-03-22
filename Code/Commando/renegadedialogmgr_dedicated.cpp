#include "renegadedialogmgr.h"

#include "translatedb.h"

#include <algorithm>
#include <cwchar>

DialogFactoryBaseClass *FactoryArray[FACTORY_COUNT] = {};
WWUIInputClass * _TheWWUIInput = nullptr;

int MyLoadStringW(UINT str_id, LPWSTR buffer, int buffer_len)
{
	if (buffer == nullptr || buffer_len <= 0) {
		return 0;
	}

	const WCHAR * source = TRANSLATE(str_id);
	if (source == nullptr) {
		source = L"";
	}

	const std::size_t count = std::min<std::size_t>(static_cast<std::size_t>(buffer_len - 1), std::wcslen(source));
	if (count > 0) {
		std::wmemcpy(buffer, source, count);
	}
	buffer[count] = 0;
	return static_cast<int>(count);
}

void RenegadeDialogMgrClass::Initialize(void)
{
}

void RenegadeDialogMgrClass::Shutdown(void)
{
}

void RenegadeDialogMgrClass::Do_Simple_Dialog(int)
{
}

void RenegadeDialogMgrClass::Goto_Location(LOCATION)
{
}