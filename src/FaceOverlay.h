#pragma once

#include "ofMain.h"
#include "ofxFaceTracker.h"
#include "ofxTiming.h"

class FaceOverlay {
private:
    
    void drawCurve(const ofPolyline& polyline, ofxIlda::Frame& frame) {
        if(polyline.size() == 0) {
            return;
        }
        frame.addPoly(polyline);
    }
    
    void drawPolyline(const ofPolyline& polyline, ofxIlda::Frame& frame) {
        if(polyline.size() == 0) {
            return;
        }
        frame.addPoly(polyline);
    }
    
    void drawMesh(const ofMesh& mesh, ofxIlda::Frame& frame) {
        ofPolyline polyline;
        ofVec2f last;
        for(int i = 0; i < mesh.getNumVertices(); i++) {
            ofVec2f cur = mesh.getVertex(i);
            if(cur != last) {
                polyline.addVertex(cur);
            }
            last = cur;
        }
        drawPolyline(polyline, frame);
    }
    
    static ofPolyline buildCircle(const ofVec2f& center, float radius, int resolution = 32) {
        ofPolyline polyline;
        ofVec2f base(radius, 0);
        for(int i = 0; i < resolution; i++) {
            float angle = 360 * (float) i / resolution;
            ofVec2f cur = center + base.getRotated(angle);
            polyline.addVertex(cur);
        }
        polyline.close();
        return polyline;
    }
    
    static float determinant(const ofVec2f& start, const ofVec2f& end, const ofVec2f& point) {
        return (end.x - start.x) * (point.y - start.y) - (end.y - start.y) * (point.x - start.x);
    }
    
    static void divide(const ofVec2f& a, const ofVec2f& b,
                const ofVec2f& left, const ofVec2f& right,
                ofMesh& top, ofMesh& bottom) {
        ofVec2f avg = a.getInterpolated(b, 0.5);
        if(determinant(left, right, avg) < 0) {
            top.addVertex(a);
            top.addVertex(b);
        } else {
            bottom.addVertex(a);
            bottom.addVertex(b);
        }
    }
    
    static void divide(const ofPolyline& poly,
                const ofVec2f& left, const ofVec2f& right,
                ofMesh& top, ofMesh& bottom) {
        top.setMode(OF_PRIMITIVE_LINES);
        bottom.setMode(OF_PRIMITIVE_LINES);
        const vector<ofVec3f>& vertices = poly.getVertices();
        for(int i = 0; i + 1 < vertices.size(); i++) {
            divide(vertices[i], vertices[i + 1], left, right, top, bottom);
        }
        if(poly.isClosed()) {
            divide(vertices.back(), vertices.front(), left, right, top, bottom);
        }
    }
    
public:
    Hysteresis noseHysteresis, mouthHysteresis;
    float noseAngle = 15;
    float nostrilWidth = .06;
    float lipWidth = .7;
    float scale = 1;
    
    FaceOverlay() {
        noseHysteresis.setDelay(1000);
        mouthHysteresis.setDelay(1000);
    }
    void draw(ofxFaceTracker& tracker, ofxIlda::Frame& frame) {
        if(noseHysteresis.set(abs(tracker.getOrientation().y) > ofDegToRad(noseAngle))) {
            drawPolyline(tracker.getImageFeature(ofxFaceTracker::NOSE_BRIDGE), frame);
        }
        
        ofPolyline eyeLeft = tracker.getImageFeature(ofxFaceTracker::LEFT_EYE);
        ofPolyline eyeRight = tracker.getImageFeature(ofxFaceTracker::RIGHT_EYE);
        ofVec2f eyeLeftLeft = tracker.getImagePoint(36);
        ofVec2f eyeLeftRight = tracker.getImagePoint(39);
        ofVec2f eyeRightLeft = tracker.getImagePoint(42);
        ofVec2f eyeRightRight = tracker.getImagePoint(45);
        float eyeRadius = tracker.getScale() * 3;
        
        float lidHeight = .5;
        ofVec2f eyeLeftLidLeft = tracker.getImagePoint(36).getInterpolated(tracker.getImagePoint(37), lidHeight);
        ofVec2f eyeLeftLidRight = tracker.getImagePoint(38).getInterpolated(tracker.getImagePoint(39), lidHeight);
        ofVec2f eyeRightLidLeft = tracker.getImagePoint(42).getInterpolated(tracker.getImagePoint(43), lidHeight);
        ofVec2f eyeRightLidRight = tracker.getImagePoint(44).getInterpolated(tracker.getImagePoint(45), lidHeight);
        
        ofVec2f eyeCenterLeft = eyeLeftLidLeft.getInterpolated(eyeLeftLidRight, .5);
        ofVec2f eyeCenterRight = eyeRightLidLeft.getInterpolated(eyeRightLidRight, .5);
        
        float irisSize = .5;
        ofMesh leftTop, leftBottom;
        ofPolyline leftCircle = buildCircle(eyeCenterLeft, eyeRadius * irisSize);
        divide(leftCircle, eyeLeftLidLeft, eyeLeftLidRight, leftTop, leftBottom);
        ofMesh rightTop, rightBottom;
        ofPolyline rightCircle = buildCircle(eyeCenterRight, eyeRadius * irisSize);
        divide(rightCircle, eyeRightLidLeft, eyeRightLidRight, rightTop, rightBottom);
        
        drawMesh(leftBottom, frame);
        drawMesh(rightBottom, frame);
        
        ofPolyline lowerLip;
        lowerLip.addVertex(tracker.getImagePoint(59).getInterpolated(tracker.getImagePoint(48), lipWidth));
        lowerLip.addVertex(tracker.getImagePoint(59));
        lowerLip.addVertex(tracker.getImagePoint(58));
        lowerLip.addVertex(tracker.getImagePoint(57));
        lowerLip.addVertex(tracker.getImagePoint(56));
        lowerLip.addVertex(tracker.getImagePoint(55));
        lowerLip.addVertex(tracker.getImagePoint(55).getInterpolated(tracker.getImagePoint(54), lipWidth));
        drawCurve(lowerLip, frame);
        
        ofPolyline innerLip;
        innerLip.addVertex(tracker.getImagePoint(60).getInterpolated(tracker.getImagePoint(48), lipWidth));
        innerLip.addVertex(tracker.getImagePoint(60));
        innerLip.addVertex(tracker.getImagePoint(61));
        innerLip.addVertex(tracker.getImagePoint(62));
        innerLip.addVertex(tracker.getImagePoint(62).getInterpolated(tracker.getImagePoint(54), lipWidth));
        drawCurve(innerLip, frame);
        
        // if mouth is open than some amount, draw this line
        if(mouthHysteresis.set(tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT) > 2)) {
            ofPolyline innerLowerLip;
            innerLowerLip.addVertex(tracker.getImagePoint(65).getInterpolated(tracker.getImagePoint(48), lipWidth));
            innerLowerLip.addVertex(tracker.getImagePoint(65));
            innerLowerLip.addVertex(tracker.getImagePoint(64));
            innerLowerLip.addVertex(tracker.getImagePoint(63));
            innerLowerLip.addVertex(tracker.getImagePoint(63).getInterpolated(tracker.getImagePoint(54), lipWidth));
            drawCurve(innerLowerLip, frame);
        }
        
        // nose
        ofPolyline nose;
        nose.addVertex(tracker.getImagePoint(31).getInterpolated(tracker.getImagePoint(4), nostrilWidth));
        nose.addVertex(tracker.getImagePoint(31));
        nose.addVertex(tracker.getImagePoint(32));
        nose.addVertex(tracker.getImagePoint(33));
        nose.addVertex(tracker.getImagePoint(34));
        nose.addVertex(tracker.getImagePoint(35));
        nose.addVertex(tracker.getImagePoint(35).getInterpolated(tracker.getImagePoint(12), nostrilWidth));
        drawCurve(nose, frame);
        
        ofPolyline upperLip;
        upperLip.addVertex(tracker.getImagePoint(49).getInterpolated(tracker.getImagePoint(48), lipWidth));
        upperLip.addVertex(tracker.getImagePoint(49));
        upperLip.addVertex(tracker.getImagePoint(50));
        upperLip.addVertex(tracker.getImagePoint(51));
        upperLip.addVertex(tracker.getImagePoint(52));
        upperLip.addVertex(tracker.getImagePoint(53));
        upperLip.addVertex(tracker.getImagePoint(53).getInterpolated(tracker.getImagePoint(54), lipWidth));
        drawCurve(upperLip, frame);
        
        drawCurve(tracker.getImageFeature(ofxFaceTracker::LEFT_EYE_TOP), frame);
        drawCurve(tracker.getImageFeature(ofxFaceTracker::RIGHT_EYE_TOP), frame);
        drawCurve(tracker.getImageFeature(ofxFaceTracker::LEFT_EYEBROW), frame);
        drawCurve(tracker.getImageFeature(ofxFaceTracker::RIGHT_EYEBROW), frame);
        drawCurve(tracker.getImageFeature(ofxFaceTracker::JAW), frame);
    }
};
