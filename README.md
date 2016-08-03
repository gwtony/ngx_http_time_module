Description
===========
Nginx time module: Get expire time to set cookie

Usage
=====

* Nginx command
 - time_expire: expire time in second (must between 0 to 86400)
```
   http {
	   ...
	   time_expire 1800;
	   ...
   }
```
* Nginx variables
 - time_second: relative second (expire or second to end of today)
 - time_unix: second in unix time mode (expire second or second to end of today)
  
