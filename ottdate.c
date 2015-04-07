/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 *
 * This program fetches HTTP URLs.
 */

#include "fossa.h"

static int s_exit_flag = 0;
static int s_show_headers = 0;
static const char *s_show_headers_opt = "--show-headers";

static void process_data( const char *json, int json_len )
{
    struct json_token *arr, *tok;

		// Tokenize json string, fill in tokens array
		arr = parse_json2(json, strlen(json));

		tok = find_json_token(arr, "update_available");
		if(tok!=NULL) {
			fprintf(stderr,"update_available= [%.*s]\n", tok->len, tok->ptr);
		}
		tok = find_json_token(arr, "url");
		if(tok!=NULL) {
			fprintf(stderr,"url= [%.*s]\n", tok->len, tok->ptr);
		}
		tok = find_json_token(arr, "size");
		if(tok!=NULL) {
			fprintf(stderr,"size= [%d]\n", atoi(tok->ptr));
		}
		tok = find_json_token(arr, "checksum");
		if(tok!=NULL) {
			fprintf(stderr,"checksum= [%.*s]\n", tok->len, tok->ptr);
		}
		tok = find_json_token(arr, "new_version");
		if(tok!=NULL) {
			fprintf(stderr,"new_version= [%.*s]\n", tok->len, tok->ptr);
		}

		// Do not forget to free allocated tokens array
		free(arr);
}

static void ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case NS_CONNECT:
      if (* (int *) ev_data != 0) {
        fprintf(stderr, "connect() failed: %s\n", strerror(* (int *) ev_data));
        s_exit_flag = 1;
      }
      break;
    case NS_HTTP_REPLY:
			fprintf(stderr,"\nNS_HTTP_REPLY\n");
      nc->flags |= NSF_CLOSE_IMMEDIATELY;

			struct ns_str* content=ns_get_http_header(hm,"Content-Type");
			if( content ) {	
				char *content_type="application/json";
				if( !strncmp(content_type,content->p, strlen(content_type)) )
					process_data(hm->body.p, hm->body.len);
			}

      putchar('\n');
      s_exit_flag = 1;
      break;
		case NS_RECV:
			fprintf(stderr,".");
      break;

    default:
      break;
  }
}

int main(int argc, char *argv[])
{
  struct ns_mgr mgr;
  int i;

  ns_mgr_init(&mgr, NULL);

  /* Process command line arguments */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], s_show_headers_opt) == 0) {
      s_show_headers = 1;
    } else if (strcmp(argv[i], "--hexdump") == 0 && i + 1 < argc) {
      mgr.hexdump_file = argv[++i];
    } else {
      break;
    }
  }

  if (i + 1 != argc) {
    fprintf(stderr, "Usage: %s [%s] [--hexdump <file>] <URL>\n",
            argv[0], s_show_headers_opt);
    exit(EXIT_FAILURE);
  }

  ns_connect_http(&mgr, ev_handler, argv[i], NULL, NULL);

  while (s_exit_flag == 0) {
    ns_mgr_poll(&mgr, 1000);
  }
  ns_mgr_free(&mgr);

  return 0;
}
