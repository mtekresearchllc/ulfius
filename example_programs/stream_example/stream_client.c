/**
 * 
 * Ulfius Framework example program
 * 
 * This example program get a stream data from the server
 * 
 * Copyright 2016-2022 Nicolas Mora <mail@babelouest.org>
 * 
 * License MIT
 *
 */

#include <stdio.h>
#include <string.h>
#include <ulfius.h>

#include "u_example.h"

#if defined(U_DISABLE_CURL)
#error You must build ulfius with libcurl support enabled to compile this example, check the install documentation
#else

#define URL "http://localhost:7876/stream"

size_t my_write_body(void * contents, size_t size, size_t nmemb, void * user_data) {
  (void)(user_data);
  printf("got %s", (char *)contents);
  
  return size * nmemb;
}

size_t my_write_meta_body(void * contents, size_t size, size_t nmemb, void * user_data) {
  (void)(contents);
  (void)(user_data);
  printf("got %zu %zu\n", size, nmemb);
  
  return size * nmemb;
}

int main(void) {
  struct _u_request request;
  struct _u_response response;
  int res;
  
  ulfius_set_log_callback(ULFIUS_LOG_DEBUG, u_example_log_callback, stdout);
  ulfius_log_message(ULFIUS_LOG_INFO, "%s", "Starting stream_example");
  
  ulfius_init_response(&response);
  ulfius_init_request(&request);
  
  ulfius_set_request_properties(&request,
                                U_OPT_HTTP_VERB, "GET",
                                U_OPT_HTTP_URL, URL,
                                U_OPT_NONE); // Required to close the parameters list
  
  ulfius_log_message(ULFIUS_LOG_DEBUG, "Press <enter> to run stream test");
  getchar();
  res = ulfius_send_http_streaming_request(&request, &response, my_write_body, NULL);
  if (res == U_OK) {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "ulfius_send_http_streaming_request OK");
  } else {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "Error in http request: %d", res);
  }
  ulfius_clean_response(&response);
  
  ulfius_log_message(ULFIUS_LOG_DEBUG, "Press <enter> to run stream audio test");
  ulfius_init_response(&response);
  ulfius_set_request_properties(&request, U_OPT_HTTP_VERB, "GET", U_OPT_HTTP_URL, URL, U_OPT_HTTP_URL_APPEND, "/audio", U_OPT_NONE);
  
  getchar();
  res = ulfius_send_http_streaming_request(&request, &response, my_write_meta_body, NULL);
  if (res == U_OK) {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "ulfius_send_http_streaming_request OK");
  } else {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "Error in http request: %d", res);
  }
  ulfius_clean_response(&response);
  
  ulfius_log_message(ULFIUS_LOG_DEBUG, "Press <enter> to run no stream test");
  ulfius_init_response(&response);
  ulfius_set_request_properties(&request, U_OPT_HTTP_VERB, "GET", U_OPT_HTTP_URL, "http://www.w3.org/", U_OPT_NONE);

  getchar();
  res = ulfius_send_http_request(&request, &response);
  if (res == U_OK) {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "ulfius_send_http_request OK");
  } else {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "Error in http request: %d", res);
  }
  ulfius_clean_response(&response);
  
  ulfius_clean_request(&request);
  
  ulfius_set_log_callback(ULFIUS_LOG_NONE, NULL, NULL);
  
  return 0;
}
#endif
