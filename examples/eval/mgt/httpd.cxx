#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <myService/MyServiceModule.hxx>
#include <myService/Subscribers.hxx>
extern myService::MyServiceModule mgt;
extern int accessCounts;

#define PAGE  "<html><head><title>SAFplus demo</title></head><body>start</body></html>"


static int ahc_echo(void * cls,
		    struct MHD_Connection * connection,
		    const char * url,
		    const char * method,
                    const char * version,
		    const char * upload_data,
		    size_t * upload_data_size,
                    void ** ptr) {
  static int dummy;
  const char * page = (char*) cls;
  struct MHD_Response * response;
  int ret;
  
  if (0 != strcmp(method, "GET"))
    return MHD_NO; /* unexpected method */
  if (&dummy != *ptr)
    {
      /* The first time only the headers are valid, do not respond in the first round... */
      *ptr = &dummy;
      return MHD_YES;
    }
  if (0 != *upload_data_size)
    return MHD_NO; /* upload data in a GET!? */

  const char* user = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user");
  if (user == NULL)
    return MHD_NO;

  myService::Subscribers* sub = (myService::Subscribers*) mgt.subscribersList.find(user);
  if (sub)
    {
      if (sub->active)
        {
          if (sub->use >= sub->limit)
            {
            const char * nouser = "<html><head><title>SAFplus demo</title></head><body>usage limit exceeded</body></html>";
            response = MHD_create_response_from_data(strlen(nouser), (void*) nouser, MHD_NO, MHD_NO);
            }
          else
            {
              const char* resp = "<html><head><title>SAFplus demo</title></head><body>Hi %s.  You've accessed this %d times</body></html>";
              static char buf[1024]={0};
              snprintf(buf,1024,resp,sub->name.value.c_str(),sub->use.value);
              accessCounts++;
              sub->use.value++;
              *ptr = NULL; /* clear context pointer */
              response = MHD_create_response_from_data(strlen(buf), (void*) buf, MHD_NO, MHD_NO);
            }
        }
      else
        {
        const char * nouser = "<html><head><title>SAFplus demo</title></head><body>account is inactive</body></html>";
        response = MHD_create_response_from_data(strlen(nouser), (void*) nouser, MHD_NO, MHD_NO);
        }
    }
  else
    {
      const char * nouser = "<html><head><title>SAFplus demo</title></head><body>Unknown user</body></html>";
      response = MHD_create_response_from_data(strlen(nouser), (void*) nouser, MHD_NO, MHD_NO);
    }
 
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}

struct MHD_Daemon * myDaemon=NULL;

void startHttpDaemon(int port)
{
  myDaemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
		       port,
		       NULL,
		       NULL,
		       &ahc_echo,
		       (void*) PAGE,
                       MHD_OPTION_END
		       );
}


void stopHttpDaemon(int port)
{
  if (myDaemon) MHD_stop_daemon (myDaemon);
}
