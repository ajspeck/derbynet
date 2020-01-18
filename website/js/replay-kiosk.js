// from https://stackoverflow.com/questions/470832/getting-an-absolute-url-from-a-relative-one-ie6-issue/22918332#22918332
function canonicalize(url) {
    var div = document.createElement('div');
    div.innerHTML = "<a></a>";
    div.firstChild.href = url; // Ensures that the href is properly escaped
    div.innerHTML = div.innerHTML; // Run the current innerHTML back through the parser
    return div.firstChild.href;
  }
  
  
  var Lineup = {
    // heat and roundid identify the lineup currently displayed.  They're also
    // sent in the polling query to tell the server what lineup we'd want results
    // for.
    roundid: 0,
    heat: 0,
    // Normally we just acquire more heat results for the current heat, as they're
    // reported by the timer.  If the number of <heat-result> elements decreases
    // for a heat, we need to clear out the old values by treating the update like
    // it's a new heat.  previous_heat_results tracks how many <heat-result>
    // elements were in the previous update.
    previous_heat_results: 0,
  
    // hold_display_until tells when it's OK to change the display.  Value is a
    // timestamp in milliseconds.
    hold_display_until: 0,
  
    // hold information about replay
    replay_video: '',
    replay_times: 2,
    replay_max_times:2,
  
    // After an animation of heat results, hold the display for a few seconds
    // before advancing to the next heat.
    hold_display: function() {
      this.hold_display_until = (new Date()).valueOf() + 10000;
    },
  
    ok_to_change: function() {
      return ((new Date()).valueOf() > this.hold_display_until)&&(this.replay_times>=this.replay_max_times);
    },
  
    // When the current heat differs from what we're presently displaying,
    // we get a <current-heat/> element, plus some number of <racer>
    // elements identifying the new heat's contestants.
    process_new_lineup: function(now_racing) {
      if (now_racing.getElementsByTagName("hold-current-screen").length > 0) {
        // Each time a hold-current-screen element is sent, reset the
        // hold-display deadline.  (Our display is presumed not to be visible,
        // so the display-for-ten-seconds clock shouldn't start yet.)
        this.hold_display();
      }
      
      var current = now_racing.getElementsByTagName("current-heat")[0];
  
      // We always need to notice an increase in the number of heat-results, in
      // case they get cleared before the ok_to_change() test lets us update the
      // screen.
      var new_heat_results = now_racing.getElementsByTagName("heat-result").length;
      if (new_heat_results > this.previous_heat_results) {
        this.previous_heat_results = new_heat_results;
      }

      var replay_info = now_racing.getElementsByTagName("replay")[0];
      var new_replay_url = canonicalize(replay_info.getAttribute('url'));
      var replay_rate = replay_info.getAttribute('rate');
      $('#replay-video')[0].playbackRate=replay_rate;
      if ((new_replay_url) && (new_replay_url != $('#replay-video-src')[0].src)) {
        $('#replay-video-src')[0].src = new_replay_url;
        this.replay_times=0;
        $('#replay-video')[0].load();
        $('#replay-video').css('visibility','visible');
        $('#replay-video').off('ended');
        $('#replay-video').on('ended',$.proxy(function(){
          this.replay_times+=1;
          if (this.replay_times<this.replay_max_times)
          {
            $('#replay-video')[0].play();
          }
          else
          {
            $('#replay-video').css('visibility','hidden');
          }
        },this));
        $('#replay-video')[0].play();
      }
      if (this.ok_to_change()) {
        var new_roundid = current.getAttribute("roundid");
        var new_heat = current.getAttribute("heat");
        var is_new_heat = new_roundid != this.roundid || new_heat != this.heat
            || new_heat_results < this.previous_heat_results;
        this.roundid = new_roundid;
        this.heat = new_heat;
        this.previous_heat_results = new_heat_results;
  
        if (current.firstChild) {  // The body of the <current-heat>
          // element names the class
          $('.banner_title').text(current.firstChild.data
                                  + ', Heat ' + this.heat
                                  + ' of ' + current.getAttribute('number-of-heats'));
        }  
      }
  
      // NOTE Any failure to get here will cause the page to get stuck.
      Poller.queue_next_request(this.roundid, this.heat);
    }
  };
  
  
  
  var Poller = {
    // Timestamp when the last polling request was sent; used by watchdog to
    // detect a failure to queue a new request.
    time_of_last_request: 0,
  
    // Records the identifier of the timer to send the next polling request, or 0
    // if no request is queued.
    id_of_timeout: 0,
  
    // Queues the next poll request when processing of the current request has
    // completed, including animations, if any.  Because animations are handled
    // asynchronously, with completion callbacks, we can't just start the next
    // request when process_now_racing_element returns.
    queue_next_request: function(roundid, heat) {
      if (this.id_of_timeout != 0) {
        console.log("Trying to queue a polling request when there's already one pending; ignored.");
      } else {
        Poller.id_of_timeout = setTimeout(function() {
          Poller.id_of_timeout = 0;
          Poller.poll_for_update(roundid, heat);
        }, 500);  // 0.5 sec
      }
    },
  
    poll_for_update: function(roundid, heat) {
      if (typeof(simulated_poll_for_update) == "function") {
        simulated_poll_for_update();
      } else {
  
        this.time_of_last_request = (new Date()).valueOf();
        $.ajax('action.php',
               {type: 'GET',
                data: {query: 'poll.now-racing',
                       roundid: roundid,
                       heat: heat},
                success: function(data) {
                  process_polling_result(data);
                },
                error: function() {
                  Poller.queue_next_request(roundid, heat);
                }
               });
      }
    },
  
    // Correct operation depends on queuing a new request when we're done
    // processing the last one, including any animations (could take a few
    // seconds).  As a safeguard against a bug that perhaps prevents queuing the
    // next request, the watchdog periodically tests whether one seems overdue,
    // and may queue a new request if so.
    watchdog: function() {
      if (this.id_of_timeout != 0 && this.time_of_last_request + 15000 < (new Date()).valueOf()) {
        console.error("Watchdog notices no requests lately, and none queued!");
        this.queue_next_request();
      }
    }
  };
  
  // See ajax/query.poll.now-racing.inc for XML format
  
  // Processes the top-level <now-racing> element.
  //
  // Walks through each of the <heat-result lane= time= place= speed=> elements,
  // in order, building a mapping from the reported place to the matching lane.
  //
  
  function process_polling_result(now_racing) {
    var heat_results = now_racing.getElementsByTagName("heat-result");
    Lineup.process_new_lineup(now_racing);
  }
  

  $(function () {
    // This 1-second delay is to let the initial resizing take effect
    setTimeout(function() { Poller.poll_for_update(0, 0); }, 1000);
    // Run the watchdog every couple seconds
    setInterval(function() { Poller.watchdog(); }, 2000);
  });
