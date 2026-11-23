# Response body compression callback function for Ulfius Framework

Compress the response body using `deflate` or `gzip` depending on the request header `Accept-Encoding` and the callback configuration. The rest of the response, status, headers, cookies won't change. After compressing response body, the response header Content-Encoding will be set accordingly.

By default, this callback will use `gzip` algorithm if possible, `deflate` otherwise.

To use this callback function, declare it in your endpoint list with the lowest priority to make sure it's called at the end of the callback functions list. This callback function won't be called if a websocket or a streaming response is declared in a previous callback.

Simple usage example:

```C
// Will call callback_http_compression for every endpoint
ulfius_add_endpoint_by_val(&instance, "*", NULL, "*", 1000, &callback_http_compression, NULL);
```

Use `callback_http_compression` for `deflate` compression only:

```C
struct _http_compression_config config;
config.allow_gzip = 0;
config.allow_deflate = 1;

ulfius_add_endpoint_by_val(&instance, "*", NULL, "*", 1000, &callback_http_compression, &config);
```

## Build support

Compile `src/u_string.c` and `src/u_token.c` from the matching Ulfius source
release alongside this callback, with `include/` on the include path. A CMake
installation also provides those sources and headers under
`share/doc/ulfius/utility_support`; Make installs only the library and public
headers. These private helpers use the library's public allocator and are not
exported from `libulfius`. Do not compile a separate `u_memory.c`. Keep any
additional dependencies documented for this callback.
