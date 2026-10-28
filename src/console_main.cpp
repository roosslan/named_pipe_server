/* Точка входа для подсистемы Console (конфигурация Debug).
 * Передаёт управление в WinMain из MFC, поэтому приложение запускается одинаково
 * в обеих конфигурациях. В подсистеме Windows (Release) main не используется.
 * Вся логика - в CBgHelperSrv::InitInstance, сюда её не добавлять. */

#include "stdafx.h"
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

#ifdef UNICODE
#error "Проект собирается в многобайтовой кодировке; для UNICODE нужен wWinMain"
#endif

int main()
{
    return WinMain(GetModuleHandleA(nullptr), nullptr, PathGetArgsA(GetCommandLineA()), SW_HIDE);
}
