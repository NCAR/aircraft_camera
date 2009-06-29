<?php
	ini_set('display_errors','1');

    $dbh = getenv('PGHOST');
    $dbU = getenv('PGUSER');
    $dbd = getenv('PGDATABASE');
    $dbconn = pg_connect("host=$dbh dbname=$dbd user=$dbU")
		or die('Could not connect: '.pg_last_error());

	if(isset($_GET["first"])) {
		$query = "SELECT value FROM global_attributes WHERE key='FlightNumber'";
		$result = pg_query($query) or die('Query Failed: '. pg_last_error());
		$flNum = str_replace(" ","",pg_fetch_array($result));
		echo "<span id='curFlNum' style='display:none;'>$flNum[0]</span>\n";

		$date = time()*1000;
		echo "<span id='datetime' style='display:none;'>$date</span>\n";
	} 

	$query = "SELECT status,message FROM camera";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$status = pg_fetch_row($result);	

	if ($status[0]) {
		echo "<strong style=\"color: green\">";
	} else {
		echo "<strong style=\"color: red\">";
	}

	echo "$status[1]</strong>";

	pg_free_result($result);
	pg_close($dbconn);

?>
