/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
