<?php
	putenv('PGHOST=lenado.eol.ucar.edu');
	putenv('PGDATABASE=real-time');
	putenv('PGUSER=ads');
	$flNum = shell_exec("(psql -A -t -c \"SELECT value FROM global_attributes WHERE key='FlightNumber'\";) |  tr -d \"\\n\" | tr -d \" \"");
    $numCams = shell_exec(' psql -t -c "SELECT COUNT(*) FROM camera;" ');
	
	//This may not work in IE because it caches php pages!
	echo '<meta http-equiv="refresh" content="1" >';

	$filename = './pid';
	if (file_exists($filename)) {
		echo "<strong style=\"color: green\">Running</strong> - Displaying live images <br />";
	} else {
		echo "<strong style=\"color: red\">Stopped</strong> - Displaying latest archived image<br />";
	}


	for ($counter = 0; $counter < $numCams; $counter += 1) {
	    $direction = shell_exec(' psql -A -t -c "SELECT direction FROM camera ORDER BY direction LIMIT 1 OFFSET ' . $counter . ';" ');
	    $image = shell_exec(' psql -A -t -c "SELECT latest FROM camera ORDER BY direction LIMIT 1 OFFSET ' . $counter . ';" ');
		echo '<img src="flight_number_'.$flNum.'/'.$direction.'/'.$image.'" width="'.$_GET['scale'].'"/>'.$output;
	}


?>
