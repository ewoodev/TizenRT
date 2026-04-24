/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <tinyara/config.h>
#include <tinyara/kmalloc.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#include "netmgr/netstack.h"
#include <tinyara/net/netlog.h>

#define TAG "[NETMGR]"

struct tr_netmgr {
	void *dev;
};

static struct tr_netmgr g_netmgr;

extern int netdev_mgr_start(void);
#ifdef CONFIG_VIRTUAL_WLAN
extern void vwifi_start(void);
#endif
#ifdef CONFIG_VIRTUAL_BLE
extern void vble_start(void);
#endif
extern int trwifi_run_handler(void);
/****************************************************************************
 * Name: net_setup
 *
 * Description:
 *   This is called from the OS initialization logic before driver bring-up
 *   to prepare the networking subsystem.  The current implementation can
 *   allocate/register the LWNL manager device, invoke the default socket
 *   netstack init hook, and then start the network-device manager.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
void net_setup(void)
{
	int res = 0;
#ifdef CONFIG_LWNL80211
	// todo
	if (!g_netmgr.dev) {
		g_netmgr.dev = (void *)kmm_zalloc(sizeof(struct lwnl_lowerhalf_s));
		if (!g_netmgr.dev) {
			NET_LOGKE(TAG, "!!!alloc dev fail!!!\n");
		}
	}

	res = lwnl_register((struct lwnl_lowerhalf_s *)g_netmgr.dev);
	if (res < 0) {
		NET_LOGKE(TAG, "!!!register device fail!!!\n");
	}
#endif

	struct netstack *stk = get_netstack(TR_SOCKET);
	NETSTACK_CALL_RET(stk, init, (NULL), res);
	if (res < 0) {
		NET_LOGKE(TAG, "!!!initialize stack fail!!!\n");
	}
	netdev_mgr_start();
}

/****************************************************************************
 * Name: net_initialize
 *
 * Description:
 *   This function completes networking bring-up after hardware facilities
 *   such as timers and interrupts are available.  The current
 *   implementation optionally starts virtual interfaces, then starts the
 *   default socket netstack and the TRWiFi event handler.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
void net_initialize(void)
{
#ifdef CONFIG_VIRTUAL_WLAN
	vwifi_start();
#endif
#ifdef CONFIG_VIRTUAL_BLE
    vble_start();
#endif
	/*  start network stack */
	struct netstack *stk = get_netstack(TR_SOCKET);
	int res = -1;
	NETSTACK_CALL_RET(stk, start, (NULL), res);
	if (res < 0) {
		NET_LOGKE(TAG, "!!!start stack fail!!!\n");
		assert(0);
	}
	if (trwifi_run_handler() != 0) {
		NET_LOGKE(TAG, "!!!start event handler fail!!!\n");
		assert(0);
	}

	return;
}
