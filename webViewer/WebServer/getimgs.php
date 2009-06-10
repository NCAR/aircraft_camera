<?php
echo shell_exec("wget -qO- sloop2/cam/getimgs.php?cam=".$_GET['cam'] );
?>
