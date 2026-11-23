/**
 * 
 * Ulfius Framework example program
 * 
 * This example program describes the multiple callback feature
 * that are available in ulfius 2.0
 * 
 * Copyright 2017-2022 Nicolas Mora <mail@babelouest.org>
 * 
 * License MIT
 *
 */

#include <sys/socket.h>
#include "u_string.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <string.h>
#include <ulfius.h>

#include "u_example.h"

#define PORT 6875
#define PREFIX "/multiple"

#define USER           "test"
#define PASSWORD       "testpassword"
#define DEFAULT_REALM  "default_realm"
#define SPECIFIC_REALM "specific_realm"

/**
 * callback functions declaration
 */
int callback_multiple_level_zero (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_multiple_level_one (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_multiple_level_two (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_multiple_level_three (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_multiple_level_one_complete (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_multiple_level_final (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_multiple_level_auth_check (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_multiple_level_auth_data (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data);

/**
 * decode a u_map into a string
 */
char * print_map(const struct _u_map * map) {
  char * line, * to_return = NULL;
  const char **keys, * value;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      value = u_map_get(map, keys[i]);
      len = snprintf(NULL, 0, "key is %s, value is %s", keys[i], value);
      line = ulfius_malloc((size_t)(len+1));
      snprintf(line, (size_t)(len+1), "key is %s, value is %s", keys[i], value);
      if (to_return != NULL) {
        len = (int)(u_strlen(to_return) + u_strlen(line) + 1);
        to_return = ulfius_realloc(to_return, (size_t)(len+1));
        if (u_strlen(to_return) > 0) {
          strcat(to_return, "\n");
        }
      } else {
        to_return = ulfius_malloc((u_strlen(line) + 1));
        to_return[0] = 0;
      }
      strcat(to_return, line);
      u_free(line);
    }
    return to_return;
  } else {
    return NULL;
  }
}

int main (void) {
  int ret;
  
  // Set the framework port number
  struct _u_instance instance;
  
  ulfius_set_log_callback(ULFIUS_LOG_DEBUG, u_example_log_callback, stdout);
  ulfius_log_message(ULFIUS_LOG_INFO, "%s", "Starting multiple_callbacks_example");
  
  if (ulfius_init_instance(&instance, PORT, NULL, DEFAULT_REALM) != U_OK) {
    ulfius_log_message(ULFIUS_LOG_ERROR, "Error ulfius_init_instance, abort");
    return(1);
  }
  
  u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  
  // Maximum body size sent by the client is 1 Kb
  instance.max_post_body_size = 1024;
  
  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "*", PREFIX, "/zero/*", 99, &callback_multiple_level_final, NULL);
  
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/zero/*", 0, &callback_multiple_level_zero, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/zero/one/*", 1, &callback_multiple_level_one, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/zero/one/two/*", 2, &callback_multiple_level_two, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/zero/one/two/three/", 3, &callback_multiple_level_three, NULL);
  
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/zero/onec/*", 1, &callback_multiple_level_one_complete, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/zero/onec/two/*", 2, &callback_multiple_level_two, NULL);
  
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/auth/*", 1, &callback_multiple_level_auth_check, NULL);
  ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/auth/*", 1, &callback_multiple_level_auth_check, NULL);
  ulfius_add_endpoint_by_val(&instance, "*", PREFIX, "/auth/data/", 2, &callback_multiple_level_auth_data, NULL);
  
  // default_endpoint declaration
  ulfius_set_default_endpoint(&instance, &callback_default, NULL);
  
  // Start the framework
  ret = ulfius_start_framework(&instance);
  
  if (ret == U_OK) {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "Start framework on port %d", instance.port);
    
    // Wait for the user to press <enter> on the console to quit the application
    getchar();
  } else {
    ulfius_log_message(ULFIUS_LOG_DEBUG, "Error starting framework");
  }
  ulfius_log_message(ULFIUS_LOG_DEBUG, "End framework");
  
  ulfius_set_log_callback(ULFIUS_LOG_NONE, NULL, NULL);
  
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  
  return 0;
}

/**
 * Callback function of level zero
 */
int callback_multiple_level_zero (const struct _u_request * request, struct _u_response * response, void * user_data) {
  (void)(request);
  (void)(user_data);
  ulfius_set_string_body_response(response, 200, "Level zero");
  response->shared_data = u_strdup("shared from level zero");
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function of level one
 */
int callback_multiple_level_one (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char old_body[response->binary_body_length + 1], * new_body;
  (void)(request);
  (void)(user_data);
  
  memcpy(old_body, response->binary_body, response->binary_body_length);
  old_body[response->binary_body_length] = '\0';
  new_body = u_printf("%s\n%s", old_body, "Level one");
  ulfius_set_string_body_response(response, 200, new_body);
  u_free(new_body);
  ulfius_log_message(ULFIUS_LOG_DEBUG, "shared_data is %s", (const char *)response->shared_data);
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function of level one with complete return
 */
int callback_multiple_level_one_complete (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char old_body[response->binary_body_length + 1], * new_body;
  (void)(request);
  (void)(user_data);
  
  memcpy(old_body, response->binary_body, response->binary_body_length);
  old_body[response->binary_body_length] = '\0';
  new_body = u_printf("%s\n%s", old_body, "Level one");
  ulfius_set_string_body_response(response, 200, new_body);
  u_free(new_body);
  ulfius_log_message(ULFIUS_LOG_DEBUG, "shared_data is %s", (const char *)response->shared_data);
  u_free(response->shared_data);
  return U_CALLBACK_COMPLETE;
}

/**
 * Callback function of level two
 */
int callback_multiple_level_two (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char old_body[response->binary_body_length + 1], * new_body;
  (void)(request);
  (void)(user_data);
  
  memcpy(old_body, response->binary_body, response->binary_body_length);
  old_body[response->binary_body_length] = '\0';
  new_body = u_printf("%s\n%s", old_body, "Level two");
  ulfius_set_string_body_response(response, 200, new_body);
  u_free(new_body);
  ulfius_log_message(ULFIUS_LOG_DEBUG, "shared_data is %s", (const char *)response->shared_data);
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function of level three
 */
int callback_multiple_level_three (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char old_body[response->binary_body_length + 1], * new_body;
  (void)(request);
  (void)(user_data);
  
  memcpy(old_body, response->binary_body, response->binary_body_length);
  old_body[response->binary_body_length] = '\0';
  new_body = u_printf("%s\n%s", old_body, "Level three");
  ulfius_set_string_body_response(response, 200, new_body);
  u_free(new_body);
  ulfius_log_message(ULFIUS_LOG_DEBUG, "shared_data is %s", (const char *)response->shared_data);
  return U_CALLBACK_CONTINUE;
}

/**
 * Final callback function called on multiple calls
 */
int callback_multiple_level_final (const struct _u_request * request, struct _u_response * response, void * user_data) {
  (void)(request);
  (void)(user_data);
  u_free(response->shared_data);
  return U_CALLBACK_COMPLETE;
}

/**
 * Check authentication
 */
int callback_multiple_level_auth_check (const struct _u_request * request, struct _u_response * response, void * user_data) {
  (void)(user_data);
  ulfius_log_message(ULFIUS_LOG_DEBUG, "basic auth user: %s", request->auth_basic_user);
  ulfius_log_message(ULFIUS_LOG_DEBUG, "basic auth password: %s", request->auth_basic_password);
  if (request->auth_basic_user != NULL && request->auth_basic_password != NULL && 
      0 == u_strcmp(request->auth_basic_user, USER) && 0 == u_strcmp(request->auth_basic_password, PASSWORD)) {
    return U_CALLBACK_CONTINUE;
  } else {
    if (0 == u_strcmp("PUT", request->http_verb)) {
      response->auth_realm = u_strdup(SPECIFIC_REALM);
    }
    ulfius_set_string_body_response(response, 401, "Error authentication");
    return U_CALLBACK_UNAUTHORIZED;
  }
}

/**
 * Send Hello World!
 */
int callback_multiple_level_auth_data (const struct _u_request * request, struct _u_response * response, void * user_data) {
  (void)(request);
  (void)(user_data);
  ulfius_set_string_body_response(response, 200, "Hello World!");
  return U_CALLBACK_CONTINUE;
}

/**
 * Default callback function called if no endpoint has a match
 */
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
  (void)(request);
  (void)(user_data);
  ulfius_set_string_body_response(response, 404, "Page not found, do what you want");
  return U_CALLBACK_CONTINUE;
}
