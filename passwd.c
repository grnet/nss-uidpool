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

#define USERNAME_PREFIX "pool"
#define DEFAULT_GID 65534 /* nogroup */
#define UID_MIN 10000
#define UID_MAX 11000

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
	ASSIGN_FROM_BUF(result->pw_gecos, "uidpool user,,,");
	ASSIGN_FROM_BUF(result->pw_dir, "/nonexistent");
	ASSIGN_FROM_BUF(result->pw_shell, "/bin/false");

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
	*errnop = 0;
	if (strncmp(name, USERNAME_PREFIX, strlen(USERNAME_PREFIX)) != 0) {
		return NSS_STATUS_NOTFOUND;
	}

	uid_t uid = atoi(name + strlen(USERNAME_PREFIX));
	uid += UID_MIN;

	return _nss_uidpool_getpwuid_r(uid, result, buf, buflen, errnop);
}
