var curDate = new Date();
// "Reminder" pin
var reminderPIN = {
  "id": "alarms-next-alarm",
  "time": curDate.toISOString(),
  "layout": {
    "type": "genericPin",
    "title": "Next Alarm",
    "subtitle": "",
    "body": "Launch app to modify it.",
    "primaryColor": "black",
    "secondaryColor": "black",
    "tinyIcon": "system://images/ALARM_CLOCK",
    "largeIcon": "system://images/ALARM_CLOCK",
    "backgroundColor": "pictonblue"
  },
  "actions": [
              {
              "title": "Launch App",
              "type": "openWatchApp",
              "launchCode": 0
              }]
};

/******************************* timeline lib *********************************/

// The timeline public URL root
var API_URL_ROOT = 'https://timeline-api.getpebble.com/';

/**
 * Send a request to the Pebble public web timeline API.
 * @param pin The JSON pin to insert. Must contain 'id' field.
 * @param type The type of request, either PUT or DELETE.
 * @param topics Array of topics if a shared pin, 'null' otherwise.
 * @param apiKey Timeline API key for this app, available from dev-portal.getpebble.com
 * @param callback The callback to receive the responseText after the request has completed.
 */
function timelineRequest(pin, type, topics, apiKey, callback) {
  // User or shared?
  var url = API_URL_ROOT + 'v1/' + ((topics != null) ? 'shared/' : 'user/') + 'pins/' + pin.id;

  // Create XHR
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    console.log('timeline: response received: ' + this.responseText);
    callback(this.responseText);
  };
  xhr.open(type, url);

  // Set headers
  xhr.setRequestHeader('Content-Type', 'application/json');
  if(topics != null) {
    xhr.setRequestHeader('X-Pin-Topics', '' + topics.join(','));
    xhr.setRequestHeader('X-API-Key', '' + apiKey);
  }

  // Get token
  Pebble.getTimelineToken(function(token) {
    // Add headers
    xhr.setRequestHeader('X-User-Token', '' + token);

    // Send
    xhr.send(JSON.stringify(pin));
    console.log('timeline: request sent.');
  }, function(error) { console.log('timeline: error getting timeline token: ' + error); });
}

/**
 * Insert a pin into the timeline for this user.
 * @param pin The JSON pin to insert.
 * @param callback The callback to receive the responseText after the request has completed.
 */
function insertUserPin(pin, callback) {
  timelineRequest(pin, 'PUT', null, null, callback);
}

/**
 * Delete a pin from the timeline for this user.
 * @param pin The JSON pin to delete.
 * @param callback The callback to receive the responseText after the request has completed.
 */
function deleteUserPin(pin, callback) {
  timelineRequest(pin, 'DELETE', null, null, callback);
}

/**
 * Insert a pin into the timeline for these topics.
 * @param pin The JSON pin to insert.
 * @param topics Array of topics to insert pin to.
 * @param apiKey Timeline API key for this app, available from dev-portal.getpebble.com
 * @param callback The callback to receive the responseText after the request has completed.
 */
function insertSharedPin(pin, topics, apiKey, callback) {
  timelineRequest(pin, 'PUT', topics, apiKey, callback);
}

/**
 * Delete a pin from the timeline for these topics.
 * @param pin The JSON pin to delete.
 * @param topics Array of topics to delete pin from.
 * @param apiKey Timeline API key for this app, available from dev-portal.getpebble.com
 * @param callback The callback to receive the responseText after the request has completed.
 */
function deleteSharedPin(pin, topics, apiKey, callback) {
  timelineRequest(pin, 'DELETE', topics, apiKey, callback);
}

/****************************** end timeline lib ******************************/

////////////////////////////////////////////////////////////////////////////////////////////////////
// App Message
//
var keys = {
  "DESCRIPTION": 1,
  "NEXT_ALARM": 0,
  "READY": 2
}
// message received
Pebble.addEventListener('appmessage', function(e) {
                        // check for key
                        // check that it is valid to send pins i.e. its SDK 3.0 or greater
                        if (typeof Pebble.getTimelineToken == 'function') {
                        // add time
                        curDate = new Date();
                        curDate.setTime(curDate.getTime() + e.payload[0]*1000);
                        reminderPIN.layout.subtitle = e.payload[1];
                        console.log('Description: ' + e.payload[1]);
                        reminderPIN.time = curDate;
                        // insert pin
                        insertUserPin(reminderPIN, function (responseText) {
                                      console.log('Pin Sent Result: ' + responseText);
                                      });
                        }
                        });

// loaded and ready
Pebble.addEventListener('ready', function(e) {
                        console.log("JS ready!");
//                        console.log(keys.READY);
                        Pebble.sendAppMessage({ 2: true });
                        });
