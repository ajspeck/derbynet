<?php
// Action to schedule heats for a round, with each car running once in each lane.
// (An exception is if max-runs-per-car is non-zero; see below.)
//

$roundid = $_POST['roundid'];
$n_times_per_lane = isset($_POST['n_times_per_lane']) ? $_POST['n_times_per_lane'] : 1;

$trace = !isset($_POST['trace']) ? 0 : ($_POST['trace'] == 'v' ? 2 : 1);

//
// Normally we use max-runs-per-car = 0, so that each car runs once per lane.
// For some unusual situations, we want to limit to just 1 run per car (no
// matter which lane).  In that case, we first pad out the roster with BYEs so
// each car can get one run before we would start running anyone's second run.
// Then we form the rough schedule as a simple rotation chart (because the
// normal chart generators may put a given car only in heats that include a
// second run for some other car).  Finally, in make_ordered_schedule, we
// abandon those heats from the rough schedule that would cause any car to have
// second run.

require_once('inc/data.inc');
require_once('inc/masterheat.inc');

require_once('inc/schedule_generator.inc');

// RACE_COUNT_WEIGHT is the weight (penalty) for uneven race counts.  That is,
// when some racers have raced a lot more than others, RACE_COUNT_WEIGHT scales
// the deviation from a preferred condition of having the same race count for
// all racers.
define('RACE_COUNT_WEIGHT', 10);
// CONSECUTIVE_WEIGHT is the weight applied when a car appears in consecutive
// heats.
define('CONSECUTIVE_WEIGHT', 1000);
// CONSECUTIVE_WEIGHT + REPEAT_WEIGHT is the weight if a car appears in the same
// lane in consecutive heats.
define('REPEAT_WEIGHT', 1000);


// $appearances[], indexed by racer (0 .. $ncars - 1), counts the
// number of heats that include that racer.
function rate_race_counts(&$next_heat, $n_prev_heats, &$appearances) {
  $nlanes = count($next_heat);
  $ncars = count($appearances);
  $new_appearances = $appearances;  // Copies $appearances
  foreach ($next_heat as $car) {
    ++$new_appearances[$car];
  }

  $dev = 0;
  // For any car, $target tells how many appearances we'd expect after
  // $next_heat.
  $target = (($n_prev_heats + 1) * $nlanes) / $ncars;
  foreach ($new_appearances as $app) {
    $dev += ($app - $target) * ($app - $target);
  }
  return $dev * RACE_COUNT_WEIGHT;
}

// Assess penalties for having the same car(s) appear in consecutive heats.
function rate_consecutive(&$prev_heat, &$next_heat) {
  $result = 0;
  for ($lane = 0; $lane < count($next_heat); ++$lane) {
    for ($prev_lane = 0; $prev_lane < count($prev_heat); ++$prev_lane) {
      if ($prev_heat[$prev_lane] == $next_heat[$lane]) {
        $result += CONSECUTIVE_WEIGHT;
        if ($lane == $prev_lane) {
          $result += REPEAT_WEIGHT;
        }
      }
    }
  }
  return $result;
}

function rate_next_heat(&$prev_heat, $n_prev_heats, &$next_heat, &$appearances) {
  $rating = rate_race_counts($next_heat, $n_prev_heats, $appearances);
  if ($n_prev_heats > 0) {
    $rating += rate_consecutive($prev_heat, $next_heat);
  }
  return $rating;
}

// Starts with the rough schedule (as produced by generator(s)), and then
// reorders the heats to optimize goals of keeping even race counts, and
// avoiding consecutive races for the same racer.
function make_ordered_schedule($nlanes, $ncars, $n_times_per_lane,
                               $max_runs_per_car, $trace) {
  $rough = make_rough_schedule($nlanes, $ncars, $n_times_per_lane,
                               $max_runs_per_car, $trace);
  if (empty($rough)) {
    return $rough;
  }

  // $appearances[$car] tells how many times $car has appeared in the ordered schedule
  $appearances = array();
  for ($i = 0; $i < $ncars; ++$i) {
    $appearances[] = 0;
  }

  $ordered = array();
  $prev_heat = array();
  for ($round = 0; $round < $n_times_per_lane; ++$round) {
    for ($car_index = 0; $car_index < $ncars; ++$car_index) {
      $best_heat_index = count($rough) - 1;
      $best_heat_score = 10000;
      for ($rough_i = 0; $rough_i < count($rough); ++$rough_i) {
        if ($rough[$rough_i]) {
          $score = rate_next_heat($prev_heat, count($ordered), $rough[$rough_i],
                                  $appearances);
          if ($trace > 1) {
            echo "<rating score='".$score."' index='".$rough_i."'/>";
          }
          if ($score < $best_heat_score - 0.00001) {
            $best_heat_score = $score;
            $best_heat_index = $rough_i;
          }
        }
      }

      if ($trace) {
        echo "<ordered rough_index='".$best_heat_index."' score='".$best_heat_score."'>";
        echo json_encode($rough[$best_heat_index]);
        echo "</ordered>\n";
      }
      
      $prev_heat = $rough[$best_heat_index];
      $rough[$best_heat_index] = NULL;
      foreach ($prev_heat as $car) {
        ++$appearances[$car];
        if ($max_runs_per_car > 0 && $appearances[$car] > $max_runs_per_car) {
          // Exit early, without adding this new heat
          return $ordered;
        }
      }

      $ordered[] = $prev_heat;
    }
  }

  return $ordered;
}

// Returns roster, where roster[sequence] = racerid,
// 0 <= sequence < number of racers in roundid.
function read_roster($roundid) {
  global $db;
  $roster = array();
  $stmt = $db->prepare('SELECT rosterid, Roster.racerid FROM Roster'
                       .' INNER JOIN RegistrationInfo'
                       .' ON Roster.racerid = RegistrationInfo.racerid'
                       .' WHERE roundid = :roundid'
                       .' AND passedinspection = 1'
                       .' ORDER BY rosterid');
  $stmt->execute(array(':roundid' => $roundid));
  foreach ($stmt as $row) {
    $roster[] = $row['racerid'];
  }
  return $roster;
}

// Converts a schedule (racers identified by sequence, 0 <= c < $ncars) to a
// race chart (racers identified by racerid).
function form_chart(&$schedule, &$roster) {
  $heats = array();
  foreach ($schedule as $sched_heat) {
    $heat = array();
    foreach ($sched_heat as $index) {
      $heat[] = $roster[$index];
    }
    $heats[] = $heat;
  }
  return $heats;
}

// The track may have more lanes than are actually used.
function spread_across_unused_lanes(&$chart) {
  // 1's in the mask mark lanes that will NOT be used.
  $mask = 0 + read_raceinfo('unused-lane-mask', 0);
  if ($mask == 0) {
    return $chart;
  }
  $wide = array();
  foreach ($chart as $narrow_heat) {
    $p = 0;  // Physical lane index
    $wide_heat = array();
    foreach ($narrow_heat as $racer) {
      while (($mask & (1 << $p)) != 0) {
        $wide_heat[] = NULL;
        ++$p;
      }
      $wide_heat[] = $racer;
      ++$p;
    }
    $wide[] = $wide_heat;
  }
  return $wide;
}

// Writes the race chart entries into the database.
function write_chart($roundid, &$heats) {
  global $db;
  $db->exec('DELETE FROM RaceChart WHERE roundid = '.$roundid);
  $classid = read_single_value('SELECT classid FROM Rounds WHERE roundid = '.$roundid);
  $stmt = $db->prepare('INSERT INTO RaceChart(roundid, heat, lane, racerid, classid)'
                       .' VALUES(:roundid, :heat, :lane, :racerid, :classid)');
  $heat_index = 0;
  foreach ($heats as $heat) {
    ++$heat_index;
    $lane_index = 0;
    foreach ($heat as $racerid) {
      ++$lane_index;
      if ($racerid) {
        $stmt->execute(array(':roundid' => $roundid,
                             ':heat' => $heat_index,
                             ':lane' => $lane_index,
                             ':racerid' => $racerid,
                             ':classid' => $classid));
      }
    }
  }
}

function results_exist($roundid) {
  global $db;
  return read_single_value('SELECT COUNT(*) FROM RaceChart WHERE roundid = :roundid'
                           .' AND (finishtime IS NOT NULL OR finishplace IS NOT NULL)',
                           array(':roundid' => $roundid));
}


// The top-level scheduling function.  Constructs a schedule, converts to a race
// chart, and writes the race chart into the database.
function schedule_one_round($roundid, $nlanes, $n_times_per_lane, $trace) {
  $roster = read_roster($roundid);
  if (count($roster) < 2) {
    return false;
  }
  while (count($roster) < $nlanes) {
    $roster[] = null;  // Byes to bring up the roster count to at least the number of lanes
  }
  $max_runs_per_car = read_raceinfo('max-runs-per-car', 0);
  while (count($roster) * $max_runs_per_car % $nlanes != 0) {
    $roster[] = null;
  }
  $sched = make_ordered_schedule($nlanes, count($roster), $n_times_per_lane,
                                 $max_runs_per_car, $trace);
  write_chart($roundid, spread_across_unused_lanes(form_chart($sched, $roster)));
  return true;
}

// For debugging only:
function print_chart(&$heats) {
  echo '<table>';
  $first = true;
  foreach ($heats as $heat) {
    if ($first) {
      echo '<tr>';
      for ($lane = 1; $lane <= count($heat); ++$lane) {
        echo '<th>Lane '.$lane.'</th>';
      }
      echo '</tr>'."\n";
      $first = false;
    }
    echo '<tr>';
    foreach ($heat as $car) {
      echo '<td>'.$car.'</td>';
    }
    echo '</tr>'."\n";
  }
  echo '</table>'."\n";
}

function read_race_chart($roundid) {
  global $db;
  $heats = array();
  $stmt = $db->prepare('SELECT heat, lane, racerid FROM RaceChart WHERE roundid = :roundid ORDER BY heat, lane');
  foreach ($stmt->execute(array(':roundid' => $roundid)) as $row) {
    while ($row['heat'] > count($heats)) {
      $heats[] = array();
    }
    while ($row['lane'] > count($heats[$row['heat'] - 1])) {
      $heats[$row['heat'] - 1][] = 'Bye';
    }
    $heats[$row['heat'] - 1][$row['lane'] - 1] = $row['racerid'];
  }

  return $heats;
}

function count_one_bits($n) {
  $onebits = 0;
  while ($n != 0) {
    if (($n & 1) != 0) {
      ++$onebits;
    }
    $n = $n >> 1;
  }
  return $onebits;
}

start_response();

if (have_permission(CONTROL_RACE_PERMISSION)) {
  // $nlanes reports the number of physical lanes, even if some won't be used.
  $nlanes = read_raceinfo('lane_count', 0);
  // For generating the schedule, we care about the number of lanes actually in use.
  $nlanes -= count_one_bits(read_raceinfo('unused-lane-mask', 0));
  if ($nlanes <= 0) {
	echo '<failure code="nlanes">Number of lanes hasn\'t been determined.</failure>';
// TODO
//  } else if (!isset($all_generators[$nlanes])) {
//    echo '<failure code="too-many-lanes">Lane scheduling is only available for up to '
//    .max(array_keys($all_generators)).' lanes.</failure>';
  } else if (results_exist($roundid)) {
	echo '<failure code="results-exist">Race results already exist for this round</failure>';
  } else {
	if (schedule_one_round($roundid, $nlanes, $n_times_per_lane, $trace)) {
      echo '<success/>';
      require('ajax/query.poll.coordinator.inc');
    } else {
      echo '<failure code="too-few-racers">There need to be at least two racers checked in'
      .' to generate a schedule.</failure>';
    }
  }

  assign_master_heats(read_single_value('SELECT round FROM Rounds WHERE roundid = :roundid',
                                        array(':roundid' => $roundid)));
} else {
  not_authorized_body();
}

end_response();
?>
