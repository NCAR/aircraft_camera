
<?php

    if(isset($_GET["start"])) {
		$arg="start=yes";
	}
	else if(isset($_GET["stop"])) {
		$arg="stop=yes";
	}
	echo shell_exec("wget -qO- sloop2/cam/capture.php?".$arg);
 
?>

