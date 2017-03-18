<?php
include('variables.php');
include('util.php');
include('functions.php');
include('sessions.php');

logout();
header('Location: ./login.php');
exit();
?>
