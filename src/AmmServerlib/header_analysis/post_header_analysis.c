#include "post_header_analysis.h"

#include "../tools/logs.h"
#include "../tools/http_tools.h"
#include "../stringscanners/postHeader.h"
#include "../stringscanners/httpHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Quick Reference -> A TYPICAL POST MESSAGE WITH A BINARY FILE CONTENT!
---------------------------------------------------------------------------
POST /formtest.html HTTP/1.1
Host: 127.0.0.1:8080
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:16.0) Gecko/20100101 Firefox/16.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
DNT: 1
Connection: keep-alive
Referer: http://127.0.0.1:8080/formtest.html
Content-Type: application/x-www-form-urlencoded
Content-Length: 40 <- This is the binary find content
user=&vehicle=Bike&testfile=DSC01537.JPG <-This is the form data as a URL var encoding

---

POST /myloader/index.php HTTP/1.1
Host: ammar.gr
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:16.0) Gecko/20100101 Firefox/16.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
DNT: 1
Connection: keep-alive
Referer: http://ammar.gr/myloader/
Cookie: wp-settings-1=m5%3Do%26m6%3Dc%26m4%3Do%26editor%3Dhtml%26m0%3Do%26imgsize%3Dmedium; wp-settings-time-1=1344596242; myshop=2883roc4oeq2k75eudsj3oa257; PHPSESSID=l5jejlnkd1htdh5t1uh1aa5ma0; wordpress_test_cookie=WP+Cookie+check; wordpress_logged_in_b7b2f92af29b61c363b9691a65231742=admin%7C1350397864%7C29cae87a47979992a2d057d8fa006626
Content-Type: multipart/form-data; boundary=---------------------------8096311293393534011208767067
Content-Length: 5347975
-----------------------------8096311293393534011208767067
Content-Disposition: form-data; name="rawresponse"

NO
-----------------------------8096311293393534011208767067
Content-Disposition: form-data; name="uploadedfile"; filename="DSC_1967.JPG"
Content-Type: image/jpeg

ÿØÿáÿþExif
HTTP/1.1 200 OK
Date: Mon, 15 Oct 2012 21:52:39 GMT
Server: Apache/2.2.14 (Ubuntu)
X-Powered-By: PHP/5.3.2-1ubuntu4.17
Vary: Accept-Encoding
Content-Encoding: gzip
Content-Length: 1128
Keep-Alive: timeout=15, max=98
Connection: Keep-Alive
Content-Type: text/html


*/

int AnalyzePOSTLineRequest(
                            struct AmmServer_Instance * instance,
                            struct HTTPHeader * output,
                            char * request,
                            unsigned int request_length,
                            unsigned int lines_gathered
                          )
{
         if (instance==0)
            {
             warning("Cannot AnalyzePOSTLineRequest for a null instance\n");
             fprintf(stderr,"AnalyzePOSTLineRequest lines_gathered = %u \n",lines_gathered);
             return 0;
            }
         //If we just had a POST request , it  may have a file associated with it , so we will check for content tags..
         //Scanning for the case that the line is -> Content-Type: (i.e.) application/x-www-form-urlencoded
         /* OR SOMETHING LIKE
         Content-Type: multipart/form-data; boundary=---------------------------15737843761039122011733265042
         Content-Length: 9288
         -----------------------------15737843761039122011733265042
         Content-Disposition: form-data; name="rawresponse"

         NO
         -----------------------------15737843761039122011733265042
         Content-Disposition: form-data; name="uploadedfile"; filename="feels.jpeg"
         Content-Type: image/jpeg

         IMAGE DATA STARTS HERE */

         unsigned int payload_start = 0;


          unsigned int requestType = scanFor_httpHeader(request,request_length);
          //fprintf(stderr,"Thinking about string (%s) starts with %c and %c  got back %u \n",request,request[0],request[1] , requestType);
          switch (requestType)
          {
            case POSTHEADER_CONTENT_TYPE :
                 payload_start+=strlen("CONTENT-TYPE:");
                 //if (output->ContentType!=0) { free(output->ContentType); output->ContentType=0; }
                 output->contentType=request+payload_start;
                 output->contentTypeLength=request_length-payload_start;
                 char * boundary = strstr(request+payload_start,"boundary=");
                 if (boundary==0) { fprintf(stderr,"Could not found boundary in string %s \n",output->contentType); return 0; }
                 boundary+=9;
                 fprintf(stderr,"Boundary = %s \n",boundary);

                 freeString(&output->boundary);
                 output->boundary = GetNewStringFromHTTPHeaderFieldPayload(boundary,strlen(boundary));
                 if (output->boundary==0) { fprintf(stderr,"Could not get boundary\n"); return 0; }
                return 1;
              break;

            case POSTHEADER_CONTENT_DISPOSITION :
                payload_start+=strlen("CONTENT-DISPOSITION:");
                output->contentDisposition=request+payload_start;
                output->contentDispositionLength=request_length-payload_start;
                return 1;
             break;

            case POSTHEADER_CONTENT_LENGTH :
                payload_start+=strlen("CONTENT-LENGTH:");
                output->ContentLength = GetIntFromHTTPHeaderFieldPayload(request+payload_start,request_length-payload_start);
               return (output->ContentLength!=0);
            break;

          };


   return 0;
}






