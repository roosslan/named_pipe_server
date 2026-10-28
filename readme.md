# IFC exporter background helper

Сервер именованного канала (named pipe server) и TCP-клиент для проекта IFC exporter.

- Принимает сообщения плагина Revit через канал `\\.\Pipe\bghelperpipe` (только локальные клиенты).
- Пересылает их в `ifc_exporter.exe` по TCP на `127.0.0.1:<tcp_port>`.
- Раз в 20 секунд проверяет расписание в INI-файле и в заданную минуту запускает Revit.
  Сообщение `START_IMMEDIATELY` запускает Revit сразу, `End of export` завершает запущенный Revit.
- Одновременно работает только один экземпляр.

## Сборка

1. Клонировать репозиторий вместе с submodule: `git clone --recursive <url>`.
   Для уже склонированного репозитория необходимо выполнить `git submodule update --init`.
2. Скопировать `sensitive_data.h.example` в `src/sensitive_data.h` и указать реальные пути.
3. Открыть `bgHelper.sln` и собрать конфигурацию x64.

Лог пишется в файл из sensitive_data.h с ротацией по 10 МБ (хранятся до 5 предыдущих файлов).

Debug собирается как консольное приложение (окно консоли скрывается при запуске), Release - как приложение без консоли со статически подключённым MFC.

## Настройки INI-файла

Путь к файлу задаётся в `sensitive_data.h` относительно `%APPDATA%`.

| Секция | Ключ | Назначение |
|---|---|---|
| `Manufacturer` | `tcp_port` | порт ifc_exporter, по умолчанию 6777 |
| `ControlFlags` | `Enabled` | `true` - экспорт по расписанию разрешён |
| `ControlFlags` | `Date`, `Time` | дата `ДД.ММ.ГГГГ` и время `ЧЧ:ММ` запуска |
| `ControlFlags` | `RevitVersion` | версия Revit из 4 цифр, например `2027` |
| `ControlFlags` | `RemoteSavDir` | каталог .sav-файлов удалённого экспорта |

## Завершение работы

Для корректного завершения необходимо установить именованное событие `Local\bghelper_stop_event`

Частично использует код Bruno van Dooren (лицензия MIT)
