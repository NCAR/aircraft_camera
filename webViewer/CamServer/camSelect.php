
<?php

//	ini_set('display_errors','1');
	putenv('PGHOST=lenado.eol.ucar.edu');
	putenv('PGDATABASE=real-time');
	putenv('PGUSER=ads');

    $numCams = shell_exec(' psql -t -c "SELECT COUNT(*) FROM camera;" ');
	echo '<span id="numCams" style="display:none;">'.$numCams.'</span>';
	
	for ($counter = 0; $counter < $numCams; $counter += 1) {
	    $output = shell_exec(' psql -A -t -c "SELECT direction FROM camera ORDER BY direction LIMIT 1 OFFSET ' . $counter . ';" ');
		echo '<input type="checkbox" id="showCam'.($counter+1).'" class="camCheck" alt="'.$output."\"";
		if ($counter == 1) {
			echo " checked";
		}
		echo ' onChange="autoscaler();"/>'.$output.' <br /> ';
	}
 
?>

