<?php
	
	sleep(2);
	
	$output = shell_exec('tail capture.log');
	$flastmod = date("F d Y H:i:s", filemtime('capture.log'));

	echo "<br />Latest log entry ( ".$flastmod." ):<br /><br /><pre>".$output."</pre>";


?>
