<?php
//	ini_set('display_errors','1');

	$dbconn = pg_connect("host=lenado.eol.ucar.edu dbname=real-time user=ads")
		or die('Could not connect: '.pg_last_error());

	$query = "SELECT direction FROM camera ORDER BY direction";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$count = 1;
	while ($line = pg_fetch_array($result, NULL, PGSQL_ASSOC)) {
		echo '<input type="checkbox" id="showCam' . $count . '" class="camCheck" alt="' . $line['direction'] . '"' ;
		if ($count == 2) echo " checked";
		echo ' onChange="autoscaler();" />' . $line['direction'] . " <br />\n";
		$count += 1;
	}

?>

