#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup()
{

    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetLogLevel("ofThread", OF_LOG_ERROR);
    doDrawInfo = false;    
    doPrintInfo = false;
    

    //Hide mouse
    ofHideCursor();

    // OSC			
    receiver.setup(12345);

    // Shaders
    shaderFx.setup();


    lastFrameTime = ofGetElapsedTimef();

    #ifndef EMULATE_ON_OSX
    //allows keys to be entered via terminal remotely (ssh)
    consoleListener.setup(this);
    ofxOMXCameraSettings settings;
    ofBuffer jsonBuffer = ofBufferFromFile("settings.json");
    settings.parseJSON(jsonBuffer.getText());

    videoGrabber.setup(settings);
    int settingsCount = 0;

    SettingsEnhancement* enhancement= new SettingsEnhancement();
    enhancement->setup(&videoGrabber);
    enhancement->name = "enhancement";
    listOfSettings[settingsCount] = enhancement;
    settingsCount++;

    SettingsZoomCrop* zoomCrop= new SettingsZoomCrop();
    zoomCrop->setup(&videoGrabber);
    zoomCrop->name = "zoomCrop";
    listOfSettings[settingsCount] = zoomCrop;
    settingsCount++;

    SettingsFilters* filters = new SettingsFilters();
    filters->setup(&videoGrabber);
    filters->name = "filters";
    listOfSettings[settingsCount] = filters;
    settingsCount++;

    SettingsWhiteBalance* whiteBalance = new SettingsWhiteBalance();
    whiteBalance->setup(&videoGrabber);
    whiteBalance->name = "whiteBalance";
    listOfSettings[settingsCount] = whiteBalance;
    settingsCount++;
    #else
    #define USE_ARB 1
#if USE_ARB
    ofEnableArbTex();
    videoGrabber.setup(ofGetWidth(),ofGetHeight(),GL_TEXTURE_RECTANGLE_ARB);
#else
    ofDisableArbTex();
    videoGrabber.setup(ofGetWidth(),ofGetHeight(),GL_TEXTURE_2D);
#endif
    

    #endif
    
}


//--------------------------------------------------------------
void ofApp::update()
{
    shaderFx.update();

	while(receiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		receiver.getNextMessage(m);
        

		int value = m.getArgAsInt32(0);
        auto splitted = ofSplitString(m.getAddress(), "/",true);
        if(splitted.size()<2)continue;
        
		string add0= splitted[0];
        string add1= splitted[1];
        ofLogVerbose() << "\n osc message received add0: " << add0 << " add1 " << add1 << " value: "<< ofToString(value);

        if(add0 == "shader" && shaderFx.processOSCMessage(m,splitted,1)) continue;
        #ifndef EMULATE_ON_OSX
        for (int i=0; i<NB_SETTINGS; i++){
          ofLogVerbose() << " For: " << ofToString(i);

          if( add0 == (listOfSettings[i]->name)){
            ofLogVerbose() << "\n OSC settings:" << add0 << " - " << add1 << " : " << ofToString(value);

                //ROUTE osc message according the type of settings
                // Then transmit the adress and value to the specific setting class
                (listOfSettings[i])->onOsc(add1, value);

                //Finally update the class to apply new settings
                (listOfSettings[i])->update();

            }


        }
        ofLogVerbose() << " end of For: " ;
        #endif
        
        if( add0 == "transport"){
           transport = add1;
       }



   }





}


//--------------------------------------------------------------
void ofApp::draw()
{
    #if EMULATE_ON_OSX
    ofSetColor(255);
    videoGrabber.update();
    ofVec2f reso(ofGetWidth(),ofGetHeight());
    auto curT = ofGetElapsedTimef();
    float deltaT = curT - lastFrameTime ;
    lastFrameTime  =curT;
    shaderFx.begin(reso,deltaT);
    videoGrabber.draw(0,0,ofGetWidth(),ofGetHeight());
    shaderFx.end();
    #endif


    if (doDrawInfo || doPrintInfo) 
    {
        stringstream info;
        info << endl;
        info << "App FPS: " << ofGetFrameRate() << endl;
        info << "CAMERA RESOLUTION: "   << videoGrabber.getWidth() << "x" << videoGrabber.getHeight()
        #ifndef EMULATE_ON_OSX
         <<" @ "<< videoGrabber.getFrameRate() <<"FPS"
         #endif
         << endl;
        info << endl;
        info << endl;
        info << "Press SPACE for next Demo" << endl;
        info << "Press r to reset camera settings" << endl;
        info << "Press z TO START RECORDING" << endl;
        info << "Press x TO STOP RECORDING" << endl;
        info << shaderFx.getCurrentShaderInfo() << endl;
        
        if (doDrawInfo) 
        {
            int x = 100;
            if(videoGrabber.getWidth()<1280)
            {
                x = videoGrabber.getWidth();
            }
            ofDrawBitmapStringHighlight(info.str(), x, 40, ofColor(ofColor::black, 50), ofColor::yellow);
        }
        if (doPrintInfo) 
        {
            ofLogVerbose() << info.str();
            doPrintInfo = false;
        }
    }

}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key)
{
    ofLog(OF_LOG_VERBOSE, "%c keyPressed", key);
    switch (key) 
    {

        case 'd':
        {
            doDrawInfo = !doDrawInfo;
            break;
        }
        case 'i':
        {
            doPrintInfo = !doPrintInfo;
            break;
        }
        case 'p' :
        {
            break;
        }
        case 's':{
            if(!shaderFx.reload()){
                ofLogError() << "couldn't reload shader";
            }
        }

#ifndef EMULATE_ON_OSX
        case 'r' :
        {
            videoGrabber.reset();
            break;
        }
        case 'z' :
        {
            videoGrabber.startRecording();
            break;
        }
        case 'x' :
        {
            videoGrabber.stopRecording();
            break;
        }
#endif
        default:
        {
            break;
        }

    }
}

#ifndef EMULATE_ON_OSX
void ofApp::onCharacterReceived(KeyListenerEventData& e)
{
    keyPressed((int)e.character);
}
#endif