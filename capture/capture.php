<?php
	# Install on camera computer as /var/www/html/camera/capture.php

	if (isset($_SERVER['HTTP_USER_AGENT']) && (strpos($_SERVER['HTTP_USER_AGENT'], 'MSIE') !== false)) {
	  # This is for Internet Explorer, the browser that doesn't listen to HTTP standards.
	  header('Cache-Control: no-cache');
	  header('Expires: -1');
	}
	else {
	  # This is for everyone else that listens to HTTP standards; like FireFox, Opera, and Chrome.
	  header('Cache-Control: no-cache, must-revalidate'); // HTTP/1.1
	  header('Expires: Sat, 26 Jul 1997 05:00:00 GMT'); // Date in the past
	}

	$capture = "/etc/init.d/capture";

	if(isset($_GET["start"])) {
		shell_exec("$capture start > /dev/null &");
	}
	else if(isset($_GET["stop"])) {
		shell_exec("$capture stop");
	}
	else if(isset($_GET["restart"])) {
		shell_exec("$capture restart");
	}

?>

