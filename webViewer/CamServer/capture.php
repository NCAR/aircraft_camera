<?php

    if(isset($_GET["start"])) {
		shell_exec("./cameras.sh start > /dev/null &");
	}
	else if(isset($_GET["stop"])) {
		echo shell_exec("./cameras.sh stop");
	}

?>

