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
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "paroduscl.h"

#define PCL_INVALID_STR_LEN (24)

static char pcl_invalid_str[PCL_INVALID_STR_LEN];

static const char *pcl_invalid_return(int value);

const char *pcl_invalid_return(int value) {
   snprintf(pcl_invalid_str, PCL_INVALID_STR_LEN, "INVALID(%d)", value);
   pcl_invalid_str[PCL_INVALID_STR_LEN - 1] = '\0';
   return(pcl_invalid_str);
}

const char *pcl_result_str(pcl_result_t result) {
   switch(result) {
      case PCL_RESULT_SUCCESS:                 return("SUCCESS");
      case PCL_RESULT_ERROR_PARAMS:            return("ERROR_PARAMS");
      case PCL_RESULT_ERROR_OUT_OF_MEMORY:     return("ERROR_OUT_OF_MEMORY");
      case PCL_RESULT_ERROR_SOCK_RECV_CREATE:  return("ERROR_SOCK_RECV_CREATE");
      case PCL_RESULT_ERROR_SOCK_RECV_SETOPT:  return("ERROR_SOCK_RECV_SETOPT");
      case PCL_RESULT_ERROR_SOCK_RECV_GETOPT:  return("ERROR_SOCK_RECV_GETOPT");
      case PCL_RESULT_ERROR_SOCK_RECV_BIND:    return("ERROR_SOCK_RECV_BIND");
      case PCL_RESULT_ERROR_SOCK_RECV_TIMEOUT: return("ERROR_SOCK_RECV_TIMEOUT");
      case PCL_RESULT_ERROR_SOCK_RECV_READ:    return("ERROR_SOCK_RECV_READ");
      case PCL_RESULT_ERROR_SOCK_RECV_WRP:     return("ERROR_SOCK_RECV_WRP");
      case PCL_RESULT_ERROR_SOCK_RECV_SVCNAME: return("ERROR_SOCK_RECV_SVCNAME");
      case PCL_RESULT_ERROR_SOCK_RECV_MSGTYPE: return("ERROR_SOCK_RECV_MSGTYPE");
      case PCL_RESULT_ERROR_SOCK_RECV_CONTENT: return("ERROR_SOCK_RECV_CONTENT");
      case PCL_RESULT_ERROR_SOCK_RECV_PAYLOAD: return("ERROR_SOCK_RECV_PAYLOAD");
      case PCL_RESULT_ERROR_SOCK_SEND_CREATE:  return("ERROR_SOCK_SEND_CREATE");
      case PCL_RESULT_ERROR_SOCK_SEND_SETOPT:  return("ERROR_SOCK_SEND_SETOPT");
      case PCL_RESULT_ERROR_SOCK_SEND_GETOPT:  return("ERROR_SOCK_SEND_GETOPT");
      case PCL_RESULT_ERROR_SOCK_SEND_CONNECT: return("ERROR_SOCK_SEND_CONNECT");
      case PCL_RESULT_ERROR_SOCK_SEND_WRP:     return("ERROR_SOCK_SEND_WRP");
      case PCL_RESULT_ERROR_SOCK_SEND_WRITE:   return("ERROR_SOCK_SEND_WRITE");
      case PCL_RESULT_ERROR_SOCK_SEND_PARTIAL: return("ERROR_SOCK_SEND_PARTIAL");
      case PCL_RESULT_ERROR_SOCK_SEND_AUTH:    return("ERROR_SOCK_SEND_AUTH");
      case PCL_RESULT_ERROR_REGISTER:          return("ERROR_REGISTER");
      case PCL_RESULT_ERROR_INTERNAL:          return("ERROR_INTERNAL");
      case PCL_RESULT_INVALID:                 return("INVALID");
   }
   return(pcl_invalid_return(result));
}
