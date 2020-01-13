<?php
// Renaming a rank:
// $_POST['rankid']
// $_POST['name']

require_once('inc/classes-xml.inc');
require_once('inc/schema_version.inc');

start_response();

if (have_permission(SET_UP_PERMISSION)) {
  if (isset($_POST['rankid']) && isset($_POST['name'])) && isset($_POST['mincarnumber']) {
    $stmt = $db->prepare('UPDATE Ranks SET rank = :name WHERE rankid = :rankid');
    $stmt->execute(array(':rankid' => $_POST['rankid'],
                         ':name' => trim($_POST['name'])));
    if (schema_version() >= 4) {
      $stmt = $db->prepare('UPDATE Ranks SET mincarnumber = :mincarnumber WHERE rankid = :rankid');
      $stmt->execute(array(':mincardnumber' => $_POST['mincarnumber'],
                           ':rankid' => $_POST['rankid']));
    }
    echo "<success/>\n";
    emit_all_classes();
  } else {
    echo "<failure code='missing-args'>Unrecognized argument signature</failure>\n";
  }
} else {
  echo "<failure code='permission'>Not authorized</failure>\n";
}

end_response();
?>