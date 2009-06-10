
<?php

	$arg="";	
	if(isset($_GET["first"])) {
		$arg="?first=yes";
	}

	echo shell_exec("wget -qO- sloop2/cam/status.php".$arg);
 
?>

