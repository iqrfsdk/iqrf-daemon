#pragma once

extern void init_JsonSerializer();
extern void init_MqttMessaging();
extern void init_ClientServicePm();

#define STATIC_INIT \
init_JsonSerializer(); \
init_MqttMessaging(); \
init_ClientServicePm();
