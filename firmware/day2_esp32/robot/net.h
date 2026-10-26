// ======================================================================
//  net.h — Wi-Fi точка доступу + сторінка телеметрії (192.168.4.1)
//  Телефон підключається до «sosny-robot», браузер сам оновлює стан 2 р/с.
// ======================================================================
#pragma once
#include <Arduino.h>

void netBegin();
void netLoop();

// реалізовані в robot.ino
String telemetryJson();
void cmdStart();
void cmdStop(const char *why);
void cmdTarget(int n);
