/* Taken from RFC1889 Appendix 6 */



/*
* Generate a random 32-bit quantity.
*/
#include <sys/types.h>   /* u_long */
#include <sys/time.h>    /* gettimeofday() */
#include <unistd.h>      /* get..() */
#include <stdio.h>       /* printf() */
#include <time.h>        /* clock() */
#include <sys/utsname.h> /* uname() */

#include "config.h"
#include "global.h"      /* from RFC 1321 */
#include "md5.h"         /* from RFC 1321 */


#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final


static u_long md_32(char *string, int length)
{
   MD_CTX context;
   union {
	   char   c[16];
	   u_long x[4];
   } digest;
   u_long r;
   int i;

   MDInit (&context);
   MDUpdate (&context, string, length);
   MDFinal ((unsigned char *)&digest, &context);
   r = 0;
   for (i = 0; i < 3; i++) {
	   r ^= digest.x[i];
   }
   return r;
}                               /* md_32 */



/*
* Return random unsigned 32-bit quantity. Use 'type' argument if you
* need to generate several different values in close succession.
*/
u_int32_t random32(int type)
{
   struct {
	   int     type;
	   struct  timeval tv;
	   clock_t cpu;
	   pid_t   pid;
	   u_long  hid;
	   uid_t   uid;
	   gid_t   gid;
	   struct  utsname name;
   } s;

   gettimeofday(&s.tv, 0);
   uname(&s.name);
   s.type = type;
   s.cpu  = clock();
   s.pid  = getpid();
   s.hid  = gethostid();
   s.uid  = getuid();
   s.gid  = getgid();

   return md_32((char *)&s, sizeof(s));
}
