# ArduinoGoogleAPI


## Overview

Google API access on Arduino by implementing Google's OAuth 2.0 for TV and Limited-Input Device Applications

https://developers.google.com/identity/protocols/OAuth2ForDevices

Limited API scopes are available under this method

See - https://developers.google.com/identity/protocols/OAuth2ForDevices#allowedscopes

Created for Arduino MKR1000 with WiFi.

## Usage

### Arduino gives you an activation code in console
You could also show on LCD display, etc.

![alt text](https://github.com/8bitkick/ArduinoGoogleAPI/blob/master/console.png)


### User grants access in a web browser

![alt text](https://raw.githubusercontent.com/8bitkick/ArduinoGoogleAPI/master/allow.png)


### 3) Arduino can now call Google APIs
Default is allowing access to user files on Google Drive - but only files the app creates

https://developers.google.com/drive/v2/reference/#Files
