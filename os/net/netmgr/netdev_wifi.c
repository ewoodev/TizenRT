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
#include <stdint.h>
#include <errno.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <debug.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/net/if/wifi.h>
#include <tinyara/net/if/ethernet.h>
#include <tinyara/netmgr/netdev_mgr.h>
#include "netdev_mgr_internal.h"
#include <tinyara/net/netlog.h>

#define TRWIFI_CALL(res, dev, method, param)	\
	do {										\
		if (dev->t_ops.wl->method) {			\
			res = (dev->t_ops.wl->method)param;	\
		}										\
	} while (0)

#define LOCK_EVTQUEUE(queue)								\
	do {													\
		if (sem_wait(&queue.lock) != 0) {					\
			int derr_no = get_errno();						\
			if (derr_no == EINTR) {							\
				continue;									\
			} else {										\
				NET_LOGKE(TAG, "lock fail %d\n", derr_no);	\
				assert(0);									\
			}												\
		}													\
	} while (0)

#define UNLOCK_EVTQUEUE(queue)							\
	do {												\
		if (sem_post(&queue.lock) != 0) {				\
			NET_LOGKE(TAG, "unlock fail %d\n", errno);	\
			assert(0);									\
		}												\
	} while (0)

#define WAIT_EVTQUEUE(queue)										\
	do {															\
		if (sem_wait(&queue.signal) != 0) {							\
			int derr_no = get_errno();								\
			if (derr_no == EINTR) {									\
				continue;											\
			} else {												\
				NET_LOGKE(TAG, "wait signal error %d\n", derr_no);	\
				assert(0);											\
			}														\
		}															\
	} while (0)

#define SIGNAL_EVTQUEUE(queue)								\
	do {													\
		if (sem_post(&queue.signal) != 0) {					\
			NET_LOGKE(TAG, "send signal fail %d\n", errno);	\
			assert(0);										\
		}													\
	} while (0)

#define CONTAINER_OF(ptr, type, member)							\
	((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define TRWIFI_GET_EVT(ptr)						\
	CONTAINER_OF(ptr, trwifi_evt, entry)

#define NETMGR_EVTHANDLER_PRIO 100
#define NETMGR_EVTHANDLER_STACKSIZE 2048
#define TAG "[NETMGR]"

typedef struct {
	sem_t lock;
	sem_t signal;
	sq_queue_t queue;
} trwifi_evt_queue;

typedef struct {
	uint32_t evt;
	void *dev;
	void *buf;
	int32_t buf_len;
	sq_entry_t entry;
} trwifi_evt;

static trwifi_evt_queue g_queue;

// this function have to succeed. otherwise it'd be better to reset.
int _trwifi_handle_event(struct netdev *dev, lwnl_cb_wifi evt, void *buffer, uint32_t buf_len)
{
	if (!dev) {
		NET_LOGKE(TAG, "invalid parameter dev\n");
		return -1;
	}
	if (evt == LWNL_EVT_STA_CONNECTED) {
		return ND_NETOPS(dev, softup(dev));
	} else if (evt == LWNL_EVT_STA_DISCONNECTED) {
		return ND_NETOPS(dev, softdown(dev));
	}
	return 0;
}

// this function have to succeed. other it'd be better to reset.
int _trwifi_handle_command(struct netdev *dev, lwnl_req cmd)
{
	if (!dev) {
		NET_LOGKE(TAG, "invalid parameter dev\n");
		return -1;
	}
	switch (cmd.type) {
	case LWNL_REQ_WIFI_STARTSOFTAP: {
		return ND_NETOPS(dev, softup(dev));
	}
	case LWNL_REQ_WIFI_STOPSOFTAP: {
		return ND_NETOPS(dev, softdown(dev));
	}
	case LWNL_REQ_WIFI_INIT: {
		return ND_NETOPS(dev, ifup(dev));
	}
	case LWNL_REQ_WIFI_DEINIT: {
		return ND_NETOPS(dev, ifdown(dev));
	}
	default:
		break;
	}
	return 0;
}

/* it's for reloading operation in binary manager.
 * it must be used to binary manager only.
 */
trwifi_result_e netdev_deinit_wifi(void)
{
	struct netdev *dev = (struct netdev *)nm_get_netdev((uint8_t *)"wlan0");
	if (!dev) {
		return TRWIFI_FAIL;
	}
	trwifi_result_e res = TRWIFI_FAIL;
	TRWIFI_CALL(res, dev, deinit, (dev));
	return res;
}

int netdev_handle_wifi(struct netdev *dev, lwnl_req cmd, void *data, uint32_t data_len)
{
	trwifi_result_e res = TRWIFI_FAIL;

	NET_LOGKV(TAG, "T%d cmd(%d) (%p) (%d)\n", getpid(), cmd.type, data, data_len);
	switch (cmd.type) {
	case LWNL_REQ_WIFI_INIT:
	{
		TRWIFI_CALL(res, dev, init, (dev));
	}
	break;
	case LWNL_REQ_WIFI_DEINIT:
	{
		TRWIFI_CALL(res, dev, deinit, (dev));
	}
	break;
	case LWNL_REQ_WIFI_GETINFO:
	{
		TRWIFI_CALL(res, dev, get_info, (dev, (trwifi_info *)data));
	}
	break;
	case LWNL_REQ_WIFI_SETAUTOCONNECT:
	{
		TRWIFI_CALL(res, dev, set_autoconnect, (dev, *((uint8_t *)data)));
	}
	break;
	case LWNL_REQ_WIFI_STARTSTA:
	{
		TRWIFI_CALL(res, dev, start_sta, (dev));
	}
	break;
	case LWNL_REQ_WIFI_CONNECTAP:
	{
		TRWIFI_CALL(res, dev, connect_ap, (dev, (trwifi_ap_config_s*)data, NULL));
	}
	break;
	case LWNL_REQ_WIFI_DISCONNECTAP:
	{
		TRWIFI_CALL(res, dev, disconnect_ap, (dev, NULL));
	}
	break;
	case LWNL_REQ_WIFI_STARTSOFTAP:
	{
		TRWIFI_CALL(res, dev, start_softap, (dev, (trwifi_softap_config_s *)data));
	}
	break;
	case LWNL_REQ_WIFI_STOPSOFTAP:
	{
		TRWIFI_CALL(res, dev, stop_softap, (dev));
	}
	break;
	case LWNL_REQ_WIFI_SCANAP:
	{
		TRWIFI_CALL(res, dev, scan_ap, (dev, (trwifi_scan_config_s*)data));
	}
	break;
	case LWNL_REQ_WIFI_IOCTL:
	{
		TRWIFI_CALL(res, dev, drv_ioctl, (dev, (trwifi_msg_s *)data));
	}
	break;
	case LWNL_REQ_WIFI_SCAN_MULTI_APS:
	{
		TRWIFI_CALL(res, dev, scan_multi_aps, (dev, (trwifi_scan_multi_configs_s *)data));
	}
	break;
	case LWNL_REQ_WIFI_SET_CHANNEL_PLAN:
	{
		TRWIFI_CALL(res, dev, set_channel_plan, (dev, *((uint8_t *)data)));
	}
	break;
	case LWNL_REQ_WIFI_GET_SIGNAL_QUALITY:
	{
		TRWIFI_CALL(res, dev, get_signal_quality, (dev, (trwifi_signal_quality *)data));
	}
	break;
	case LWNL_REQ_WIFI_GET_DISCONNECT_REASON:
	{
		TRWIFI_CALL(res, dev, get_deauth_reason, (dev, (int *)data));
	}
	break;
	case LWNL_REQ_WIFI_GET_DRIVER_INFO:
	{
		TRWIFI_CALL(res, dev, get_driver_info, (dev, (trwifi_driver_info *)data));
	}
	break;
	case LWNL_REQ_WIFI_GET_WPA_SUPPLICANT_STATE:
	{
		TRWIFI_CALL(res, dev, get_wpa_supplicant_state, (dev, (trwifi_wpa_states *)data));
	}
	break;
#if defined(CONFIG_ENABLE_HOMELYNK) && (CONFIG_ENABLE_HOMELYNK == 1)
	case LWNL_REQ_WIFI_SETBRIDGE:
	{
		lwip_set_bridge_mode(*((uint8_t *)data));
		TRWIFI_CALL(res, dev, set_bridge, (dev, *((uint8_t *)data)));
	}
	break;
#endif
	default:
		break;
	}
	if (res == TRWIFI_SUCCESS) {
		if (_trwifi_handle_command(dev, cmd) < 0) {
			// if network stack is not enabled. it needs to be restart
			NET_LOGKE(TAG, "critical error network stack is not enabled\n");
			assert(0);
		}
	}
	return res;
}

int trwifi_serialize_scaninfo(uint8_t **buffer, trwifi_scan_list_s *scan_list)
{
	trwifi_scan_list_s *item = scan_list;
	int32_t cnt = 0;
	int32_t total = 0;

	if (item == NULL) {
		return 0;
	}

	while (item) {
		item = item->next;
		cnt++;
	}
	total = cnt * sizeof(trwifi_ap_scan_info_s);
	uint32_t item_size = sizeof(trwifi_ap_scan_info_s);
	NET_LOGKV(TAG, "total size(%d) (%d) \n", sizeof(trwifi_ap_scan_info_s), total);

	*buffer = (uint8_t *)kmm_malloc(total);
	if (!(*buffer)) {
		NET_LOGKE(TAG, "malloc fail %d\n", total);
		return -1;
	}

	item = scan_list;
	cnt = 0;
	while (item) {
		memcpy(*buffer + (item_size * cnt), &item->ap_info, item_size);
		item = item->next;
		cnt++;
	}
	return total;
}

int trwifi_post_event(struct netdev *dev, lwnl_cb_wifi evt, void *buffer, uint32_t buf_len)
{
	trwifi_evt *msg = (trwifi_evt *)kmm_zalloc(sizeof(trwifi_evt));
	if (!msg) {
		return -1;
	}
	msg->evt = evt;
	msg->dev = dev;
	if (buffer) {
		msg->buf = (void *)kmm_malloc(buf_len);
		if (!msg->buf) {
			return -1;
		}
		memcpy(msg->buf, buffer, buf_len);
		msg->buf_len = buf_len;
	}
	LOCK_EVTQUEUE(g_queue);
	sq_addlast(&msg->entry, &g_queue.queue);
	UNLOCK_EVTQUEUE(g_queue);
	SIGNAL_EVTQUEUE(g_queue);
	return 0;
}

void *_trwifi_event_handler(void *arg)
{
	while (1) {
		WAIT_EVTQUEUE(g_queue);
		LOCK_EVTQUEUE(g_queue);
		trwifi_evt *evt = TRWIFI_GET_EVT(sq_remfirst(&g_queue.queue));
		UNLOCK_EVTQUEUE(g_queue);
		int res = _trwifi_handle_event(evt->dev, evt->evt, evt->buf, evt->buf_len);
		if (res < 0) {
			// if network stack is not enabled. it needs to be restart
			NET_LOGKE(TAG, "critical error network stack is not enabled\n");
			assert(0);
		}

		res = lwnl_postmsg(LWNL_DEV_WIFI, evt->evt, evt->buf, evt->buf_len);
		if (res < 0) {
			NET_LOGKE(TAG, "critical error network stack is not enabled\n");
			assert(0);
		}
		if (evt->buf) {
			kmm_free(evt->buf);
			evt->buf_len = 0;
		}
		kmm_free(evt);
	}

	NET_LOGKE(TAG, "critical error\n", errno);
	return NULL;
}

int trwifi_run_handler(void)
{
	if (sem_init(&g_queue.lock, 0, 1) != 0) {
		NET_LOGKE(TAG, "init lock fail %d\n", errno);
		return -1;
	}
	if (sem_init(&g_queue.signal, 0, 0) != 0) {
		NET_LOGKE(TAG, "init signal fail %d\n", errno);
		return -1;
	}

	sys_thread_t tid;
	tid = kernel_thread("netmgr_event_handler", NETMGR_EVTHANDLER_PRIO,
						NETMGR_EVTHANDLER_STACKSIZE, _trwifi_event_handler, NULL);
	if (tid == ERROR) {
		NET_LOGKE(TAG, "critical error %d\n", errno);
		return -2;
	}
	return 0;
}
