#pragma once

#include "ofMain.h"
#include "ofxVideoRecorder.h"
#include "ofEvents.h"
#include "ofxSocketIO.h"
#include "ofxSocketIOData.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void exit();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		void audioIn(float * input, int bufferSize, int nChannels);

		vector <float> left;
		vector <float> right;
		vector <float> volHistory;

		int 	bufferCounter;
		int 	drawCounter;

		float smoothedVol;
		float scaledVol;

		ofSoundStream soundStream;

		ofVideoGrabber      vidGrabber;
    ofxVideoRecorder    vidRecorder;
    bool bRecording;
    int sampleRate;
    int channels;
    string fileName;
    string fileExt;

    void recordingComplete(ofxVideoRecorderOutputFileCompleteEventArgs& args);

    ofFbo recordFbo;
    ofPixels recordPixels;


    //socket
    void gotEvent(std::string& name);
    void onServerEvent(ofxSocketIOData& data);
    void onPingEvent(ofxSocketIOData& data);

    ofxSocketIO socketIO;

    bool isConnected;
    void onConnection();
    void bindEvents();
    ofEvent<ofxSocketIOData&> serverEvent;
    ofEvent<ofxSocketIOData&> pingEvent;

    std::string address;
    std::string status;

};
