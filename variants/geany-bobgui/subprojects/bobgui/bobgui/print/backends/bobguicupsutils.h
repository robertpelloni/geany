/* bobguicupsutils.h
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>
#include <cups/cups.h>
#include <cups/language.h>
#include <cups/http.h>
#include <cups/ipp.h>

G_BEGIN_DECLS

typedef struct _BobguiCupsRequest        BobguiCupsRequest;
typedef struct _BobguiCupsResult         BobguiCupsResult;
typedef struct _BobguiCupsConnectionTest BobguiCupsConnectionTest;

typedef enum
{
  BOBGUI_CUPS_ERROR_HTTP,
  BOBGUI_CUPS_ERROR_IPP,
  BOBGUI_CUPS_ERROR_IO,
  BOBGUI_CUPS_ERROR_AUTH,
  BOBGUI_CUPS_ERROR_GENERAL
} BobguiCupsErrorType;

typedef enum
{
  BOBGUI_CUPS_POST,
  BOBGUI_CUPS_GET
} BobguiCupsRequestType;


/*
 * Direction we should be polling the http socket on.
 * We are either reading or writing at each state.
 * This makes it easy for mainloops to connect to poll.
 */
typedef enum
{
  BOBGUI_CUPS_HTTP_IDLE,
  BOBGUI_CUPS_HTTP_READ,
  BOBGUI_CUPS_HTTP_WRITE
} BobguiCupsPollState;

typedef enum
{
  BOBGUI_CUPS_CONNECTION_AVAILABLE,
  BOBGUI_CUPS_CONNECTION_NOT_AVAILABLE,
  BOBGUI_CUPS_CONNECTION_IN_PROGRESS  
} BobguiCupsConnectionState;

typedef enum
{
  BOBGUI_CUPS_PASSWORD_NONE,
  BOBGUI_CUPS_PASSWORD_REQUESTED,
  BOBGUI_CUPS_PASSWORD_HAS,
  BOBGUI_CUPS_PASSWORD_APPLIED,
  BOBGUI_CUPS_PASSWORD_NOT_VALID
} BobguiCupsPasswordState;

struct _BobguiCupsRequest 
{
  BobguiCupsRequestType type;

  http_t *http;
  http_status_t last_status;
  ipp_t *ipp_request;

  char *server;
  char *resource;
  GIOChannel *data_io;
  int attempts;

  BobguiCupsResult *result;

  int state;
  BobguiCupsPollState poll_state;
  guint64 bytes_received;

  char *password;
  char *username;

  unsigned int own_http : 1;
  unsigned int need_password : 1;
  unsigned int need_auth_info : 1;
  char **auth_info_required;
  char **auth_info;
  BobguiCupsPasswordState password_state;
};

struct _BobguiCupsConnectionTest
{
  BobguiCupsConnectionState at_init;
  http_addrlist_t       *addrlist;
  http_addrlist_t       *current_addr;
  http_addrlist_t       *last_wrong_addr;
  int                    socket;
};

#define BOBGUI_CUPS_REQUEST_START 0
#define BOBGUI_CUPS_REQUEST_DONE 500

/* POST states */
enum 
{
  BOBGUI_CUPS_POST_CONNECT = BOBGUI_CUPS_REQUEST_START,
  BOBGUI_CUPS_POST_SEND,
  BOBGUI_CUPS_POST_WRITE_REQUEST,
  BOBGUI_CUPS_POST_WRITE_DATA,
  BOBGUI_CUPS_POST_CHECK,
  BOBGUI_CUPS_POST_AUTH,
  BOBGUI_CUPS_POST_READ_RESPONSE,
  BOBGUI_CUPS_POST_DONE = BOBGUI_CUPS_REQUEST_DONE
};

/* GET states */
enum
{
  BOBGUI_CUPS_GET_CONNECT = BOBGUI_CUPS_REQUEST_START,
  BOBGUI_CUPS_GET_SEND,
  BOBGUI_CUPS_GET_CHECK,
  BOBGUI_CUPS_GET_AUTH,
  BOBGUI_CUPS_GET_READ_DATA,
  BOBGUI_CUPS_GET_DONE = BOBGUI_CUPS_REQUEST_DONE
};

BobguiCupsRequest        * bobgui_cups_request_new_with_username (http_t             *connection,
                                                            BobguiCupsRequestType  req_type,
                                                            int                 operation_id,
                                                            GIOChannel         *data_io,
                                                            const char         *server,
                                                            const char         *resource,
                                                            const char         *username);
BobguiCupsRequest        * bobgui_cups_request_new               (http_t             *connection,
                                                            BobguiCupsRequestType  req_type,
                                                            int                 operation_id,
                                                            GIOChannel         *data_io,
                                                            const char         *server,
                                                            const char         *resource);
void                    bobgui_cups_request_ipp_add_string    (BobguiCupsRequest     *request,
                                                            ipp_tag_t           group,
                                                            ipp_tag_t           tag,
                                                            const char         *name,
                                                            const char         *charset,
                                                            const char         *value);
void                    bobgui_cups_request_ipp_add_strings   (BobguiCupsRequest     *request,
                                                            ipp_tag_t           group,
                                                            ipp_tag_t           tag,
                                                            const char         *name,
                                                            int                 num_values,
                                                            const char         *charset,
                                                            const char * const *values);
const char            * bobgui_cups_request_ipp_get_string    (BobguiCupsRequest     *request,
                                                            ipp_tag_t           tag,
                                                            const char         *name);
gboolean                bobgui_cups_request_read_write        (BobguiCupsRequest     *request,
                                                            gboolean            connect_only);
BobguiCupsPollState        bobgui_cups_request_get_poll_state    (BobguiCupsRequest     *request);
void                    bobgui_cups_request_free              (BobguiCupsRequest     *request);
BobguiCupsResult         * bobgui_cups_request_get_result        (BobguiCupsRequest     *request);
gboolean                bobgui_cups_request_is_done           (BobguiCupsRequest     *request);
void                    bobgui_cups_request_encode_option     (BobguiCupsRequest     *request,
                                                            const char         *option,
                                                            const char         *value);
void                    bobgui_cups_request_set_ipp_version   (BobguiCupsRequest     *request,
                                                            int                 major,
                                                            int                 minor);
gboolean                bobgui_cups_result_is_error           (BobguiCupsResult      *result);
ipp_t                 * bobgui_cups_result_get_response       (BobguiCupsResult      *result);
BobguiCupsErrorType        bobgui_cups_result_get_error_type     (BobguiCupsResult      *result);
int                     bobgui_cups_result_get_error_status   (BobguiCupsResult      *result);
int                     bobgui_cups_result_get_error_code     (BobguiCupsResult      *result);
const char            * bobgui_cups_result_get_error_string   (BobguiCupsResult      *result);
BobguiCupsConnectionTest * bobgui_cups_connection_test_new       (const char         *server,
                                                            const int           port);
BobguiCupsConnectionState  bobgui_cups_connection_test_get_state (BobguiCupsConnectionTest *test);
void                    bobgui_cups_connection_test_free      (BobguiCupsConnectionTest *test);

G_END_DECLS

