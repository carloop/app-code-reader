var particle = new Particle();
var token;
var device;
var $app = $("#app");

var codeUrl = "https://raw.githubusercontent.com/carloop/app-code-reader/master/src/";
var appFiles = [
  "app-code-reader.cpp",
  "dtc.cpp",
  "dtc.h",
  "OBDMessage.cpp",
  "OBDMessage.h",
  "project.properties",
];

var templates = {
  loginForm: _.template($("#template-login-form").text()),
  selectDevice: _.template($("#template-select-device").text()),
  error: _.template($("#template-error").text()),
  mainUI: _.template($("#main-ui").text()),
};

function getToken() {
  token = localStorage.getItem('particle-token');
  return token;
}

function setToken(newToken) {
  token = newToken;
  localStorage.setItem('particle-token', token);
}

function getDevice() {
  device = localStorage.getItem('particle-device');
  return device;
}

function setDevice(newDevice) {
  device = newDevice;
  localStorage.setItem('particle-device', device);
}


function login() {
  if (!getToken()) {
    $app.html(templates.loginForm());
    $('#login-form').on('submit', function (event) {
      event.preventDefault();
      particleLogin()
    });
  } else {
    selectDeviceForm();
  }
}

function particleLogin() {
  var $username = $('#username');
  var $password = $('#password');
  var $submit = $('#login');
  var $errorMessage = $('#error-message');

  $username.prop('disabled', true);
  $password.prop('disabled', true);
  $submit.prop('disabled', true);

  var username = $username.val();
  var password = $password.val();

  particle.login({ username: username, password: password })
  .then(function (data) {
    setToken(data.body.access_token);
    selectDeviceForm();
  }, function (err) {
    var message = err.body && err.body.error_description || "User credentials are invalid";
    $errorMessage.html(message);

    $username.prop('disabled', false);
    $password.prop('disabled', false);
    $submit.prop('disabled', false);
  });
}

function selectDeviceForm(force) {
  if (force || !getDevice()) {
    particle.listDevices({ auth: token })
    .then(function (data) {
      var devices = data.body;
      $app.html(templates.selectDevice({ devices: devices}));
      $('[data-toggle="select"]').select2();
      
      $("#select-device").on("submit", function (event) {
        event.preventDefault();
        setDevice($("#device").val());
        mainUI();
      });
    }, function (err) {
      showError();
    });
  } else {
    mainUI();
  }
}

function mainUI() {
  $app.html(templates.mainUI());

  var $flashButton = $("#flash-button").on('click', flashApp);
}

function flashApp() {

  var filePromises = appFiles.map(function (f) {
    return $.ajax(codeUrl + "/" + f);
  });

  Promise.all(filePromises)
  .then(function (files) {
    console.log("finished downloading files");
  }, function (err) {
    showError();
  });
}

function showError() {
  $app.html(templates.error());
}


login();
