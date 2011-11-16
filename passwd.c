/*
    Copyright © 2011 Faidon Liambotis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or (at
    your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nss.h>
#include <pwd.h>
#include <errno.h>

enum nss_status _nss_uidpool_getpwuid_r(uid_t, struct passwd *, char *, size_t, int *);
enum nss_status _nss_uidpool_getpwnam_r(const char *, struct passwd *, char *, size_t, int *);
char * strstrip(char * string, char * trimchars);
int  fileparser(void);

#define CONFIG_FILE "/etc/libnss_uidpool.conf"


/* Some safe defaults first in case we don't have a config file */
char *USERNAME_PREFIX = "pool";
char *SHELL = "/bin/false";
char *HOME  = "/noexistent";
char *GECOS = "uidpool user,,,";
int UID_MIN = 10001;
int UID_MAX = 10101;
int DEFAULT_GID = 65534;

char * strstrip(char * s, char * t)
{
/*
    This function returns a pointer to a substring
    of the s, with all occurences of t removed.
*/
    char * cleaned;
    int counter = 0;

    /* at most strlen(s) */
    cleaned = malloc(strlen(s));
    while(*s)
    {
        if ( !strchr(t, *s) )
        {
            cleaned[counter] = *s;
            counter++ ;
        }
        ++s;
    }
    return cleaned;
}

int fileparser(void)
{
    FILE *fp;
    size_t len = 0;
    ssize_t read;
    char *key, *value, *saveptr, *clean, *cleaned;
    char *line = NULL;
    char *cleaners = "'\"";

    fp = fopen(CONFIG_FILE, "r");
    if (fp != NULL )
    {
        while ((read = getline(&line, &len, fp)) != -1)
        {
            /* comments get ignored first */
            if (line[0] == '#')
                continue;
            /* Then we clean usual invalid characters */
            cleaned = strstrip(line, cleaners);
            /* a = b, not a == b or a = b = c */
            if (index(cleaned, '=') == rindex(cleaned, '='))
            {
                /* clean newlines first */
                clean = strsep(&cleaned, "\n");
                /* Break the key-value pair */
                key   = strtok_r(clean, " =", &saveptr);
                value = strtok_r(NULL, " =", &saveptr);
                /* Our little grammar */
                if (strcmp(key, "UID_PREFIX") == 0)
                    USERNAME_PREFIX = value;
                if (strcmp(key, "UID_MIN") == 0)
                    UID_MIN = atoi(value);
                if (strcmp(key, "UID_MAX") == 0)
                    UID_MAX = atoi(value);
                if (strcmp(key, "DEFAULT_GID") == 0)
                    DEFAULT_GID = atoi(value);
                if (strcmp(key, "SHELL") == 0)
                    SHELL = value;
                if (strcmp(key, "HOME") == 0)
                    HOME  = value;
                if (strcmp(key, "GECOS") == 0)
                    GECOS  = value;
                /* None of our keywords, next line please*/
            }
        }
        fclose(fp);
    }
    return 0;
}
/* append a string to buf, move buf forward and reduce buflen */
static int append_to_buf(char **buf, size_t *buflen, const char *string)
{
	size_t len = strlen(string) + 1;

	if (len == 0) {
		errno = EOVERFLOW;
		return 1;
	}

	if (*buflen < len) {
		errno = ERANGE;
		return 1;
	}

	memcpy(*buf, string, len);
	*buf += len;
	*buflen -= len;

	return 0;
}

enum nss_status _nss_uidpool_getpwuid_r(uid_t uid, struct passwd *result, char *buf, size_t buflen, int *errnop)
{
    fileparser();
	*errnop = 0;

	/* the last check is a paranoid safeguard against returning root */
	if (uid < UID_MIN+1 || uid >= UID_MAX || uid == 0) {
		return NSS_STATUS_NOTFOUND;
	}


	if (!result || !buf) {
		return NSS_STATUS_UNAVAIL;
	}

	result->pw_uid = uid;
	result->pw_gid = DEFAULT_GID;


	#define ASSIGN_FROM_BUF(x, str) { 				\
		x = buf;				\
		if (append_to_buf(&buf, &buflen, str) == 1) {	\
			*errnop = errno;			\
			return NSS_STATUS_TRYAGAIN;		\
		}						\
	}

	ASSIGN_FROM_BUF(result->pw_passwd, "!");
	ASSIGN_FROM_BUF(result->pw_gecos, GECOS);
	ASSIGN_FROM_BUF(result->pw_dir, HOME);
	ASSIGN_FROM_BUF(result->pw_shell, SHELL);


	/* assume that uid length is 10 (i.e. uid_t is 32-bits) */
	if (buflen < strlen(USERNAME_PREFIX) + 10 + 1) {
		*errnop = ERANGE;
		return NSS_STATUS_TRYAGAIN;
	}
	sprintf(buf, "%s%u", USERNAME_PREFIX, (uid_t)(uid - UID_MIN));
	result->pw_name = buf;

	return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_uidpool_getpwnam_r(const char *name, struct passwd *result, char *buf, size_t buflen, int *errnop)
{
    fileparser();

	*errnop = 0;
	if (strncmp(name, USERNAME_PREFIX, strlen(USERNAME_PREFIX)) != 0) {
		return NSS_STATUS_NOTFOUND;
	}

	uid_t uid = atoi(name + strlen(USERNAME_PREFIX));
	uid += UID_MIN;

	return _nss_uidpool_getpwuid_r(uid, result, buf, buflen, errnop);
}
