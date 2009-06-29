<?php
	ini_set('display_errors', '1');

    $dbh = getenv('PGHOST');
    $dbU = getenv('PGUSER');
    $dbd = getenv('PGDATABASE');
    $dbconn = pg_connect("host=$dbh dbname=$dbd user=$dbU")
		or die('Could not connect: '.pg_last_error());

	echo '<meta http-equiv="refresh" content="1" >'; 

	$query = "SELECT status,message FROM camera";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$status = pg_fetch_row($result);	

	if ($status[0]) {
		echo "<strong style=\"color: green\">";
	} else {
		echo "<strong style=\"color: red\">";
	}
	echo "$status[1]</strong><br />\n";

	$query = "SELECT value FROM global_attributes WHERE key='FlightNumber'";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$flNum = str_replace(' ', '', pg_fetch_result($result, 0, 0));

	$query = "SELECT direction,latest FROM camera";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$bla = pg_fetch_row($result, NULL, PGSQL_ASSOC);
	
	preg_match_all('/\d+\-\d+/', $bla['latest'], $latest);
	$latest = $latest[0];

	preg_match('/\{(.+)\}/', $bla['direction'], $direc);
	$directions = explode(',', $direc[1]);

	$i = 0;
	foreach ($directions as $dir) {
		echo "<img src='http://lenado/camera/flight_number_$flNum/$dir/$latest[$i].jpg' ";
		if (isset($_GET['scale'])) echo "width= ".$_GET['scale'];
		echo " />\n";
		$i++;
	}

    pg_free_result($result);
    pg_close($dbconn);

?>
