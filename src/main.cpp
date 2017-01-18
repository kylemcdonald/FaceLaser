#include "ofMain.h"

#include "ofxPS3EyeGrabber.h"
#include "ofxFaceTracker.h"
#include "ofxCv.h"
#include "ofxEtherdream.h"

#include "FaceOverlay.h"

using namespace ofxCv;

class ofApp: public ofBaseApp {
    public:
    
    FaceOverlay faceOverlay;
    ofxFaceTracker tracker;
    ofVideoGrabber grabber;
    ofxIlda::Frame ildaFrame;
    ofxEtherdream etherdream;
    
    ofImage gray;
    bool debug = true;
    
    void setup() {
        for(float x = -0.1; x < 1.1; x += 0.01) {
            int16_t y = ofMap(x, 0, 1, kIldaMinPoint, kIldaMaxPoint);
            cout << x << " " << y << endl;
        }
        exit();
        
        etherdream.setup();
        etherdream.setPPS(30000);
        
        grabber.setGrabber(std::make_shared<ofxPS3EyeGrabber>());
        grabber.setDeviceID(0);
        grabber.setPixelFormat(OF_PIXELS_NATIVE);
        grabber.setDesiredFrameRate(60);
        grabber.setup(640, 480);
        grabber.getGrabber<ofxPS3EyeGrabber>()->setAutogain(true);
        grabber.getGrabber<ofxPS3EyeGrabber>()->setAutoWhiteBalance(true);
        
        gray.allocate(grabber.getHeight(), grabber.getHeight(), OF_IMAGE_GRAYSCALE);
        
        faceOverlay.scale = 1. / grabber.getHeight();
        
        tracker.setup();
    }
    
    void update() {
        grabber.update();
        if(grabber.isFrameNew()) {
            unsigned char* yuy2Pixels = grabber.getPixels().getData();
            unsigned char* grayPixels = gray.getPixels().getData();
            int w = grabber.getWidth();
            int h = grabber.getHeight();
            int skip = (w - h);
            int off = skip / 2;
            int end = off + w;
            int i = 0;
            int j = off;
            for(int y = 0; y < h; y++) {
                for(int x = 0; x < h; x++) {
                    grayPixels[i] = yuy2Pixels[j];
                    i += 1;
                    j += 2;
                }
                j += skip * 2;
            }
            
            tracker.update(toCv(gray));
            
            if(debug) {
                gray.update();
            }
        
            if(etherdream.stateIsFound() && tracker.getFound()) {
                ildaFrame.clear();
                faceOverlay.draw(tracker, ildaFrame);
                
                ildaFrame.params.output.transform.doFlipY = true;
                ildaFrame.params.output.transform.offset.set(0.5, 0.5);
                ildaFrame.params.output.transform.scale.set(1./480, 1./480);
                ildaFrame.polyProcessor.params.targetPointCount = mouseX;
                ildaFrame.update();
                etherdream.setPoints(ildaFrame);
            }
        }
    }
    
    void draw() {
        ofBackground(0);
        ofSetColor(255);
        
        if(debug) {
            gray.draw(0, 0);
        }
        
        ofNoFill();
        tracker.draw();
//        faceOverlay.draw(tracker);
        
        ildaFrame.draw(0, 0, ofGetWidth(), ofGetHeight());
        
//        ofSetColor(255);
//        ofDrawBitmapString(ildaFrame.getString(), 10, 30);
        
        std::stringstream ss;
        ss << " App FPS: " << ofGetFrameRate() << std::endl;
        ss << " Cam FPS: " << grabber.getGrabber<ofxPS3EyeGrabber>()->getFPS()  << std::endl;
        ss << "Real FPS: " << grabber.getGrabber<ofxPS3EyeGrabber>()->getActualFPS();
        ofDrawBitmapStringHighlight(ss.str(), ofPoint(10, 15));
    }
    
    void keyPressed(int key) {
        if(key == 'd') {
            debug = !debug;
        }
    }
};

int main() {
    ofGLWindowSettings settings;
    settings.setGLVersion(3, 2);
    settings.width = 480;
    settings.height = 480;
    ofCreateWindow(settings);
    ofRunApp(std::make_shared<ofApp>());
}
