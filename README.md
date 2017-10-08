# nss-uidpool

This Name Service Switch (NSS) module creates a pool of a predefined
number of users, say pool1-pool999. It can be used when wanting to
separate a number of processes into separate users. It was primarily
created to run KVM virtual machines each with a different user. 

## HOWTO

  * Build the Debian package or compile the source
  * Make sure your **/etc/libnss_uidpool.conf** contains something like:
```
USERNAME_PREFIX = "pool"
SHELL = "/bin/false"
HOME  = "/nonexistent"
GECOS = "uidpool_user,,,"
UID_MIN = 10000
UID_MAX = 11000
DEFAULT_GID = 65534
```
  * Change **/etc/nsswitch.conf** to include a uidpool entry for passwd: 
```
passwd:         compat uidpool
```

If you're using ganeti make sure to change uid-pool setting:
```
# gnt-cluster modify --uid-pool=10001-10500 
```
