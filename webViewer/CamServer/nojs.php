<?php
//	ini_set('display_errors', '1');

	$dbconn = pg_connect("host=lenado.eol.ucar.edu dbname=real-time user=ads")
		or die('Could not connect: '.pg_last_error());

	echo '<meta http-equiv="refresh" content="1" >'; 

	$filename = './pid';
	if (file_exists($filename)) {
		echo "<strong style=\"color: green\">Running</strong> - Displaying live images <br />";
	} else {
		echo "<strong style=\"color: red\">Stopped</strong> - Displaying latest archived image<br />";
	}

	$query = "SELECT value FROM global_attributes WHERE key='FlightNumber'";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$flNum = str_replace(' ', '', pg_fetch_result($result, 0, 0));

	$query = "SELECT direction,latest FROM camera ORDER BY direction";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	while ($line = pg_fetch_array($result, NULL, PGSQL_ASSOC)) {
		echo "<img src='http://lenado/camera/flight_number_" . $flNum . "/"
		. $line['direction'] . "/" . $line['latest'] . "' width=" . $_GET['scale'] . "/>";
	}


?>
