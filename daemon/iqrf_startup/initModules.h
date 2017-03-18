#pragma once

extern void init_SimpleSerializer();
extern void init_JsonSerializer();
extern void init_MqMessaging();
extern void init_MqttMessaging();
extern void init_ClientServicePlain();

#define STATIC_INIT \
init_SimpleSerializer(); \
init_JsonSerializer(); \
init_MqMessaging(); \
init_MqttMessaging(); \
init_ClientServicePlain();
