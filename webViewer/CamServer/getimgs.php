<?php
	putenv('PGHOST=lenado.eol.ucar.edu');
	putenv('PGDATABASE=real-time');
	putenv('PGUSER=ads');
	
	$image = shell_exec(' psql -A -t -c "SELECT latest FROM camera ORDER BY direction LIMIT 1 OFFSET ' . $_GET['cam'] . ';" ');
	echo $image;
?>
