<?php
//	ini_set('display_errors', '1');

	$dbconn = pg_connect("host=lenado.eol.ucar.edu dbname=real-time user=ads")
		or die('Could not connect: '.pg_last_error());

	$query = "SELECT latest FROM camera ORDER BY direction LIMIT 1 OFFSET " . $_GET['cam'];
	$result = pg_query($query) or die('Query Failed: '. pg_last_error());
	$image = str_replace(' ', '', pg_fetch_result($result, 0, 0));
	echo $image;
?>
