var particle = new Particle();
var token;

function getToken() {
  token = Cookies.get('particle-token');
  return token;
}

function setToken(newToken) {
  token = newToken;
  Cookies.set('particle-token', token);
}

function login() {
  if (!getToken()) {
    var loginForm =
      '<form class="login-form">' +
      '<h1 class="main-title">Code Reader App</h1>' +
      '<h2 class="prompt">Log in with your Particle account</h2>' +
      '<input id="username" placeholder="Email address" class="form-control">' +
      '<input id="password" type="password" placeholder="Password" class="form-control">' +
      '<input id="login" type="submit" value="Log in" class="btn btn-block btn-lg btn-info">' +
      '<p id="error-message" class="error"></p>'
      '<a href="https://carloop.readme.io" class="help">See the docs to set up your account</a>' +
      '</form>';
    $('#app').html(loginForm);
    $('.login-form').on('submit', function (event) {
      event.preventDefault();
      particleLogin()
    });
  } else {
    selectDevice();
  }
}

function particleLogin() {
  var $username = $('#username');
  var $password = $('#password');
  var $submit = $('#login');

  $username.prop('disabled', true);
  $password.prop('disabled', true);
  $submit.prop('disabled', true);

  var username = $username.val();
  var password = $password.val();

  particle.login({ username: username, password: password })
  .then(function (data) {
    setToken(data.body.access_token);
    selectDevice();
  }, function (err) {
    $('#error').html(err);

    $username.prop('disabled', false);
    $password.prop('disabled', false);
    $submit.prop('disabled', false);
  });
}

function selectDevice() {

}

login();
