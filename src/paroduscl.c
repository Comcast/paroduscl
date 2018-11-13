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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <semaphore.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "paroduscl.h"
#ifdef USE_RDKX_LOGGER
#include "rdkx_logger.h"
#else
#define XLOGD_INFO(FORMAT, ...) do { printf("%s:" FORMAT "\n", __FUNCTION__, ##__VA_ARGS__); } while(0)
#endif

#define PCL_SERVICE_NAME_LEN_MAX (64)
#define PCL_URL_LEN_MAX          (256)

#define PCL_SERVICE_NAME_DEFAULT "iot"
#define PCL_URL_CLIENT_DEFAULT   "tcp://127.0.0.1:6667"
#define PCL_RECV_TIMEOUT_DEFAULT (2)
#define PCL_SEND_TIMEOUT_DEFAULT (2)

#define PCL_MUTEX_LOCK()   sem_wait(&obj->semaphore)
#define PCL_MUTEX_UNLOCK() sem_post(&obj->semaphore)

typedef struct {
   int sock;
   int fd;
   int timeout;
} pcl_sock_t;

typedef struct {
   sem_t semaphore;
   bool  authorized;
   int   auth_status;
   int   service_name_len;
   char  service_name[PCL_SERVICE_NAME_LEN_MAX];
   char  url_parodus[PCL_URL_LEN_MAX];
   char  url_client[PCL_URL_LEN_MAX];
   
   pcl_sock_t        recv;
   pcl_sock_t        send;
   pcl_msg_handler_req_t   handler_request;
   pcl_msg_handler_event_t handler_event;
   pcl_msg_handler_crud_t  handler_create;
   pcl_msg_handler_crud_t  handler_retrieve;
   pcl_msg_handler_crud_t  handler_update;
   pcl_msg_handler_crud_t  handler_delete;
   pcl_msg_handler_alive_t handler_alive;
} pcl_obj_t;


static void         pcl_obj_destroy(pcl_obj_t **obj, int *errsv);
static pcl_result_t pcl_register(pcl_obj_t *obj, int *errsv);
static pcl_result_t pcl_sock_send_wrp(pcl_obj_t *obj, wrp_msg_t *msg, int *errsv);
static bool         pcl_service_name_match(pcl_obj_t *obj, const char *dest);
static pcl_result_t pcl_msg_handler_auth(pcl_obj_t *obj, struct wrp_auth_msg *msg);
static pcl_result_t pcl_msg_handler_request(struct wrp_req_msg *msg);
static pcl_result_t pcl_msg_handler_event(struct wrp_event_msg *msg);
static pcl_result_t pcl_msg_handler_create(struct wrp_crud_msg *msg);
static pcl_result_t pcl_msg_handler_retrieve(struct wrp_crud_msg *msg);
static pcl_result_t pcl_msg_handler_update(struct wrp_crud_msg *msg);
static pcl_result_t pcl_msg_handler_delete(struct wrp_crud_msg *msg);
static pcl_result_t pcl_msg_handler_register(pcl_obj_t *obj, struct wrp_svc_registration_msg *msg);
static pcl_result_t pcl_msg_handler_alive(void);

pcl_result_t pcl_init(pcl_object_t *object, int *fd_recv, int *fd_send, int *errsv, pcl_params_t *params) {
   if(object == NULL) {
      return(PCL_RESULT_ERROR_PARAMS);
   }
   int errsink;
   if(errsv == NULL) {
      errsv = &errsink;
   }
   *errsv = 0;
   
   pcl_obj_t *obj = (pcl_obj_t *)malloc(sizeof(pcl_obj_t));

   if(obj == NULL) {
      return(PCL_RESULT_ERROR_OUT_OF_MEMORY);
   }
   bzero(obj, sizeof(*obj));

   sem_init(&obj->semaphore, 0, 1);
   obj->authorized  = false;
   obj->auth_status = -1;
   obj->recv.sock   = -1;
   obj->send.sock   = -1;
   obj->recv.fd     = -1;
   obj->send.fd     = -1;
   if(params == NULL) { // use defaults
      snprintf(obj->service_name, PCL_SERVICE_NAME_LEN_MAX, "%s", PCL_SERVICE_NAME_DEFAULT);
      snprintf(obj->url_parodus,  PCL_URL_LEN_MAX,          "%s", PCL_URL_PARODUS_DEFAULT);
      snprintf(obj->url_client,   PCL_URL_LEN_MAX,          "%s", PCL_URL_CLIENT_DEFAULT);
      obj->recv.timeout     = PCL_RECV_TIMEOUT_DEFAULT;
      obj->send.timeout     = PCL_SEND_TIMEOUT_DEFAULT;
      obj->handler_request  = pcl_msg_handler_request;
      obj->handler_event    = pcl_msg_handler_event;
      obj->handler_create   = pcl_msg_handler_create;
      obj->handler_retrieve = pcl_msg_handler_retrieve;
      obj->handler_update   = pcl_msg_handler_update;
      obj->handler_delete   = pcl_msg_handler_delete;
      obj->handler_alive    = pcl_msg_handler_alive;
   } else {
      snprintf(obj->service_name, PCL_SERVICE_NAME_LEN_MAX, "%s", params->service_name ? params->service_name : PCL_SERVICE_NAME_DEFAULT);
      snprintf(obj->url_parodus,  PCL_URL_LEN_MAX,          "%s", params->url_parodus  ? params->url_parodus  : PCL_URL_PARODUS_DEFAULT);
      snprintf(obj->url_client,   PCL_URL_LEN_MAX,          "%s", params->url_client   ? params->url_client   : PCL_URL_CLIENT_DEFAULT);
      obj->recv.timeout     = params->timeout_recv     ? *(params->timeout_recv)  : PCL_RECV_TIMEOUT_DEFAULT;
      obj->send.timeout     = params->timeout_send     ? *(params->timeout_send)  : PCL_SEND_TIMEOUT_DEFAULT;
      obj->handler_request  = params->handler_request  ? params->handler_request  : pcl_msg_handler_request;
      obj->handler_event    = params->handler_event    ? params->handler_event    : pcl_msg_handler_event;
      obj->handler_create   = params->handler_create   ? params->handler_create   : pcl_msg_handler_create;
      obj->handler_retrieve = params->handler_retrieve ? params->handler_retrieve : pcl_msg_handler_retrieve;
      obj->handler_update   = params->handler_update   ? params->handler_update   : pcl_msg_handler_update;
      obj->handler_delete   = params->handler_delete   ? params->handler_delete   : pcl_msg_handler_delete;
      obj->handler_alive    = params->handler_alive    ? params->handler_alive    : pcl_msg_handler_alive;
   }
   obj->service_name_len = strlen(obj->service_name);
   
   XLOGD_INFO("service name <%s> parodus <%s> client <%s>", obj->service_name, obj->url_parodus, obj->url_client);
   
   obj->recv.sock = nn_socket(AF_SP, NN_PULL);
   if(obj->recv.sock < 0) {
      *errsv = errno;
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_SOCK_RECV_CREATE);
   }
   if(obj->recv.timeout > 0) {
      obj->recv.timeout *= 1000;
      if(nn_setsockopt(obj->recv.sock, NN_SOL_SOCKET, NN_RCVTIMEO, &obj->recv.timeout, sizeof(obj->recv.timeout)) < 0) {
         *errsv = errno;
         pcl_obj_destroy(&obj, NULL);
         return(PCL_RESULT_ERROR_SOCK_RECV_SETOPT);
      }
   }
   
   if(nn_bind(obj->recv.sock, obj->url_client) < 0) {
      *errsv = errno;
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_SOCK_RECV_BIND);
   }
   size_t optvallen = sizeof(obj->recv.fd);
   if(nn_getsockopt(obj->recv.sock, NN_SOL_SOCKET, NN_RCVFD, &obj->recv.fd, &optvallen) < 0) {
      *errsv = errno;
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_SOCK_RECV_GETOPT);
   }
   
   obj->send.sock = nn_socket(AF_SP, NN_PUSH);
   if(obj->send.sock < 0) {
      *errsv = errno;
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_SOCK_SEND_CREATE);
   }
   if(nn_setsockopt(obj->send.sock, NN_SOL_SOCKET, NN_SNDTIMEO, &obj->send.timeout, sizeof(obj->send.timeout)) < 0 || optvallen != sizeof(obj->recv.fd)) {
      *errsv = errno;
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_SOCK_SEND_SETOPT);
   }
   if(nn_connect(obj->send.sock, obj->url_parodus) < 0) {
      *errsv = errno;
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_SOCK_SEND_CONNECT);
   }
   optvallen = sizeof(obj->send.fd);
   if(nn_getsockopt(obj->send.sock, NN_SOL_SOCKET, NN_SNDFD, &obj->send.fd, &optvallen) < 0 || optvallen != sizeof(obj->send.fd)) {
      *errsv = errno;
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_SOCK_SEND_GETOPT);
   }

   if(PCL_RESULT_SUCCESS != pcl_register(obj, errsv)) {
      pcl_obj_destroy(&obj, NULL);
      return(PCL_RESULT_ERROR_REGISTER);
   }
   
   if(fd_recv != NULL) {
      *fd_recv = obj->recv.fd;
   }
   if(fd_send != NULL) {
      *fd_send = obj->send.fd;
   }
   *object = obj;
   return(PCL_RESULT_SUCCESS);
}

void pcl_obj_destroy(pcl_obj_t **obj, int *errsv) {
   int errsink;
   if(errsv == NULL) {
      errsv = &errsink;
   }
   *errsv = 0;

   if(obj == NULL || *obj == NULL) {
      return;
   }
   errno = 0;
   if((*obj)->recv.sock >= 0) {
      nn_shutdown((*obj)->recv.sock, 0);
      nn_close((*obj)->recv.sock);
      (*obj)->recv.sock = -1;
   }
   if((*obj)->send.sock >= 0) {
      nn_shutdown((*obj)->send.sock, 0);
      nn_close((*obj)->send.sock);
      (*obj)->send.sock = -1;
   }
   sem_destroy(&(*obj)->semaphore);
   *errsv = errno;
   free(*obj);
   *obj = NULL;
   
}

pcl_result_t pcl_term(pcl_object_t object, int *errsv) {
   pcl_obj_t *obj = (pcl_obj_t *)object;
   int errsink;
   if(errsv == NULL) {
      errsv = &errsink;
   }
   *errsv = 0;

   PCL_MUTEX_LOCK();
   pcl_obj_destroy(&obj, errsv);
   if(*errsv) {
      return(PCL_RESULT_ERROR_INTERNAL);
   }
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_recv(pcl_object_t object, int *errsv) {
   pcl_obj_t *obj = (pcl_obj_t *)object;
   int errsink;
   if(errsv == NULL) {
      errsv = &errsink;
   }
   *errsv = 0;
   if(obj == NULL) {
      return(PCL_RESULT_ERROR_PARAMS);
   }
   
   PCL_MUTEX_LOCK();
   
   // Receive from socket
   char *     msg_buf = NULL;
   wrp_msg_t *msg_wrp = NULL;
   int msg_len = nn_recv(obj->recv.sock, &msg_buf, NN_MSG, 0);

   if(msg_len < 0 || msg_buf == NULL) {
      *errsv = errno;
      if(errno == ETIMEDOUT) {
         PCL_MUTEX_UNLOCK();
         return(PCL_RESULT_ERROR_SOCK_RECV_TIMEOUT);
      }
      PCL_MUTEX_UNLOCK();
      return(PCL_RESULT_ERROR_SOCK_RECV_READ);
   }

   // Convert bytes to wrp
   msg_len = (int) wrp_to_struct(msg_buf, msg_len, WRP_BYTES, &msg_wrp);
   nn_freemsg(msg_buf);
   msg_buf = NULL;
   
   if(msg_len < 1 || msg_wrp == NULL) {
      PCL_MUTEX_UNLOCK();
      return(PCL_RESULT_ERROR_SOCK_RECV_WRP);
   }
   PCL_MUTEX_UNLOCK();

   pcl_result_t result = PCL_RESULT_ERROR_INTERNAL;
   
   // Call handler based on message type
   switch(msg_wrp->msg_type) {
      case WRP_MSG_TYPE__AUTH: {
         result = pcl_msg_handler_auth(obj, &msg_wrp->u.auth);
         break;
      }
      case WRP_MSG_TYPE__SVC_REGISTRATION: {
         result = pcl_msg_handler_register(obj, &msg_wrp->u.reg);
         break;
      }
      case WRP_MSG_TYPE__SVC_ALIVE: {
         result = (*obj->handler_alive)();
         break;
      }
      case WRP_MSG_TYPE__REQ: {
         if(!pcl_service_name_match(obj, msg_wrp->u.req.dest)) {
            result = PCL_RESULT_ERROR_SOCK_RECV_SVCNAME;
         } else {
            result = (*obj->handler_request)(&msg_wrp->u.req);
         }
         break;
      }
      case WRP_MSG_TYPE__EVENT: {
         if(!pcl_service_name_match(obj, msg_wrp->u.event.dest)) {
            result = PCL_RESULT_ERROR_SOCK_RECV_SVCNAME;
         } else {
            result = (*obj->handler_event)(&msg_wrp->u.event);
         }
         break;
      }
      case WRP_MSG_TYPE__CREATE: {
         if(!pcl_service_name_match(obj, msg_wrp->u.crud.dest)) {
            result = PCL_RESULT_ERROR_SOCK_RECV_SVCNAME;
         } else {
            result = (*obj->handler_create)(&msg_wrp->u.crud);
         }
         break;
      }
      case WRP_MSG_TYPE__RETREIVE: {
         if(!pcl_service_name_match(obj, msg_wrp->u.crud.dest)) {
            result = PCL_RESULT_ERROR_SOCK_RECV_SVCNAME;
         } else {
            result = (*obj->handler_retrieve)(&msg_wrp->u.crud);
         }
         break;
      }
      case WRP_MSG_TYPE__UPDATE: {
         if(!pcl_service_name_match(obj, msg_wrp->u.crud.dest)) {
            result = PCL_RESULT_ERROR_SOCK_RECV_SVCNAME;
         } else {
            result = (*obj->handler_update)(&msg_wrp->u.crud);
         }
         break;
      }
      case WRP_MSG_TYPE__DELETE: {
         if(!pcl_service_name_match(obj, msg_wrp->u.crud.dest)) {
            result = PCL_RESULT_ERROR_SOCK_RECV_SVCNAME;
         } else {
            result = (*obj->handler_delete)(&msg_wrp->u.crud);
         }
         break;
      }
      case WRP_MSG_TYPE__UNKNOWN:
      default: {
         result = PCL_RESULT_ERROR_SOCK_RECV_MSGTYPE;
         break;
      }
   }
   wrp_free_struct(msg_wrp);
   
   return(result);
}

bool pcl_service_name_match(pcl_obj_t *obj, const char *dest) {
   if(strncmp("mac:", dest, 4) == 0) {
      const char *service = strchr(dest, '/');
      if(service == NULL) {
         return(false);
      }
      service++;

      //const char *mac = dest + 4;
      //printf("%s: mac <%.12s> service <%s>\n", __FUNCTION__, mac, service);

      // TODO check mac address
      
      if(strncmp(service, obj->service_name, obj->service_name_len) == 0) {
         // Make sure service name ends in /, ?, # or null termination
         if(service[obj->service_name_len] == '/' || service[obj->service_name_len] == '?' || service[obj->service_name_len] == '#' || service[obj->service_name_len] == '\0') {
            return(true);
         }
      }
   }
   return(false);
}

pcl_result_t pcl_send(pcl_object_t object, wrp_msg_t *msg, int *errsv) {
   pcl_obj_t *obj = (pcl_obj_t *)object;
   int errsink;
   if(errsv == NULL) {
      errsv = &errsink;
   }
   *errsv = 0;
   if(obj == NULL || msg == NULL) {
      return(PCL_RESULT_ERROR_PARAMS);
   }
   if(!obj->authorized) {
      return(PCL_RESULT_ERROR_SOCK_SEND_AUTH);
   }
   return(pcl_sock_send_wrp(obj, msg, errsv));
}

pcl_result_t pcl_register(pcl_obj_t *obj, int *errsv) {
   wrp_msg_t reg_msg;
   reg_msg.msg_type           = WRP_MSG_TYPE__SVC_REGISTRATION;
   reg_msg.u.reg.service_name = obj->service_name;
   reg_msg.u.reg.url          = obj->url_client;
   return(pcl_sock_send_wrp(obj, &reg_msg, errsv));
}

pcl_result_t pcl_sock_send_wrp(pcl_obj_t *obj, wrp_msg_t *msg, int *errsv) {
   void *msg_bytes = NULL;
   int errsink;
   if(errsv == NULL) {
      errsv = &errsink;
   }
   *errsv = 0;

   PCL_MUTEX_LOCK();
   ssize_t msg_len = wrp_struct_to(msg, WRP_BYTES, &msg_bytes);
   if(msg_len < 1 || msg_bytes == NULL) {
      PCL_MUTEX_UNLOCK();
      return(PCL_RESULT_ERROR_SOCK_SEND_WRP);
   }

   int ret = nn_send(obj->send.sock, (const char *)msg_bytes, msg_len, 0);
   
   PCL_MUTEX_UNLOCK();
   if(ret < 0) {
      *errsv = errno;
      free(msg_bytes);
      return(PCL_RESULT_ERROR_SOCK_SEND_WRITE);
   }
   free(msg_bytes);
   if(ret != msg_len) {
      return(PCL_RESULT_ERROR_SOCK_SEND_PARTIAL);
   }
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_auth(pcl_obj_t *obj, struct wrp_auth_msg *msg) {
   //printf("%s: status <%d>\n", __FUNCTION__, msg->status);
   if(msg->status == 200) {
      obj->authorized = true;
   } else {
      obj->authorized = false;
   }
   obj->auth_status = msg->status;
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_register(pcl_obj_t *obj, struct wrp_svc_registration_msg  *msg) {
   //printf("%s: service name <%s> url <%s>\n", __FUNCTION__, msg->service_name, msg->url);
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_alive(void) {
   //printf("%s: \n", __FUNCTION__);
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_request(struct wrp_req_msg *msg) {
   //printf("%s: \n", __FUNCTION__);
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_event(struct wrp_event_msg *msg) {
   //printf("%s: \n", __FUNCTION__);
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_create(struct wrp_crud_msg *msg) {
   //printf("%s: \n", __FUNCTION__);
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_retrieve(struct wrp_crud_msg *msg) {
   //printf("%s: \n", __FUNCTION__);
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_update(struct wrp_crud_msg *msg) {
   //printf("%s: \n", __FUNCTION__);
   return(PCL_RESULT_SUCCESS);
}

pcl_result_t pcl_msg_handler_delete(struct wrp_crud_msg *msg) {
   //printf("%s: \n", __FUNCTION__);
   return(PCL_RESULT_SUCCESS);
}
