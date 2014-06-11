php_pthread_fcgi
================

An extension of PHP with pthread and fastcgi.

Install:

/usr/local/php/bin/phpize
./configure --with-php-config=/usr/local/php/bin/php-config
make
make install

Sample:
<?php
fcgi_set_domain('127.0.0.1', 9000); //your ip or domain, and your port. your fastcgi server. e.g. php-fpm
fcgi_set_param(array(
"REQUEST_METHOD"=>"GET",
"SCRIPT_FILENAME"=>"/var/www/html/index.php",
"REQUEST_URI"=>"action=test"
), "");     //at least these three parameters,and you can do any thing in this php script.
fcgi_set_param(array(
"REQUEST_METHOD"=>"GET",
"SCRIPT_FILENAME"=>"/var/www/html/index.php",
"REQUEST_URI"=>"action=test2"
), "");
fcgi_set_param(array(
"REQUEST_METHOD"=>"GET",
"SCRIPT_FILENAME"=>"/var/www/html/index.php",
"REQUEST_URI"=>"action=test3"
), "");
$ret = fastcgi();
fcgi_close();
var_dump($ret);

Notice:
the thread's task at most 10.
if php-fpm, config : pm.max_children  at least 2.
