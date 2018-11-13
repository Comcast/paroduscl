/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PARODUS_CLIENT_LIB__
#define __PARODUS_CLIENT_LIB__

#include <stdint.h>
#include <stdbool.h>
#include <wrp-c/wrp-c.h>

#define PCL_URL_PARODUS_DEFAULT  "tcp://127.0.0.1:6666"

typedef enum {
   PCL_RESULT_SUCCESS                 =  0,
   PCL_RESULT_ERROR_PARAMS            =  1,
   PCL_RESULT_ERROR_OUT_OF_MEMORY     =  2,
   PCL_RESULT_ERROR_SOCK_RECV_CREATE  =  3,
   PCL_RESULT_ERROR_SOCK_RECV_SETOPT  =  4,
   PCL_RESULT_ERROR_SOCK_RECV_GETOPT  =  5,
   PCL_RESULT_ERROR_SOCK_RECV_BIND    =  6,
   PCL_RESULT_ERROR_SOCK_RECV_TIMEOUT =  7,
   PCL_RESULT_ERROR_SOCK_RECV_READ    =  8,
   PCL_RESULT_ERROR_SOCK_RECV_WRP     =  9,
   PCL_RESULT_ERROR_SOCK_RECV_SVCNAME = 10,
   PCL_RESULT_ERROR_SOCK_RECV_MSGTYPE = 11,
   PCL_RESULT_ERROR_SOCK_RECV_CONTENT = 12,
   PCL_RESULT_ERROR_SOCK_RECV_PAYLOAD = 13,
   PCL_RESULT_ERROR_SOCK_SEND_CREATE  = 14,
   PCL_RESULT_ERROR_SOCK_SEND_SETOPT  = 15,
   PCL_RESULT_ERROR_SOCK_SEND_GETOPT  = 16,
   PCL_RESULT_ERROR_SOCK_SEND_CONNECT = 17,
   PCL_RESULT_ERROR_SOCK_SEND_WRP     = 18,
   PCL_RESULT_ERROR_SOCK_SEND_WRITE   = 19,
   PCL_RESULT_ERROR_SOCK_SEND_PARTIAL = 20,
   PCL_RESULT_ERROR_SOCK_SEND_AUTH    = 21,
   PCL_RESULT_ERROR_REGISTER          = 22,
   PCL_RESULT_ERROR_INTERNAL          = 23,
   PCL_RESULT_INVALID                 = 24,
} pcl_result_t;

typedef pcl_result_t (*pcl_msg_handler_req_t)(struct wrp_req_msg *msg);
typedef pcl_result_t (*pcl_msg_handler_event_t)(struct wrp_event_msg *msg);
typedef pcl_result_t (*pcl_msg_handler_crud_t)(struct wrp_crud_msg *msg);
typedef pcl_result_t (*pcl_msg_handler_alive_t)(void);

typedef struct {
   const char *service_name;  // NULL to use default value
   const char *url_parodus;   // NULL to use default value
   const char *url_client;    // NULL to use default value
   const int  *timeout_recv;  // in seconds.  NULL to use default value
   const int  *timeout_send;  // in seconds.  NULL to use default value
   pcl_msg_handler_req_t   handler_request;
   pcl_msg_handler_event_t handler_event;
   pcl_msg_handler_crud_t  handler_create;
   pcl_msg_handler_crud_t  handler_retrieve;
   pcl_msg_handler_crud_t  handler_update;
   pcl_msg_handler_crud_t  handler_delete;
   pcl_msg_handler_alive_t handler_alive;
} pcl_params_t;

typedef void *pcl_object_t;

#ifdef __cplusplus
extern "C" {
#endif

pcl_result_t pcl_init(pcl_object_t *object, int *fd_recv, int *fd_send, int *errsv, pcl_params_t *params);
pcl_result_t pcl_term(pcl_object_t object, int *errsv);
pcl_result_t pcl_recv(pcl_object_t object, int *errsv);
pcl_result_t pcl_send(pcl_object_t object, wrp_msg_t *msg, int *errsv);

const char *pcl_result_str(pcl_result_t result);

#ifdef __cplusplus
}
#endif

#endif
