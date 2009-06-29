<?php
	ini_set('display_errors','1');
	
    $dbh = getenv('PGHOST');
    $dbU = getenv('PGUSER');
    $dbd = getenv('PGDATABASE');
    $dbconn = pg_connect("host=$dbh dbname=$dbd user=$dbU")
		or die('Could not connect: '.pg_last_error());

	$query = "SELECT units FROM variable_list WHERE name IN ('PITCH', 'THDG', 'TASX', 'ATX', 'PALTF') ORDER BY name";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$line = pg_fetch_all($result);
	foreach ($line as $nuPair) {
		foreach ($nuPair as $unit) {
			$units[] = $unit;
		}
	}	

	$query = "SELECT value FROM global_attributes WHERE key='EndTime'";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$line = pg_fetch_row($result);
	$datetime = substr_replace(str_replace(" ","",$line[0]), "", -4, 4);	

	$query = "SELECT ATX,PALTF,PITCH,TASX,THDG FROM raf_lrt WHERE datetime='$datetime'";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());

	echo "<table border='0'>\n";
	$count = 0;
	while ($line = pg_fetch_array($result, NULL, PGSQL_ASSOC)) {
		foreach ($line as $name => $col_val) {
		echo "<span class='fdName' >$name:</span><br />\n<span class='fdValue'>" . $col_val . "</span> <span class='units' >" . $units[$count] . " </span><br />\n";
		$count++;
		}
	}
	echo "</table>\n";

    pg_free_result($result);
    pg_close($dbconn);

?>
