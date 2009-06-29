<?php
	ini_set('display_errors', '1');

    $dbh = getenv('PGHOST');
    $dbU = getenv('PGUSER');
    $dbd = getenv('PGDATABASE');
    $dbconn = pg_connect("host=$dbh dbname=$dbd user=$dbU")
		or die('Could not connect: '.pg_last_error());

	$query = "SELECT latest FROM camera";
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$image = pg_fetch_row($result);
	preg_match_all('/\d+\-\d+/', $image[0], $latest);
	
	$i=0;
	
	echo '{"jsonImgs": [';
	foreach ($latest[0] as $lat) {
		if ($i>0) echo ", ";
		echo "\"$lat.jpg\"";
		$i++;
	}
	echo"]} ";

//	echo "$image.jpg";

    pg_free_result($result);
    pg_close($dbconn);

?>
