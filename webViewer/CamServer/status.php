<?php
	putenv('PGHOST=lenado.eol.ucar.edu');
	putenv('PGDATABASE=real-time');
	putenv('PGUSER=ads');

	if(isset($_GET["first"])) {
		$flNum = shell_exec("(psql -A -t -c \"SELECT value FROM global_attributes WHERE key='FlightNumber'\";) |  tr -d \"\\n\" | tr -d \" \"");
		echo "<span id='curFlNum' style='display:none;'>".$flNum."</span>";

		$date = time()*1000;
		echo "<span id='datetime' style='display:none;'>".$date."</span>";
	} 

	$night = shell_exec(' psql -A -t -c "SELECT night FROM camera ORDER BY night LIMIT 1 OFFSET 0;"');
	$filename = './pid';
	if ((file_exists($filename)) && ($night < 10)) {
		echo "<strong style=\"color: green\">Recording Images</strong>";
	} else {
		echo "<strong style=\"color: red\">Not Recording Images</strong>";
	}

?>
