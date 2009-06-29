<?php
	ini_set('display_errors','1');

    $dbh = getenv('PGHOST');
    $dbU = getenv('PGUSER');
    $dbd = getenv('PGDATABASE');
    $dbconn = pg_connect("host=$dbh dbname=$dbd user=$dbU")
		or die('Could not connect: '.pg_last_error());

	$query = "SELECT direction FROM camera";
	$result = pg_fetch_row(pg_query($query)) or die('Query Failed: '. pg_last_error());

	preg_match('/\{(.+)\}/', $result[0], $direc);
	$directions = explode(',', $direc[1]);

	$count = 1;
	foreach ($directions as $dir) {
		echo '<input type="checkbox" id="showCam'.$count.'" class="camCheck" alt="'.$dir.'"' ;
		if ($count == 1) echo " checked";
		echo ' onChange="autoscaler();" />' . $dir . " <br />\n";
		$count++;
	}

    pg_close($dbconn);

?>

