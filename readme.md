# bgHelper

Сервер именованного канала и TCP-клиент для проекта IFC exporter.

Частично использует класс CNamedPipeServer Bruno van Dooren [MVP VC++] (каталог `cnamedps`, лицензия MIT).

## Назначение

- Принимает сообщения плагина Revit через канал `\\.\Pipe\bghelperpipe` (только локальные клиенты).
- Пересылает их в `ifc_exporter.exe` по TCP на `127.0.0.1:<tcp_port>`.
- Раз в 20 секунд проверяет расписание в INI-файле и в заданную минуту запускает Revit.
  Сообщение `START_IMMEDIATELY` запускает Revit сразу, `End of export` завершает запущенный Revit.
- Одновременно работает только один экземпляр.

## Сборка

Требуется Visual Studio 2022 (v143, MFC) и Boost 1.86 x64 с собранными библиотеками (Boost.Log).

1. Скопировать `sensitive_data.h.example` в `sensitive_data.h` и указать реальные пути.
2. Указать путь к Boost одним из способов (по убыванию приоритета):
   - свойство MSBuild `BoostRoot` (например, `msbuild bgHelper.sln /p:BoostRoot=D:\boost_1_86_0`);
   - переменная окружения `BOOST_ROOT`;
   - иначе используется `C:\MyDocs\boost_1_86_0`.

   Библиотеки ищутся в `$(BoostRoot)\lib64-msvc-14.3`, этот путь переопределяется свойством `BoostLibDir`.

Debug собирается как консольное приложение (окно консоли скрывается при запуске), Release - как приложение без консоли со статически подключённым MFC.

## Настройки INI-файла

Путь к файлу задаётся в `sensitive_data.h` относительно `%APPDATA%`.

| Секция | Ключ | Назначение |
|---|---|---|
| `Manufacturer` | `tcp_port` | порт ifc_exporter, по умолчанию 7777 |
| `ControlFlags` | `Enabled` | `true` - экспорт по расписанию разрешён |
| `ControlFlags` | `Date`, `Time` | дата `ДД.ММ.ГГГГ` и время `ЧЧ:ММ` запуска |
| `ControlFlags` | `RevitVersion` | версия Revit из 4 цифр, например `2024` |
| `ControlFlags` | `RemoteSavDir` | каталог .sav-файлов удалённого экспорта, по умолчанию `L:\99_IT\00_ifc_export\` |

## Завершение работы

Для корректного завершения необходимо установить именованное событие `Local\bghelper_stop_event`
(или нажать Ctrl+C в консоли Debug-сборки).

Обновлено 16.10.2026
