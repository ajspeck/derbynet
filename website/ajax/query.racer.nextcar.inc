<?php
//
// $_GET['rankid']:   rank id


require_once('inc/schema_version.inc');

$order = 0;
if (isset($_GET['rankid']))
  $rankid = $_GET['rankid'];  // Values are: name, class, car

$nextcarnumber = 0;
if (schema_version()>4);
{
  $sql = 'SELECT IFNULL(min(carnumber)+1,'
        .' (SELECT min(mincarnumber) from Ranks where rankid='.$rankid.')) as nextcarnumber'
        .' from RegistrationInfo t'
        .' inner join Ranks on t.rankid=Ranks.rankid'
        .' where t.rankid='.$rankid
        .' and carnumber>mincarnumber'
        .' and NOT EXISTS'
        .' (SELECT NULL FROM RegistrationInfo n WHERE n.carnumber=t.carnumber+1 AND n.rankid=t.rankid);';
  $stmt = $db->query($sql);
  $row = $stmt->fetch(PDO::FETCH_NUM);
  if (!$row || !isset($row[0]) || !$row[0]) {
  } else {
    $nextcarnumber = $row[0];
  }
}

echo '<racer nextcarnumber="'.$nextcarnumber.'"/>';
?>