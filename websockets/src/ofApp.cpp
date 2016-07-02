#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

  ofSetVerticalSync(true);
  ofSetCircleResolution(80);
  ofBackground(54, 54, 54);

  // 0 output channels,
  // 2 input channels
  // 44100 samples per second
  // 256 samples per buffer
  // 4 num buffers (latency)

  //soundStream.printDeviceList();

  //if you want to set a different device id
  //soundStream.setDeviceID(0); //bear in mind the device id corresponds to all audio devices, including  input-only and output-only devices.

  int bufferSize = 256;


  left.assign(bufferSize, 0.0);
  right.assign(bufferSize, 0.0);
  volHistory.assign(400, 0.0);

  bufferCounter = 0;
  drawCounter   = 0;
  smoothedVol     = 0.0;
  scaledVol   = 0.0;

    sampleRate = 44100;
    channels = 2;

    /*soundStream.setDeviceID(4);
    soundStream.setup(this, 0, channels,sam, 256, 4);*/

    ofSetFrameRate(60);
    ofSetLogLevel(OF_LOG_VERBOSE);
    vidGrabber.setDesiredFrameRate(30);
    vidGrabber.initGrabber(640, 480);
//    vidRecorder.setFfmpegLocation(ofFilePath::getAbsolutePath("ffmpeg")); // use this is you have ffmpeg installed in your data folder

    fileName = "testMovie";
    fileExt = ".wav"; // ffmpeg uses the extension to determine the container type. run 'ffmpeg -formats' to see supported formats

    // override the default codecs if you like
    // run 'ffmpeg -codecs' to find out what your implementation supports (or -formats on some older versions)
    vidRecorder.setVideoCodec("mpeg4");
    vidRecorder.setVideoBitrate("800k");
    //vidRecorder.setAudioCodec("mp3");
    vidRecorder.setAudioCodec("pcm_s32le");
    vidRecorder.setAudioBitrate("192k");
    vidRecorder.setFfmpegOptions("-vn");

    ofAddListener(vidRecorder.outputFileCompleteEvent, this, &ofApp::recordingComplete);

    ofSetWindowShape(vidGrabber.getWidth(), vidGrabber.getHeight()  );
    bRecording = false;
    ofEnableAlphaBlending();

    isConnected = false;
    address = "http://127.0.0.1:8888";
    status = "not connected";

    socketIO.setup(address);
    ofAddListener(socketIO.notifyEvent, this, &ofApp::gotEvent);

    ofAddListener(socketIO.connectionEvent, this, &ofApp::onConnection);

}

void ofApp::onConnection () {
  isConnected = true;
  bindEvents();

  vector <ofSoundDevice> deviceList = soundStream.getDeviceList();
  std::string deviceNames;
  for (int i = 0; i < deviceList.size(); i++){
     deviceNames += ("\n"+std::to_string(deviceList[i].deviceID) +" "+ deviceList[i].name);
  }

  std::string devices = "devices";
  std::string param = deviceNames;
  socketIO.emit(devices, param);
}

void ofApp::bindEvents () {
  std::string serverEventName = "server-event";
  socketIO.bindEvent(serverEvent, serverEventName);
  ofAddListener(serverEvent, this, &ofApp::onServerEvent);

  std::string recordStartEventName = "startRecord";
  socketIO.bindEvent(recordStartEvent, recordStartEventName);
  ofAddListener(recordStartEvent, this, &ofApp::onRecordStartEvent);

  std::string recordEndEventName = "endRecord";
  socketIO.bindEvent(recordEndEvent, recordEndEventName);
  ofAddListener(recordEndEvent, this, &ofApp::onRecordEndEvent);

  std::string deviceID = "setDeviceId";
  socketIO.bindEvent(deviceIDEvent, deviceID);
  ofAddListener(deviceIDEvent, this, &ofApp::onSetDeviceIDEvent);

}

//--------------------------------------------------------------
void ofApp::gotEvent(string& name) {
  status = name;
}

//--------------------------------------------------------------
void ofApp::onServerEvent (ofxSocketIOData& data) {
  /*ofLogNotice("ofxSocketIO", data.getStringValue("stringData"));
  ofLogNotice("ofxSocketIO", ofToString(data.getIntValue("intData")));
  ofLogNotice("ofxSocketIO", ofToString(data.getFloatValue("floatData")));
  ofLogNotice("ofxSocketIO", ofToString(data.getBoolValue("boolData")));*/
}

void ofApp::onRecordStartEvent(ofxSocketIOData & data) {
  ofLogNotice("ofxSocketIO", "recordStarted");
  bRecording = !bRecording;
  fileName = data.getStringValue("fileName");
  if (bRecording && !vidRecorder.isInitialized()) {
    vidRecorder.setup(fileName + ofGetTimestampString() + fileExt, vidGrabber.getWidth(), vidGrabber.getHeight(), 30, sampleRate, channels);
    //          vidRecorder.setup(fileName+ofGetTimestampString()+fileExt, vidGrabber.getWidth(), vidGrabber.getHeight(), 30); // no audio
    //            vidRecorder.setup(fileName+ofGetTimestampString()+fileExt, 0,0,0, sampleRate, channels); // no video
    //          vidRecorder.setupCustomOutput(vidGrabber.getWidth(), vidGrabber.getHeight(), 30, sampleRate, channels, "-vcodec mpeg4 -b 1600k -acodec mp2 -ab 128k -f mpegts udp://localhost:1234"); // for custom ffmpeg output string (streaming, etc)

    // Start recording
    vidRecorder.start();
  } else if (!bRecording && vidRecorder.isInitialized()) {
    vidRecorder.setPaused(true);
  } else if (bRecording && vidRecorder.isInitialized()) {
    vidRecorder.setPaused(false);
  }
  std::string pong = "recordStarted";
  std::string param = "foo";
  socketIO.emit(pong, param);
}

void ofApp::onRecordEndEvent(ofxSocketIOData & data) {
  ofLogNotice("ofxSocketIO", "recordEnd");
  bRecording = false;
  vidRecorder.close();
  std::string pong = "recordEnded";
  std::string param = "foo";
  socketIO.emit(pong, param);
}

void ofApp::onSetDeviceIDEvent (ofxSocketIOData& data) {
  ofLogNotice("onSetDeviceIDEvent", ofToString(data.getIntValue("deviceID")));
  soundStream.setDeviceID(data.getIntValue("deviceID"));
  soundStream.setup(this, 0, channels ,sampleRate, 256, 4);
  bDeviceIdSet = true;
  //soundStream.setDeviceID(data.getIntValue("deviceID"));
}


//--------------------------------------------------------------
void ofApp::update(){
  //lets scale the vol up to a 0-1 range
  scaledVol = ofMap(smoothedVol, 0.0, 0.17, 0.0, 1.0, true);

  //lets record the volume into an array
  volHistory.push_back( scaledVol );

  //if we are bigger the the size we want to record - lets drop the oldest value
  if( volHistory.size() >= 400 ){
    volHistory.erase(volHistory.begin(), volHistory.begin()+1);
  }

  vidGrabber.update();
    if(vidGrabber.isFrameNew() && bRecording){
        bool success = vidRecorder.addFrame(vidGrabber.getPixels());
        if (!success) {
            ofLogWarning("This frame was not added!");
        }
    }

    // Check if the video recorder encountered any error while writing video frame or audio smaples.
    if (vidRecorder.hasVideoError()) {
        ofLogWarning("The video recorder failed to write some frames!");
    }

    if (vidRecorder.hasAudioError()) {
        ofLogWarning("The video recorder failed to write some audio samples!");
    }
}

//--------------------------------------------------------------
void ofApp::draw(){

  ofSetColor(225);
  ofDrawBitmapString("AUDIO INPUT EXAMPLE", 32, 32);
  ofDrawBitmapString("press 's' to unpause the audio\n'e' to pause the audio", 31, 92);

  ofNoFill();

  // draw the left channel:
  ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 170, 0);

    ofSetColor(225);
    ofDrawBitmapString("Left Channel", 4, 18);

    ofSetLineWidth(1);
    ofDrawRectangle(0, 0, 512, 200);

    ofSetColor(245, 58, 135);
    ofSetLineWidth(3);

      ofBeginShape();
      for (unsigned int i = 0; i < left.size(); i++){
        ofVertex(i*2, 100 -left[i]*180.0f);
      }
      ofEndShape(false);

    ofPopMatrix();
  ofPopStyle();

  // draw the right channel:
  ofPushStyle();
    ofPushMatrix();
    ofTranslate(32, 370, 0);

    ofSetColor(225);
    ofDrawBitmapString("Right Channel", 4, 18);

    ofSetLineWidth(1);
    ofDrawRectangle(0, 0, 512, 200);

    ofSetColor(245, 58, 135);
    ofSetLineWidth(3);

      ofBeginShape();
      for (unsigned int i = 0; i < right.size(); i++){
        ofVertex(i*2, 100 -right[i]*180.0f);
      }
      ofEndShape(false);

    ofPopMatrix();
  ofPopStyle();

  // draw the average volume:
  ofPushStyle();
    ofPushMatrix();
    ofTranslate(565, 170, 0);

    ofSetColor(225);
    ofDrawBitmapString("Scaled average vol (0-100): " + ofToString(scaledVol * 100.0, 0), 4, 18);
    ofDrawRectangle(0, 0, 400, 400);

    ofSetColor(245, 58, 135);
    ofFill();
    ofDrawCircle(200, 200, scaledVol * 190.0f);

    //lets draw the volume history as a graph
    ofBeginShape();
    for (unsigned int i = 0; i < volHistory.size(); i++){
      if( i == 0 ) ofVertex(i, 400);

      ofVertex(i, 400 - volHistory[i] * 70);

      if( i == volHistory.size() -1 ) ofVertex(i, 400);
    }
    ofEndShape(false);

    ofPopMatrix();
  ofPopStyle();

  drawCounter++;

  ofSetColor(225);
  string reportString = "buffers received: "+ofToString(bufferCounter)+"\ndraw routines called: "+ofToString(drawCounter)+"\nticks: " + ofToString(soundStream.getTickCount());
  ofDrawBitmapString(reportString, 32, 589);

  ofSetColor(255, 255, 255);

    stringstream ss;
    ss << "video queue size: " << vidRecorder.getVideoQueueSize() << endl
    << "audio queue size: " << vidRecorder.getAudioQueueSize() << endl
    << "FPS: " << ofGetFrameRate() << endl
    << (bRecording?"pause":"start") << " recording: r" << endl
    << (bRecording?"close current video file: c":"") << endl;

    if(bRecording){
    ofSetColor(255, 0, 0);
    ofDrawCircle(ofGetWidth() - 20, 20, 5);
    }

     ofDrawBitmapStringHighlight(ofApp::status, 20, 20);
}

//--------------------------------------------------------------
void ofApp::audioIn(float * input, int bufferSize, int nChannels){

  if(bDeviceIdSet){

  float curVol = 0.0;

  // samples are "interleaved"
  int numCounted = 0;

  //lets go through each sample and calculate the root mean square which is a rough way to calculate volume
  for (int i = 0; i < bufferSize; i++){
    left[i]   = input[i*2]*0.5;
    right[i]  = input[i*2+1]*0.5;

    curVol += left[i] * left[i];
    curVol += right[i] * right[i];
    numCounted+=2;
  }

  //this is how we get the mean of rms :)
  curVol /= (float)numCounted;

  // this is how we get the root of rms :)
  curVol = sqrt( curVol );

  smoothedVol *= 0.93;
  smoothedVol += 0.07 * curVol;

  bufferCounter++;

  if(bRecording)
        vidRecorder.addAudioSamples(input, bufferSize, nChannels);

  }
}

//--------------------------------------------------------------
void ofApp::exit(){
    ofRemoveListener(vidRecorder.outputFileCompleteEvent, this, &ofApp::recordingComplete);
    vidRecorder.close();
}

//--------------------------------------------------------------
void ofApp::recordingComplete(ofxVideoRecorderOutputFileCompleteEventArgs& args){
    cout << "The recoded video file is now complete." << endl;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if( key == 's' ){
    soundStream.start();
  }

  if( key == 'e' ){
    soundStream.stop();
  }

  if(key=='r'){

    }
    if(key=='c'){
        bRecording = false;
        vidRecorder.close();
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
