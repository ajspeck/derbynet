

function handle_login_photo(role, pwd) {
    $.ajax('action.php',
           {type: 'POST',
            data: {action: 'role.login',
                   name: role,
                   password: pwd},
            success: function(data) {
              if (data.hasOwnProperty('outcome') &&
                  data.outcome.hasOwnProperty('summary') &&
                  data.outcome.summary === 'success') {
              } else if (data.hasOwnProperty('outcome') &&
                         data.outcome.hasOwnProperty('summary') &&
                         data.outcome.summary === 'failure') {
				alert("Login fails: " + data.outcome.description);
		      } else {
				alert("Unrecognized XML: " + this.responseXML);
              }
            },
           });
}
handle_login_photo("Photo","flashbulb");

function handle_logout_photo()
{
    $.ajax('action.php',
           {type: 'POST',
            data: {action: 'role.login',
                   name: '',
                   password: ''}
            }
           );
}
$(window).bind('beforeunload', function(){
  handle_logout_photo();
});

