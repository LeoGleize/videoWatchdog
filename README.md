#VideoWatchdog

VideoWatchdog is a software developed for screen monitoring and video processing using an IntensityPro acquisition card. The main idea is to connect a STB as video input and analyze its data, creating events for state changes and implementing other forms of data acquisition to automate the process of testing.

  The project was written in C++ and is divided in the following components:

* Main thread: implemented in ./IntensityAcquisition.cpp, this file is responsible for generating real time video output using the data acquired from IntensityPro.

* blackmagic/: This folder contains files used for communicating with IntensityPro drivers and acquiring video and audio, it also contains a SDK for developing software for Blackmagic products.

* recognition/: This folder contains some files specialized in image processing, and implement template matching and level monitoring for OpenCV material objects (cv::Mat).

* ServerInstance/: This folder has source code related to the REST Server implementation and declaration of routes used.

* watchdog/: Implementation of an watchdog that monitors all video output and reports events.

To build R7IntensityPro first run the script installDep.sh, this will install all the dependencies needed, and then you can simply run 'make' and 'sudo make install'. 

Before running the program you'll need to open "Blackmagic Desktop Video Utility" and change "video input" to the video input you are using ("component" instead of "hdmi" if using an HDFury). 
Now you can use Media Express and check you also need to change the resolution on Edit -> Preferences -> "Project Video Format" to either 1080i50 or 720p50 depending on your STB configuration.
If the card is working correctly, you can see the frames being acquired on "Log and capture" tab. 

Now you can copy the file config.json to the directory where you'll be executing videoWatchdog, you'll also need to modify this file to point to your IP address.
```bash	
./workspace/videoWatchdog$ cp config.json ~ ;cd
$ videoWatchdog -h
```
##Video Watchdog

###Options:

    -no-video : video display is not shown, server will be executed normally
    -res X Y  : set resolution of window
    -720p : set resolution of input to 720p instead of 1080i

##REST API

The server exposes an REST API on page 8080 with the following routes:

| Route        | Function       | Method | Arguments |
| ------------- |:-------------|:-----:|---------|
| /status       | Get status of STB | POST | {"timeAnalysis":1000} |
| /image      | Get image in binary format      |   GET |  None |
| /imageb64 | Get image in base 64 | GET | None  |
| /getScreenEvent | Search for events in video input | POST | {"eventType": ["LIVE", "FREEZE", "BLACK"],"timeAnalysis":600000, "timeEvent":5000,"count":true}  |
| /checkForImage | Check if an image is being shown in the screen | POST | {"imageURL":"http://image.to.search/img.png","returnImage":true}  |


Two other routes were created to control the watchdog, the first one is /watchdog that accepts two arguments via POST.

To start looking for events of at least 1.5s and of types: BLACK, FREEZE, LIVE and FREEZE_NO_AUDIO:
```json
{"start":true, "tevent":1500, "eventType":["BLACK","FREEZE","LIVE","FREEZE_NO_AUDIO"]}
```
And then you can stop the watchdog sending another POST message to the same route (this will return the list of observed events):
```json
{"stop":true}
```

There is also an second route that allows you to view wich elements were observed while the watchdog is still running:
You can do a get request on /report, both this request and a command to stop the watchdog have the same output format:
```json
{
  "incidents": [
    {
      "duration_ms": 1900,
      "event": "Freeze",
      "isIncidentFinished": true,
      "videoName": "http://10.0.1.19/logvideo_0_live_g3p.avi",
      "when": "17.11.2015 15:02:50"
    },
    {
      "duration_ms": 9900,
      "event": "Freeze",
      "isIncidentFinished": false,
      "videoName": "http://10.0.1.19/logvideo_575_live_ij6.avi",
      "when": "17.11.2015 16:38:54"
    }
  ],
  "message": "Watchdog is running",
  "startTime": "17.11.2015 15:02:49"
}
```


###dependencies:

	g++, libpthread, Boost, opencv, restcpp
	
