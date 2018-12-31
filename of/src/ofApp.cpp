#include "AppConfig.h"

#include "ofApp.h"
#include "main.cpp.impl" // weird but remove useless main compilation unit (rpi is slooow)

#define USE_TESTIMG 1
#define USE_SHADERS 1



#if USE_TESTIMG
ofImage testImg;
#endif


ofApp::ofApp(){}
ofApp::~ofApp(){}
//--------------------------------------------------------------
void ofApp::setup()
{
    // shaderFx = new ShaderFx();
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetLogLevel("ofThread", OF_LOG_ERROR);
    doDrawInfo = false;
    doPrintInfo = false;


    //Hide mouse
    ofHideCursor();

    // OSC
    receiver.setup(12345);

    // Shaders
    root = make_shared<ParameterContainer>("root");
    Node::setRoot(root);
    auto tfps = root->addParameter<ActionParameter>("targetFPS",[this](const string & s){
        float tfps = MAX(10.0,ofToFloat(s));ofSetFrameRate(tfps);DBG("setting FPS to " << tfps);
    });
    presetManager = make_shared<PresetManager>(root);
    root->addSharedParameterContainer(presetManager);
    presetManager->setup(ofFile("presets").getAbsolutePath());


    shaderFx = make_shared<ShaderFx>();
    root->addSharedParameterContainer(shaderFx);
#if USE_SHADERS
    shaderFx->setup();
#endif

    oscBind.setup(root,"localhost",11001);

    lastFrameTime = ofGetElapsedTimef();

#ifndef EMULATE_ON_OSX
    //allows keys to be entered via terminal remotely (ssh)
    consoleListener.setup(this);
    ofxOMXCameraSettings settings;
    ofBuffer jsonBuffer = ofBufferFromFile("settings.json");
    settings.parseJSON(jsonBuffer.getText());
    settings.enableTexture = bool(USE_SHADERS);
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

#if USE_ARB
    ofEnableArbTex();
#else
    ofDisableArbTex();
#endif

#if USE_TESTIMG
    testImg.load("images/tst.jpg");
#else
    videoGrabber.setup(ofGetWidth(),ofGetHeight(),true);
#endif


#endif

}


//--------------------------------------------------------------
void ofApp::update()
{
    shaderFx->update();
#if EMULATE_ON_OSX && !USE_TESTIMG
    videoGrabber.update();
#endif

    processOSC();
}


//--------------------------------------------------------------
void ofApp::draw()
{
#if USE_TESTIMG
    testImg.resize(ofGetWidth(),ofGetHeight());
#endif

    ofTexture & drawnTexture =
#if USE_TESTIMG
    testImg.getTexture();
#elif EMULATE_ON_OSX
    videoGrabber.getTexture();
#else
    videoGrabber.getTextureReference();
#endif

#if USE_SHADERS
    ofSetColor(255);
    auto curT = ofGetElapsedTimef();
    float deltaT = curT - lastFrameTime ;
    lastFrameTime  =curT;

    shaderFx->draw(drawnTexture,deltaT);
    shaderFx->drawDbg();

#elif EMULATE_ON_OSX
    drawnTexture.draw(0,0,ofGetWidth(),ofGetHeight());
#endif

    drawInfoIfAsked();

}

void ofApp::processOSC(){
    while(receiver.hasWaitingMessages()){
        // get the next message
        ofxOscMessage m;
        receiver.getNextMessage(m);


        int value = m.getArgAsInt32(0);
        auto splitted = ofSplitString(m.getAddress(), "/",true);

        if( oscBind.processOSC(m,0)) continue;

        if(splitted.size()<2)continue;

        string add0= splitted[0];
        string add1= splitted[1];
        ofLogVerbose() << "\n osc message received add0: " << add0 << " add1 " << add1 << " value: "<< ofToString(value);

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
            presetManager->savePreset(shaderFx,"1");
            break;
        }
        case 'm' :
        {
            presetManager->recallPreset(shaderFx,"1");
            break;
        }
        case 's':{
            if(!shaderFx->reload()){
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

void ofApp::drawInfoIfAsked(){
    if(ofGetFrameNum()%60*2==0){
        ofLogVerbose() << ofGetFrameRate();
    }
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
        //        info << shaderFx->getInfo() << endl;

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

