# Хоткеи на иконках CommandSet

Буквы на иконках помогают увидеть клавиши юнитов, апгрейдов и способностей без наведения курсора. Панель рисует букву в правом верхнем углу на тёмной подложке поверх затемнения и таймера перезарядки. Для недоступной команды буква серая.

В Zero Hour галочка **Дополнительно → Показывать хоткеи** включена по умолчанию. Английское название — **Extra Options → Show Hotkeys**. Кнопка **Принять** сразу применяет выбор и сохраняет `ShowCommandHotkeys = 1` или `0` в пользовательском `Options.ini`. Кнопка **По умолчанию** снова ставит галочку, а **Отмена** закрывает меню без применения выбора.

## Файлы форка и мода

- `Core/GameEngine/Include/GameClient/GameWindow.h` задаёт флаг `WIN_STATUS_COMMAND_HOTKEY`. `Core/GameEngine/Source/GameClient/GUI/ControlBar/ControlBar.cpp` устанавливает его на кнопках CommandSet, извлекает клавишу из локализованного `TextLabel` по маркеру `&` и передаёт её в отрисовку в верхнем регистре.
- `Core/GameEngine/Include/GameClient/HotKey.h` и `Core/GameEngine/Source/GameClient/MessageStream/HotKey.cpp` возвращают результат регистрации. Если несколько кнопок претендуют на одну клавишу, букву получает кнопка с успешной регистрацией.
- `Core/GameEngineDevice/Source/W3DDevice/GameClient/GUI/Gadget/W3DPushButton.cpp` рисует букву и подложку. `GeneralsMD/Code/GameEngine/Include/Common/GlobalData.h` и `GeneralsMD/Code/GameEngine/Source/Common/GlobalData.cpp` задают значение по умолчанию и читают `Options.ini`. `GeneralsMD/Code/GameEngine/Source/GameClient/GUI/GUICallbacks/Menus/OptionsMenu.cpp` связывает настройку с галочкой.
- В `Contra Invasion 2023 Online RC/Window/ControlBar.wnd` все 18 слотов `ButtonCommand01`–`ButtonCommand18` используют Arial 16 pt Bold. `Window/Menus/OptionsMenu.wnd` добавляет `OptionsMenu.wnd:CheckShowCommandHotkeys` с текстовым ключом `GUI:ShowCommandHotkeys`.
- В `Contra Invasion 2023 Online RC/Data/English/generals.csf` ключ `GUI:ShowCommandHotkeys` содержит `Show Hotkeys`, а в `generals_ru.csf` — `Показывать хоткеи`. Обе строки хранятся в штатном формате CSF.

## Воспроизведение

Из корня проекта Invasion запустите скрипт оформления и сборку форка.

```powershell
.\GeneralsGameCode\scripts\Apply-InvasionCommandHotkeys.ps1 -ModDirectory '.\Contra Invasion 2023 Online RC'
.\Tools\Build-Fork.ps1
```

Первый скрипт использует распакованные WND и CSF мода. Он сохраняет копии изменяемых файлов в `Backups/CommandSetHotkeyAssets-*`. Скрипт сборки устанавливает `z_generals` как `generals.exe` в рабочую папку мода.

1. Откройте одиночную схватку и выберите объект с командами строительства, исследования или применения способности. Проверьте буквы на иконках. В данных мода хоткей «Хамви» — `V`, а апгрейда TOW — `T`.
2. В матче снимите галочку и нажмите **Принять**. Буквы должны исчезнуть, а команды по клавишам выполняться. Перезапустите игру и проверьте сохранённое состояние. Повторите с включённой галочкой и проверьте подпись пункта при запуске с каждой локализацией.

## Восстановление предыдущего состояния

Закройте игру. Для отката только этой функции отмените изменения перечисленных исходников, верните Arial 8 pt без полужирного начертания для слотов `ButtonCommand`, удалите блок `CheckShowCommandHotkeys` из меню и ключ `GUI:ShowCommandHotkeys` из обоих CSF. Удалите `ShowCommandHotkeys` из пользовательского `Options.ini` и пересоберите форк через `.\Tools\Build-Fork.ps1`.

В `Backups` в корне проекта лежат копии с контрольными суммами до каждого этапа. При наличии более поздних изменений в общих файлах используйте эти копии для сравнения и перенесите только строки этой функции.

- `CommandSetHotkeyOption-20261126-054105Z` — исходники, меню, оба CSF и EXE перед добавлением галочки.
- `CommandSetHotkeyFont-20261126-053138Z` — `ControlBar.wnd` перед увеличением шрифта.
- `CommandSetHotkeys-20261126-052604Z` — исходники и EXE до появления букв на иконках.
